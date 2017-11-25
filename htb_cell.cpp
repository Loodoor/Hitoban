/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#include "htb_cell.hpp"

namespace htb
{

    cell::value::value() :
        holder("", 0, cells(), cell_dict(), nullptr)
        , number_of_args(0)
    {}

    cell::value::value(const cell::value& v) :
        holder(v.holder)
        , number_of_args(v.number_of_args)
    {}

    void cell::value::init_from(cell::value* v)
    {
        holder = v->holder;
        number_of_args = v->number_of_args;
    }

    bool cel::value::operator==(const cell::value& other)
    {
        return (holder == other.holder) &&
               (number_of_args == other.number_of_args);
    }

    cell::cell(cell_type type=Symbol) :
        type(type)
        , env(0)
        , const_expr(false)
    {}

    cell::cell(proc_type proc, long n=0) :
        type(Proc)
        , env(0)
        , const_expr(false)
    {
        val.set<proc_type>(proc);
        val.number_of_args = n;
    }

    cell::cell(const cell& c) :
        val(c.val)
        , type(c.type)
        , env(c.env)
        , const_expr(c.const_expr)
    {}

    void cell::init_from(cell* c)
    {
        type = c->type;
        val.init_from(&(c->val));
        env = c->env;
        const_expr = c->const_expr;
    }

    cell cell::exec(const cells& c, const std::string& name)
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
        return val.get_ref<proc_type>()(c);
    }

    cell cell::get_in(const std::string& key)
    {
        HTB_RAISE_IF(type != Dict, "Can not access a sub element because the object is not a dict")
        HTB_RAISE_IF(val.get_ref<cell_dict>().empty(), "Can not access an element with the key " << key << " because the dict is empty")
        HTB_RAISE_IF(val.get_ref<cell_dict>().find(key) == val.get_ref<cell_dict>().end(), "Can not find the key " << key << " in the dict")

        return val.get_ref<cell_dict>()[key];
    }

    cell cell::get_in(long n)
    {
        HTB_RAISE_IF(type != List, "Can not access a sub element because the object is not a list")
        HTB_RAISE_IF(n >= long(val.get_ref<cells>().size()), "Can not find the " << n << "th element in the list")

        return val.get_ref<cells>()[n];
    }

    bool cell::operator==(const cell& r) const
    {
        return std::addressof(*this) == std::addressof(r);
    }

    bool cell::operator!=(const cell& r) const
    {
        return !(r == *this);
    }

    environment::environment(environment* outer=0) :
        isfile(false)
        , outer_(outer)
    {}

    environment::environment(const cells& parms, const cells& args, environment* outer) :
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

    cell_dict& environment::find(const std::string& var)
    {
        if (env_.find(var) != env_.end())
            return env_; // the symbol exists in this environment
        if (outer_)
            return outer_->find(var); // attempt to find the symbol in some "outer" env

        std::stringstream ss; ss << "Unbound symbol '" << var << "'";
        errors[var] = cell(Exception, ss.str());

        return errors;
    }

    cell& environment::operator[] (const std::string & var)
    {
        return env_[var];
    }

    bool environment::has_outer()
    {
        return outer_ != 0;
    }

}  // namespace htb
