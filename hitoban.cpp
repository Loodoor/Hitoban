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
    std::vector<std::string> loaded_files;

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
            std::cout << log(termcolor::cyan, "x (" << internal::convert_htbtype(x.type) << ") : " << nx) << " "
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
            return env->find(x.val.get_ref<std::string>())[x.val.get_ref<std::string>()];
        if (x.type == Number || x.type == String)
            return x;
        if (x.val.get_ref<cells>().empty())
            return nil;

        // kind of macros
        if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == ":")  // dict key symbol
        {
            HTB_RAISE_IF(x.val.get_ref<cells>().size() < 1, "':' symbolize the beginning of a dict key, the length should be of 2, not of " << x.val.get_ref<cells>().size())

            cell exps;
            exps.type = List;

            x.val.get_ref<cells>()[1].type = String;
            exps.val.get_ref<cells>().push_back(x.val.get<cells>()[1]);
            for (cell::iter exp = x.val.get_ref<cells>().begin() + 2; exp != x.val.get_ref<cells>().end(); ++exp)
                exps.val.get_ref<cells>().push_back(eval(*exp, env));
            return exps;
        }

        /// language keywords interactions definitions
        if (x.val.get_ref<cells>()[0].type == Symbol)
        {
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>().substr(0, 1) == "#")  // (#key hashmap)
            {
                std::string key = x.val.get_ref<cells>()[0].val.get_ref<std::string>().substr(1);
                HTB_RAISE_IF(key.empty(), to_string(x.val.get_ref<cells>()[0]) << " is not a valid key (length must be > 0)")
                cell c(eval(x.val.get_ref<cells>()[1], env));
                if (c.type == Dict)
                    return c.get_in(key);
                else if (c.type == List)
                    return c.get_in(internal::str_to<long>(key));
                else if (c.type == String)
                {
                    long n = internal::str_to<long>(key);
                    HTB_RAISE_IF(n >= long(c.val.get_ref<std::string>().size()), "'#' can not get a character at pos " << n << " because it is outside the string")
                    return cell(String, std::string(1, c.val.get_ref<std::string>()[n]));
                }
                HTB_RAISE("The object should be of type dict, list or string to use the # pattern, not of type " << internal::convert_htbtype(c.type))
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "quote")  // (quote exp)
                return x.val.get_ref<cells>()[1];
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "if")  // (if test conseq [alt])
                return eval(eval(x.val.get_ref<cells>()[1], env).val == false_sym.val ? (x.val.get_ref<cells>().size() < 4 ? nil : x.val.get_ref<cells>()[3]) : x.val.get_ref<cells>()[2], env);
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "set!")  // (set! var exp)
            {
                cell c = env->find(x.val.get_ref<cells>()[1].val.get_ref<std::string>())[x.val.get_ref<cells>()[1].val.get_ref<std::string>()];
                HTB_RAISE_IF(c.const_expr, x.val.get_ref<cells>()[1].val.get_ref<std::string>() << " is a const expr, can not set its value to something else")
                return env->find(x.val.get_ref<cells>()[1].val.get_ref<std::string>())[x.val.get_ref<cells>()[1].val.get_ref<std::string>()] = eval(x.val.get_ref<cells>()[2], env);
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "def")  // (def var exp)
                return (*env)[x.val.get_ref<cells>()[1].val.get_ref<std::string>()] = eval(x.val.get_ref<cells>()[2], env);
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "const")  // (const var exp)
            {
                cell c = eval(x.val.get_ref<cells>()[2], env);
                c.const_expr = true;
                return (*env)[x.val.get_ref<cells>()[1].val.get_ref<std::string>()] = c;
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "lambda")  // (lambda (var*) exp)
            {
                x.type = Lambda;
                // keep a reference to the environment that exists now (when the
                // lambda is being defined) because that's the outer environment
                // we'll need to use when the lambda is executed
                x.env = env;
                return x;
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "begin")  // (begin exp*)
            {
                for (unsigned int i = 1; i < x.val.get_ref<cells>().size() - 1; ++i)
                {
                    cell c = eval(x.val.get_ref<cells>()[i], env);
                    HTB_HANDLE_EXCEPTION(c)
                }
                return eval(x.val.get_ref<cells>()[x.val.get_ref<cells>().size() - 1], env);
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "ns")  // (ns "name" ...)
            {
                HTB_RAISE_IF(x.val.get_ref<cells>().size() < 2, "'ns' needs at least one argument 'name' (String)")
                HTB_HANDLE_EXCEPTION(x.val.get_ref<cells>()[1])
                HTB_RAISE_IF(x.val.get_ref<cells>()[1].type != String, "'ns' argument one must be of type String, not of type " << internal::convert_htbtype(x.val.get_ref<cells>()[1].type))
                environment* sub = env->get_namespace(x.val.get_ref<cells>()[1].val.get_ref<std::string>());

                if (x.val.get_ref<cells>().size() > 2)
                    for (unsigned int i=2; i < x.val.get_ref<cells>().size(); ++i)
                    {
                        cell c = eval(x.val.get_ref<cells>()[i], sub);
                        HTB_HANDLE_EXCEPTION(c)
                    }

                return nil;
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "require")  // (require exp); exp as a list, a dict or a string
            {
                HTB_RAISE_IF(x.val.get_ref<cells>().size() != 2, "require' takes only 1 argument, not " << x.val.get_ref<cells>().size())
                cell c = eval(x.val.get_ref<cells>()[1], env);

                if (c.type == List)
                {
                    for (cellit i = c.val.get_ref<cells>().begin(); i != c.val.get_ref<cells>().end(); i++)
                    {
                        HTB_RAISE_IF(i->type != String, "'require' needs arguments of type String inside the given list, not of type " << internal::convert_htbtype(i->type))
                        if (std::find(loaded_files.begin(), loaded_files.end(), i->val.get<std::string>()) == loaded_files.end())
                        {
                            HTB_HANDLE_EXCEPTION(internal::read_htb_file((*i), env))
                        }
                    }
                }
                else if (c.type == String)
                {
                    if (std::find(loaded_files.begin(), loaded_files.end(), c.val.get_ref<std::string>()) == loaded_files.end())
                    {
                        HTB_HANDLE_EXCEPTION(internal::read_htb_file(c, env))
                    }
                }
                else if (c.type == Dict)
                {
                    for (auto kv: c.val.get_ref<cell_dict>())
                    {
                        HTB_RAISE_IF(kv.second.type != String, "'require' arguments should be of type String inside the given dict, not of type " << internal::convert_htbtype(kv.second.type))
                        environment* sub = env->get_namespace(kv.first);
                        if (std::find(loaded_files.begin(), loaded_files.end(), kv.second.val.get_ref<std::string>()) == loaded_files.end())
                        {
                            HTB_HANDLE_EXCEPTION(internal::read_htb_file(kv.second, env, sub))
                        }
                    }
                }
                else
                    HTB_RAISE("require' takes a dict, a list or a single string as an argument, not a " << internal::convert_htbtype(c.type))
                return nil;
            }

            ///////////////////////////////////////////////////// procedures that need to use an environment (not provided in htb_stdlib because it only takes cells)

            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "list-current-ns")  // (list-current-ns file)
            {
                HTB_RAISE_IF(x.val.get_ref<cells>().size() != 1, "'list-current-ns' takes only no argument")
                cell output(List);
                for (auto name : env->get_namespaces())
                    output.val.get_ref<cells>().push_back(cell(String, name));
                return output;
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "get-opened-file")  // (get-opened-file)
            {
                HTB_RAISE_IF(x.val.get_ref<cells>().size() != 1, "'get-opened-file' takes no argument")
                std::string fname = env->get_parent_file();
                return cell(String, fname);
            }
            if (x.val.get_ref<cells>()[0].val.get_ref<std::string>() == "isdef")  // (isdef x)
            {
                HTB_RAISE_IF(x.val.get_ref<cells>()[1].type != Symbol, "'isdef' needs a variable as an argument, not a " << internal::convert_htbtype(x.val.get_ref<cells>()[1].type))
                return (env->find(x.val.get_ref<cells>()[1].val.get_ref<std::string>())[x.val.get_ref<cells>()[1].val.get_ref<std::string>()].type != Exception) ? true_sym : false_sym;
            }
        }

        // (proc exp*)
        cell proc(eval(x.val.get_ref<cells>()[0], env));
        cells exps;
        for (cell::iter exp = x.val.get_ref<cells>().begin() + 1; exp != x.val.get_ref<cells>().end(); ++exp)
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
                //          body
                return eval(proc.val.get_ref<cells>()[2],
                            new environment(
                                            proc.val.get_ref<cells>()[1].val.get_ref<cells>()  // params
                                            , exps                                             // args
                                            , proc.env));
            }
            catch (const std::runtime_error& e)
            {
                HTB_RAISE(e.what())
            }
        }
        else if (proc.type == Proc)
            return proc.exec(exps, x.val.get_ref<cells>()[0].val.get_ref<std::string>());
        // we have something like
        // (... (thing other ...))
        // and we are trying to parse "thing" alone
        if (x.type == List)
        {
            cell exp;
            for (cell::iter e = x.val.get_ref<cells>().begin(); e != x.val.get_ref<cells>().end(); ++e)
                exp.val.get_ref<cells>().push_back(eval(*e, env));
            return exp;
        }

        HTB_RAISE("Not a function")
    }

    // execute a string of Hitoban code, in a given environment
    cell run_string(const std::string& code, environment* env)
    {
        try
        {
            cell result = eval(internal::read(code), env);
            return result;
        }
        catch (const std::runtime_error& e)
        {
            HTB_RAISE(e.what())
        }
    }

    cell create_function(proc_type p, long n)
    {
        return cell(p, n);
    }

} // namespace htb

