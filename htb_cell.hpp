/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#pragma once

#include "htb_types.hpp"
#include "htb_includes.hpp"
#include "htb_internal.hpp"

namespace htb
{

    constexpr long INF_NB_ARGS  = -1;
    constexpr long AT_LEAST_1_ARGS  = -2;
    constexpr long AT_LEAST_2_ARGS  = -3;
    constexpr long BETWEEN_0_1_ARGS = -4;
    constexpr long BETWEEN_0_2_ARGS = -5;

    ///////////////////////////////////////////////////// cell

    struct environment; // forward declaration; cell and environment reference each other
    struct cell;

    typedef cell (*proc_type)(const std::vector<cell>&);
    typedef std::vector<cell> cells;
    typedef cells::const_iterator cellit;

    // a variant that can hold any kind of Hitoban value
    struct cell
    {
        typedef std::vector<cell>::const_iterator iter;
        typedef std::map<std::string, cell> map;

        cell_type type;
        std::string val;
        std::vector<cell> list;
        map dict;
        proc_type proc;
        environment* env;
        bool const_expr;
        long number_of_args;

        cell(cell_type type=Symbol) :
            type(type)
            , env(0)
            , const_expr(false)
            , number_of_args(0)
        {}

        template <typename T>
        cell(cell_type type, T val) :
            type(type)
            , val(internal::str<T>(val))
            , env(0)
            , const_expr(false)
            , number_of_args(0)
        {}

        cell(proc_type proc, long n=0) :
            type(Proc)
            , proc(proc)
            , env(0)
            , const_expr(false)
            , number_of_args(n)
        {}

        cell(const cell& c) :
            type(c.type)
            , val(c.val)
            , list(c.list)
            , dict(c.dict)
            , proc(c.proc)
            , env(c.env)
            , const_expr(c.const_expr)
            , number_of_args(c.number_of_args)
        {}

        void init_from(cell* c)
        {
            type = c->type;
            val = c->val;
            list = c->list;
            dict = c->dict;
            proc = c->proc;
            env = c->env;
            const_expr = c->const_expr;
            number_of_args = c->number_of_args;
        }

        cell exec(const cells& c, const std::string& name)
        {
            // first check len of args
            HTB_RAISE_IF(number_of_args != INF_NB_ARGS && long(c.size()) != number_of_args, "'" << name << "' needs " << number_of_args << " not " << c.size())
            HTB_RAISE_IF(number_of_args == AT_LEAST_1_ARGS && long(c.size()) < 1, "'" << name << "' needs at least 1 argument not " << c.size())
            HTB_RAISE_IF(number_of_args == AT_LEAST_2_ARGS && long(c.size()) < 2, "'" << name << "' needs at least 2 arguments not " << c.size())
            HTB_RAISE_IF(number_of_args == BETWEEN_0_1_ARGS && long(c.size()) > 1, "'" << name << "' needs 1 or 0 argument, not " << c.size())
            HTB_RAISE_IF(number_of_args == BETWEEN_0_2_ARGS && long(c.size()) > 2, "'" << name << "' needs between 0 and 2 arguments, not " << c.size())
            // then check if all arguments are not exceptions
            if (number_of_args > 0)
                for (long i=0; i < number_of_args; ++i)
                {
                    HTB_HANDLE_EXCEPTION(c[i])
                }
            // finally exec the procedure
            return proc(c);
        }

        cell get_in(const std::string& key)
        {
            HTB_RAISE_IF(type != Dict, "Can not access a sub element because the object is not a dict")
            HTB_RAISE_IF(dict.empty(), "Can not access an element with the key " << key << " because the dict is empty")
            HTB_RAISE_IF(dict.find(key) == dict.end(), "Can not find the key " << key << " in the dict")

            return dict[key];
        }

        cell get_in(long n)
        {
            HTB_RAISE_IF(type != List, "Can not access a sub element because the object is not a list")
            HTB_RAISE_IF(n >= long(list.size()), "Can not find the " << n << "th element in the list")

            return list[n];
        }

        bool operator==(const cell& r) const
        {
            return std::addressof(*this) == std::addressof(r);
        }

        bool operator!=(const cell& r) const
        {
            return !(r == *this);
        }
    };  // struct cell

    const cell false_sym(Symbol, "false");
    const cell true_sym(Symbol, "true"); // anything that isn't false_sym is true_sym
    const cell nil(Symbol, "nil");

    ///////////////////////////////////////////////////// environment

    // a dictionary that (a) associates symbols with cells, and
    // (b) can chain to an "outer" dictionary
    struct environment {
        // map a variable name onto a cell
        typedef std::map<std::string, cell> map;

        bool isfile;
        std::string fname;

        environment(environment* outer=0) :
            isfile(false)
            , outer_(outer)
        {}

        environment(const cells& parms, const cells& args, environment* outer) :
            isfile(false)
            , outer_(outer)
        {
            if (parms.size() != args.size())
                throw std::runtime_error("Not as much arguments given as parameters expected");

            cellit a = args.begin();
            for (cellit p = parms.begin(); p != parms.end(); ++p)
            {
                env_[p->val] = *a++;
            }
        }

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

        bool has_outer()
        {
            return outer_ != 0;
        }

        // get a namespace or create one
        environment* get_namespace(const std::string& name)
        {
            bool found_in_self = namespaces.find(name) != namespaces.end();

            if (!found_in_self)
            {
                if (outer_ != 0)
                {
                    environment* temp = outer_->_get_namespace(name);
                    if (temp != 0)
                        return temp;
                }
                namespaces[name] = new environment(this);
            }
            return namespaces[name];
        }

        // get a namespace or return 0 if it doesn't exist
        environment* _get_namespace(const std::string& name)
        {
            bool found_in_self = namespaces.find(name) != namespaces.end();

            if (found_in_self)
                return namespaces[name];
            else
            {
                // check outer_ or return 0
                if (outer_ != 0)
                    return _get_namespace(name);
                return 0;
            }
        }

        // search for an environment which is a file, and return its path
        std::string get_parent_file()
        {
            if (isfile)  // simplest case
                return fname;
            else
            {
                if (outer_ != 0)  // we have a bigger environment containing this one
                    return outer_->get_parent_file();
                return fname;
            }
        }

        // return a list of string (names of the available namespaces)
        std::list<std::string> get_namespaces()
        {
            std::list<std::string> ns;
            for (auto kv : namespaces)
                ns.push_back(kv.first);
            return ns;
        }

    private:
        environment* outer_; // next adjacent outer env, or 0 if there are no further environments
        map env_; // inner symbol->cell mapping
        map errors;
        std::map<std::string, environment*> namespaces;
    };  // struct environment

    namespace types
    {

        using ret = cell;
        using args = const cells&;

    }  // namespace types

} // namespace htb
