#pragma once

#include "htb_types.hpp"
#include "htb_includes.hpp"
#include "htb_functions.hpp"

namespace htb
{

///////////////////////////////////////////////////// cell

struct environment; // forward declaration; cell and environment reference each other

// a variant that can hold any kind of lisp value
struct cell
{
    typedef cell (*proc_type)(const std::vector<cell>&);
    typedef std::vector<cell>::const_iterator iter;
    typedef std::map<std::string, cell> map;

    cell_type type;
    std::string val;
    std::vector<cell> list;
    map dict;
    proc_type proc;
    environment* env;
    bool const_expr;

    cell(cell_type type=Symbol) :
        type(type)
        , env(0)
        , const_expr(false)
    {}
    cell(cell_type type, const std::string& val) :
        type(type)
        , val(val)
        , env(0)
        , const_expr(false)
    {}
    cell(proc_type proc) :
        type(Proc)
        , proc(proc)
        , env(0)
        , const_expr(false)
    {}

    cell get_in(const std::string& key)
    {
        RAISE_IF(type != Dict, "Can not access a sub element because the object is not a dict")
        RAISE_IF(dict.empty(), "Can not access an element with the key " << key << " because the dict is empty")
        RAISE_IF(dict.find(key) == dict.end(), "Can not find the key " << key << " in the dict")
        return dict[key];
    }
    cell get_in(long n)
    {
        RAISE_IF(type != List, "Can not access a sub element because the object is not a list")
        RAISE_IF(n >= long(list.size()), "Can not find the " << n << "th element in the list")
        return list[n];
    }

    bool operator==(const cell& r) const
    {
        return std::addressof(*this) == std::addressof(r);
    }

    bool operator!=(const cell& r) const
    {
        return ! (r == *this);
    }
};

typedef std::vector<cell> cells;
typedef cells::const_iterator cellit;

const cell false_sym(Symbol, "false");
const cell true_sym(Symbol, "true"); // anything that isn't false_sym is true
const cell nil(Symbol, "nil");

///////////////////////////////////////////////////// environment

// a dictionary that (a) associates symbols with cells, and
// (b) can chain to an "outer" dictionary
struct environment {
    environment(environment* outer=0, bool isolated=false) :
        outer_(outer)
        , isolated_(isolated)
    {}

    environment(const cells& parms, const cells& args, environment* outer) :
        outer_(outer)
        , isolated_(false)
    {
        cellit a = args.begin();
        for (cellit p = parms.begin(); p != parms.end(); ++p)
        {
            env_[p->val] = *a++;
        }
    }

    // map a variable name onto a cell
    typedef std::map<std::string, cell> map;

    // return a reference to the innermost environment where 'var' appears
    map& find(const std::string& var)
    {
        if (env_.find(var) != env_.end())
            return env_; // the symbol exists in this environment
        if (outer_)
            return outer_->find(var); // attempt to find the symbol in some "outer" env

        std::stringstream ss; ss << "Unbound symbol '" << var << "'";
        errors[var] = cell(Exception, ss.str());
        return errors;
    }

    // return a reference to the cell associated with the given symbol 'var'
    cell& operator[] (const std::string & var)
    {
        return env_[var];
    }

    // return true if the environment is isolated
    bool is_isolated()
    {
        return isolated_;
    }

    bool has_outer()
    {
        return outer_ != 0;
    }

    // get a namespace or create one
    environment* get_namespace(const std::string& name, bool is_ins=false)
    {
        if (outer_ == 0)
        {
            if (namespaces.find(name) == namespaces.end())
            {
                if (!is_ins)
                {
                    namespaces[name] = new environment(this);
                }
                else
                {
                    namespaces[name] = new environment(0, true);
                }
            }
        }
        else
        {
            outer_->get_namespace(name, is_ins);
        }

        return namespaces[name];
    }

private:
    map env_; // inner symbol->cell mapping
    map errors;
    std::map<std::string, environment*> namespaces;
    environment* outer_; // next adjacent outer env, or 0 if there are no further environments
    bool isolated_;
};

} // namespace htb
