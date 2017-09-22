/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#include "hitoban.hpp"

namespace htb
{

    ///////////////////////////////////////////////////// constants

    static bool strict_mode = false;
    static bool tracking_mode = false;

    // define functions to create and modify an Hitoban environment
    void add_globals(environment& env)
    {
        env["nil"] = nil;
        env["false"] = false_sym;
        env["true"] = true_sym;

        for(const auto& v : get_builtin())
            env[v.first] = v.second;
    }

    environment init_environment()
    {
        environment env;
        add_globals(env);

        return env;
    }

    ///////////////////////////////////////////////////// main

    cell eval(cell x, environment* env)
    {
        if (tracking_mode)
        {
            std::string nx = to_string(x);
            std::cout << log(termcolor::cyan, "x: " << nx) << " "
                                << log(termcolor::yellow, "[" << ((!env->has_outer()) ? "global": "ref on global")) << ", "
                                    << log(termcolor::green, ((env->isfile) ? (std::string("is a file `") + env->fname +"`") : "not a file"))
                                << log(termcolor::yellow, "]")
                            << std::endl;
        }

        // quitting if we got an exception
        if (x.type == Exception && strict_mode)
            throw std::runtime_error(std::string("Encountered an exception will in strict mode\n") + to_string(x));

        // handling the basics use cases
        if (x.type == Symbol)
            return env->find(x.val)[x.val];
        if (x.type == Number || x.type == String)
            return x;
        if (x.list.empty())
            return nil;

        // kind of macros
        if (x.list[0].val == ":")  // dict key symbol
        {
            HTB_RAISE_IF(x.list.size() < 1, "':' symbolize the beginning of a dict key, the length should be of 2, not of " << x.list.size())

            cell exps;
            exps.type = List;

            x.list[1].type = String;
            exps.list.push_back(x.list[1]);
            for (cell::iter exp = x.list.begin() + 2; exp != x.list.end(); ++exp)
                exps.list.push_back(eval(*exp, env));
            return exps;
        }

        // language keywords interactions definitions
        if (x.list[0].type == Symbol)
        {
            if (x.list[0].val.substr(0, 1) == "#")  // (#key hashmap)
            {
                std::string key = x.list[0].val.substr(1);
                HTB_RAISE_IF(key.empty(), x.list[0].val << " is not a valid key (length must be > 0)")
                cell c(eval(x.list[1], env));
                if (c.type == Dict)
                    return c.get_in(key);
                else if (c.type == List)
                    return c.get_in(internal::str_to<long>(key));
                else if (c.type == String)
                {
                    long n = internal::str_to<long>(key);
                    HTB_RAISE_IF(n >= long(c.val.size()), "'#' can not get a character at pos " << n << " because it is outside the string")
                    return cell(String, std::string(1, c.val[n]));
                }
                HTB_RAISE("The object should be of type dict, list or string to use the # pattern, not of type " << internal::convert_htbtype(c.type))
            }
            if (x.list[0].val == "quote")  // (quote exp)
                return x.list[1];
            if (x.list[0].val == "if")  // (if test conseq [alt])
                return eval(eval(x.list[1], env).val == false_sym.val ? (x.list.size() < 4 ? nil : x.list[3]) : x.list[2], env);
            if (x.list[0].val == "set!")  // (set! var exp)
            {
                cell c = env->find(x.list[1].val)[x.list[1].val];
                HTB_RAISE_IF(c.const_expr, x.list[1].val << " is a const expr, can not set its value to something else")
                return env->find(x.list[1].val)[x.list[1].val] = eval(x.list[2], env);
            }
            if (x.list[0].val == "def")  // (def var exp)
                return (*env)[x.list[1].val] = eval(x.list[2], env);
            if (x.list[0].val == "const")  // (const var exp)
            {
                cell c = eval(x.list[2], env);
                c.const_expr = true;
                return (*env)[x.list[1].val] = c;
            }
            if (x.list[0].val == "lambda")  // (lambda (var*) exp)
            {
                x.type = Lambda;
                // keep a reference to the environment that exists now (when the
                // lambda is being defined) because that's the outer environment
                // we'll need to use when the lambda is executed
                x.env = env;
                return x;
            }
            if (x.list[0].val == "begin")  // (begin exp*)
            {
                for (unsigned int i = 1; i < x.list.size() - 1; ++i)
                {
                    cell c = eval(x.list[i], env);
                    HTB_HANDLE_EXCEPTION(c)
                }
                return eval(x.list[x.list.size() - 1], env);
            }
            if (x.list[0].val == "ns")  // (ns "name" ...)
            {
                HTB_RAISE_IF(x.list.size() < 2, "'ns' needs at least one argument 'name' (string)")
                HTB_HANDLE_EXCEPTION(x.list[1])
                environment* sub = env->get_namespace(x.list[1].val);

                if (x.list.size() > 2)
                    for (unsigned int i = 2; i < x.list.size(); ++i)
                    {
                        cell c = eval(x.list[i], sub);
                        HTB_HANDLE_EXCEPTION(c)
                    }

                return nil;
            }
            if (x.list[0].val == "require")  // (require exp); exp as a list, a dict or a string
            {
                HTB_RAISE_IF(x.list.size() != 2, "require' takes only 1 argument, not " << x.list.size())
                cell c = eval(x.list[1], env);

                if (c.type == List)
                {
                    for (cellit i = c.list.begin(); i != c.list.end(); i++)
                    {
                        HTB_HANDLE_EXCEPTION(internal::read_htb_file((*i), env))
                    }
                }
                else if (c.type == String)
                {
                    HTB_HANDLE_EXCEPTION(internal::read_htb_file(c, env))
                }
                else if (c.type == Dict)
                {
                    for (auto kv: c.dict)
                    {
                        environment* sub = env->get_namespace(kv.first);
                        HTB_HANDLE_EXCEPTION(internal::read_htb_file(kv.second, env, sub))
                    }
                }
                else
                    HTB_RAISE("require' takes a dict, a list or a single string as an argument, not a " << internal::convert_htbtype(c.type))
                return nil;
            }

            ///////////////////////////////////////////////////// procedures that need to use an environment (not provided in htb_stdlib because it only takes cells)

            if (x.list[0].val == "list-current-ns")  // (list-current-ns file)
            {
                HTB_RAISE_IF(x.list.size() != 1, "'list-current-ns' takes only no argument")
                cell output(List);
                for (auto name : env->get_namespaces())
                    output.list.push_back(cell(String, name));
                return output;
            }
            if (x.list[0].val == "get-opened-file")  // (get-opened-file)
            {
                HTB_RAISE_IF(x.list.size() != 1, "'get-opened-file' takes no argument")
                std::string fname = env->get_parent_file();
                return cell(String, fname);
            }
            if (x.list[0].val == "isdef")  // (isdef x)
            {
                HTB_RAISE_IF(x.list[1].type != Symbol, "'isdef' needs a variable as an argument, not a " << internal::convert_htbtype(x.list[1].type))
                return (env->find(x.list[1].val)[x.list[1].val].type != Exception) ? true_sym : false_sym;
            }
        }

        // (proc exp*)
        cell proc(eval(x.list[0], env));
        cells exps;
        for (cell::iter exp = x.list.begin() + 1; exp != x.list.end(); ++exp)
        {
            exps.push_back(eval(*exp, env));
            HTB_HANDLE_EXCEPTION(exps[exps.size() - 1])
        }
        if (proc.type == Lambda)
        {
            // Create an environment for the execution of this lambda function
            // where the outer environment is the one that existed* at the time
            // the lambda was defined and the new inner associations are the
            // parameter names with the given arguments.
            // *Although the environment existed at the time the lambda was defined
            // it wasn't necessarily complete - it may have subsequently had
            // more symbols defined in that environment.
            try
            {
                //                  body                                           params              args
                return eval(proc.list[2], new environment(proc.list[1].list, exps, proc.env));
            }
            catch (const std::runtime_error& e)
            {
                HTB_RAISE(e.what())
            }
        }
        else if (proc.type == Proc)
            return proc.exec(exps, x.list[0].val);
        // we have something like
        // (... (thing other ...))
        // and we are trying to parse "thing" alone
        if (x.type == List)
        {
            cell exp;
            for (cell::iter e = x.list.begin(); e != x.list.end(); ++e)
                exp.list.push_back(eval(*e, env));
            return exp;
        }

        HTB_RAISE("Not a function")
    }