/// dirty trick to add a main only if we did not compiled with the tests
#ifndef HTB_TESTS_INCLUDED
htb::return_type mon_test(htb::args_type args)
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

int start_repl(bool benchmarking)
{
    htb::print_shell_headers();
    std::cout << "Type \"help\" for more information."
                    << std::endl;

    htb::environment global_env;
    htb::add_globals(global_env);

    /// fct name in htb        ;  create the function (ptr on fct, number of arguments)
    global_env["mon_test"] = htb::create_function(&mon_test, 1);

    htb::repl("> ", &global_env, benchmarking);

    return EXITSUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc < 2)  // we just started the program as a normal one, launch the read-eval-print-loop
        start_repl();
    else if (argc >= 2)
    {
        int c = 1;
        std::string input = argv[c];
        bool benchmarking = false;

        if (input == "-h")  // help
        {
            std::cout << "Help message" << std::endl
                            << "Usage: " << argv[0] << " [option] ... [file | tests] [args...]" << std::endl
                            << "Options and arguments:" << std::endl
                            << "-h      : print this help message and exit" << std::endl
                            << "-b      : start a benchmark. If no files are specified, " << std::endl
                            << "          start a repl evaluating the time taken by each pieces of code" << std::endl
                            << "-w      : stop execution if an exception is detected" << std::endl
                            << "-t      : print all the details of the execution" << std::endl
                            << "-v      : print the Hitoban version number and exit" << std::endl
                            << "file    : program read from script file" << std::endl
                            << "args... : arguments passed to program in ARGS" << std::endl
                            ;
            return EXITSUCCESS;
        }
        if (input == "-b")  // benchmark
        {
            benchmarking = true;
            input = argv[++c];
        }
        if (input == "-w")  // strict mode
        {
            htb::strict_mode = true;

            if (argc >= c + 2)
                input = argv[++c];
            else
            {
                start_repl(benchmarking);  // we do not have other arguments, start a repl
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
                start_repl(benchmarking);  // we do not have other arguments, start a repl
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
            auto start = std::chrono::steady_clock::now();
            std::string content = htb::internal::read_file(input);
            auto reading_time = std::chrono::steady_clock::now();

            // setting up environment
            htb::environment global_env;
            htb::add_globals(global_env);
            // we must save that we are in a file !
            global_env.isfile = true;
            global_env.fname = input;

            auto env_init_time = std::chrono::steady_clock::now();

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

            auto argc_construction_time = std::chrono::steady_clock::now();

            // running the code
            std::cout << htb::to_string(htb::run_string(content, &global_env)) << std::endl;

            auto final_time = std::chrono::steady_clock::now();

            if (benchmarking)
            {
                double e01 = std::chrono::duration_cast<std::chrono::duration<double>>(reading_time - start).count();
                double e12 = std::chrono::duration_cast<std::chrono::duration<double>>(env_init_time - reading_time).count();
                double e23 = std::chrono::duration_cast<std::chrono::duration<double>>(argc_construction_time - env_init_time).count();
                double e34 = std::chrono::duration_cast<std::chrono::duration<double>>(final_time - argc_construction_time).count();
                double total_time = std::chrono::duration_cast<std::chrono::duration<double>>(final_time - start).count();

                std::cout << "Benchmark"                                 << std::endl
                          << "================================"          << std::endl
                          << "Tokenizing/parsing       : " << e01 << "s" << std::endl
                          << "Initializing environment : " << e12 << "s" << std::endl
                          << "Command line arguments   : " << e23 << "s" << std::endl
                          << "Execution time           : " << e34 << "s" << std::endl
                          << "Total time               : " << total_time << "s" << std::endl
                          ;
            }
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
