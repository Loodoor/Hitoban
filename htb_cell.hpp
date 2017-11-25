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

    constexpr long INF_NB_ARGS      = -1;
    constexpr long AT_LEAST_1_ARGS  = -2;
    constexpr long AT_LEAST_2_ARGS  = -3;
    constexpr long BETWEEN_0_1_ARGS = -4;
    constexpr long BETWEEN_0_2_ARGS = -5;

    ///////////////////////////////////////////////////// cell

    struct environment; // forward declaration; cell and environment reference each other
    struct cell;

    typedef cell (*proc_type)(const std::vector<cell>&);
    typedef std::vector<cell> cells;
    typedef std::map<std::string, cell> cell_dict;
    typedef cells::const_iterator cellit;
    typedef cell return_type;
    typedef const cells& args_type;

    // a variant that can hold any kind of Hitoban value
    struct cell
    {
        typedef std::vector<cell>::const_iterator iter;

        // htb value holder #optimization
        struct value {
            std::string symbol_or_str;
            long number;
            cells lst;
            cell_dict dict;
            proc_type proc;

            long number_of_args;

            value() :
                symbol_or_str("")
                , number(0)
                //, lst()
                //, dict()
                , proc(nullptr)
                , number_of_args(0)
            {}

            value(const value& v) :
                symbol_or_str(v.symbol_or_str)
                , number(v.number)
                , lst(v.lst)
                , dict(v.dict)
                , proc(v.proc)
                , number_of_args(v.number_of_args)
            {}

            void init_from(value* v)
            {
                symbol_or_str = v->symbol_or_str;
                number = v->number;
                lst = v->lst;
                dict = v->dict;
                proc = v->proc;
                number_of_args = v->number_of_args;
            }

            template <typename T>
            T get()
            {
                if (typeid(T) == typeid(std::string))
                    return symbol_or_str;
                if (typeid(T) == typeid(long))
                    return number;
                if (typeid(T) == typeid(cells))
                    return lst;
                if (typeid(T) == typeid(cell_dict))
                    return dict;
                if (typeid(T) == typeid(proc_type))
                    return proc;
            }

            template <typename T>
            T get() const
            {
                if (typeid(T) == typeid(std::string))
                    return symbol_or_str;
                if (typeid(T) == typeid(long))
                    return number;
                if (typeid(T) == typeid(cells))
                    return lst;
                if (typeid(T) == typeid(cell_dict))
                    return dict;
                if (typeid(T) == typeid(proc_type))
                    return proc;
            }

            template <typename T>
            T& get_ref()
            {
                if (typeid(T) == typeid(std::string))
                    return symbol_or_str;
                if (typeid(T) == typeid(long))
                    return number;
                if (typeid(T) == typeid(cells))
                    return lst;
                if (typeid(T) == typeid(cell_dict))
                    return dict;
                if (typeid(T) == typeid(proc_type))
                    return proc;
            }

            template <typename T>
            void set(const T v)
            {
                if (typeid(T) == typeid(std::string))
                    symbol_or_str = v;
                if (typeid(T) == typeid(long))
                    number = v;
                if (typeid(T) == typeid(cells))
                    lst = v;
                if (typeid(T) == typeid(cell_dict))
                    dict = v;
                if (typeid(T) == typeid(proc_type))
                    proc = v;
            }

            bool operator==(const value& other)
            {
                return (symbol_or_str == other.symbol_or_str) &&
                       (number == other.number) &&
                       (lst == other.lst) &&
                       (dict == other.dict) &&
                       (&proc == &other.proc) &&
                       (number_of_args == other.number_of_args);
            }
        };

        value val;
        cell_type type;

        environment* env;
        bool const_expr;

        cell(cell_type type=Symbol) :
            type(type)
            , env(0)
            , const_expr(false)
        {}

        template <typename T>
        cell(cell_type type, T v) :
            type(type)
            , env(0)
            , const_expr(false)
        {
            val.set<T>(v);
        }

        cell(proc_type proc, long n=0) :
            type(Proc)
            , env(0)
            , const_expr(false)
        {
            val.set<proc_type>(proc);
            val.number_of_args = n;
        }

        cell(const cell& c) :
            val(c.val)
            , type(c.type)
            , env(c.env)
            , const_expr(c.const_expr)
        {}

        void init_from(cell* c)
        {
            type = c->type;
            val.init_from(&(c->val));
            env = c->env;
            const_expr = c->const_expr;
        }

        cell exec(const cells& c, const std::string& name)
        {
            // first check len of args
            //                          >= 0 means it's not a special code
            HTB_RAISE_IF(val.number_of_args >= 0 && long(c.size()) != val.number_of_args,
                         "'" << name << "' needs " << (val.number_of_args == AT_LEAST_1_ARGS ? "at least one" :
                                                       (val.number_of_args == AT_LEAST_2_ARGS ? "at least two" :
                                                        (val.number_of_args == BETWEEN_0_1_ARGS ? "between 0 and 1" :
                                                         (val.number_of_args == BETWEEN_0_2_ARGS ? "between 0 and 2" :
                                                          internal::str(val.number_of_args))))) << " argument(s) not " << c.size())
            HTB_RAISE_IF(val.number_of_args == AT_LEAST_1_ARGS && long(c.size()) < 1, "'" << name << "' needs at least 1 argument not " << c.size())
            HTB_RAISE_IF(val.number_of_args == AT_LEAST_2_ARGS && long(c.size()) < 2, "'" << name << "' needs at least 2 arguments not " << c.size())
            HTB_RAISE_IF(val.number_of_args == BETWEEN_0_1_ARGS && long(c.size()) > 1, "'" << name << "' needs 0 to 1 argument, not " << c.size())
            HTB_RAISE_IF(val.number_of_args == BETWEEN_0_2_ARGS && long(c.size()) > 2, "'" << name << "' needs 0 to 2 arguments, not " << c.size())
            // finally exec the procedure
            return val.proc(c);
        }

        cell get_in(const std::string& key)
        {
            HTB_RAISE_IF(type != Dict, "Can not access a sub element because the object is not a dict")
            HTB_RAISE_IF(val.dict.empty(), "Can not access an element with the key " << key << " because the dict is empty")
            HTB_RAISE_IF(val.dict.find(key) == val.dict.end(), "Can not find the key " << key << " in the dict")

            return val.dict[key];
        }

        cell get_in(long n)
        {
            HTB_RAISE_IF(type != List, "Can not access a sub element because the object is not a list")
            HTB_RAISE_IF(n >= long(val.lst.size()), "Can not find the " << n << "th element in the list")

            return val.lst[n];
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
            if (args.size() > parms.size())
                throw std::runtime_error("Too much arguments, got " + internal::str(args.size()) + " expected " + internal::str(parms.size()));
            else if (args.size() < parms.size())
                throw std::runtime_error("Too few arguments, got " + internal::str(args.size()) + " expected " + internal::str(parms.size()));

            cellit a = args.begin();
            for (cellit p = parms.begin(); p != parms.end(); ++p)
            {
                env_[p->val.get<std::string>()] = *a++;
            }
        }

        // return a reference to the innermost environment where 'var' appears
        cell_dict& find(const std::string& var)
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
        cell_dict env_; // inner symbol->cell mapping
        cell_dict errors;
        std::map<std::string, environment*> namespaces;
    };  // struct environment

} // namespace htb