    // execute a string of Hitoban code, in a given environment
    cell run_string(const std::string& code, environment* env)
    {
        cell result = eval(internal::read(code), env);
        return result;
    }

    cell create_function(proc_type p, long n)
    {
        return cell(p, n);
    }

} // namespace htb

htb::types::ret mon_test(htb::types::args args)
{
    // checking the size of the arguments
    if (args.size() < 1)
        return htb::cell(htb::Exception, "Need more arguments !");  // that's how we build an exception in Hitoban
    // check if the argument is an exception, if so, end function call
    if (args[0].type == htb::Exception) return args[0];
    // body of the function
    std::cout << "working" << std::endl;
    // returning a valid htb::cell
    return args[0];
}

int start_repl()
{
    htb::print_shell_headers();
    std::cout << "Type \"help\" for more information."
                    << std::endl;

    htb::environment global_env;
    htb::add_globals(global_env);
    // fct name in htb            ;  create the function  (ptr on fct, number of arguments)
    global_env["mon_test"] = htb::create_function(&mon_test, 1);
    htb::repl("> ", &global_env);

    return EXITSUCCESS;
}

#ifndef HTB_TESTS_INCLUDED
// dirty trick to add a main only if we did not compiled with the tests
int main(int argc, char *argv[])
{
    if (argc < 2)  // we just started the program as a normal one, launch the read-eval-print-loop
        start_repl();
    else if (argc >= 2)
    {
        int c = 1;
        std::string input = argv[c];

        if (input == "-h")  // help
        {
            std::cout << "Help message" << std::endl
                            << "Usage: " << argv[0] << " [option] ... [file | tests] [args...]" << std::endl
                            << "Options and arguments:" << std::endl
                            << "-h      : print this help message and exit" << std::endl
                            << "-w      : stop execution if an exception is detected" << std::endl
                            << "-t      : print all the details of the execution" << std::endl
                            << "-v      : print the Hitoban version number and exit" << std::endl
                            << "file    : program read from script file" << std::endl
                            << "args... : arguments passed to program in ARGS" << std::endl
                            ;
            return EXITSUCCESS;
        }
        if (input == "-w")  // strict mode
        {
            htb::strict_mode = true;

            if (argc >= c + 2)
                input = argv[++c];
            else
            {
                start_repl();  // we do not have other arguments, start a repl
                return EXITSUCCESS;
            }
        }
        if (input == "-t")  // tracking mode
        {
            htb::tracking_mode = true;

            if (argc >= c + 2)
                input = argv[++c];
            else
            {
                start_repl();  // we do not have other arguments, start a repl
                return EXITSUCCESS;
            }
        }
        if (input == "-v")  // version
        {
            htb::print_shell_headers();
            return EXITSUCCESS;
        }

        // if we are here, we have a filename passed as an argument
        if (htb::internal::check_if_file_exists(input))
        {
            std::string content = htb::internal::read_file(input);

            // setting up environment
            htb::environment global_env;
            htb::add_globals(global_env);
            // we must save that we are in a file !
            global_env.isfile = true;
            global_env.fname = input;

            // checking for arguments
            if (argc >= c + 2)
            {
                // we have arguments for the program, let's put the in a const variable named ARGS
                std::string args = "(const ARGS (list ";
                for (int i=c + 1; i < argc; i++)
                {
                    std::string cn(argv[i]);
                    args += std::string(cn) + " ";
                }
                args += "))";

                // push back the arguments in the environment by interpreting them
                htb::run_string(args, &global_env);
            }

            // running the code
            std::cout << htb::to_string(htb::run_string(content, &global_env)) << std::endl;
        }
        else
        {
            // we can not find the file, just exit
            std::cout << argv[0] << ": can not open file '" << input << "'" << std::endl;
            return EXITFAILURE;
        }
    }

    return EXITSUCCESS;
}
#endif
