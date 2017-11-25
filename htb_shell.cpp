/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#include "hitoban.hpp"

///////////////////////////////////////////////////// shell

namespace htb
{

    // the default read-eval-print-loop
    void repl(const std::string& prompt, environment* env, bool benchmark)
    {
        while (true)
        {
            std::cout << prompt;
            std::string line;
            std::getline(std::cin, line);

            if (line == "help")
            {
                std::cout << "Type \"quit\" to quit the repl" << std::endl
                                << "     \"copyright\" to display the copyrights information about Hitoban" << std::endl
                                << "     \"license\" to display the license used for Hitoban" << std::endl
                                << std::endl;
            }
            else if (line == "quit")
            {
                std::cout << "Bye !" << std::endl;
                break;
            }
            else if (line == "copyright")
            {
                std::cout << "Copyright (c) 2017 Loodoor (idea and code)" << std::endl
                                << "All rights reserved." << std::endl
                                << std::endl;
            }
            else if (line == "license")
            {
                if (internal::check_if_file_exists("LICENSE"))
                    std::cout << internal::read_file("LICENSE") << std::endl;
                else
                    std::cout << "Can not find LICENSE file. Please only download Hitoban from the official repository : https://github.com/Loodoor/Hitoban/releases" << std::endl;
            }
            else
            {
                auto start = std::chrono::steady_clock::now();
                std::cout << to_string(run_string(line, env)) << std::endl;
                if (benchmark)
                {
                    auto final_time = std::chrono::steady_clock::now();
                    double total_time = std::chrono::duration_cast<std::chrono::duration<double>>(final_time - start).count();
                    std::cout << std::endl
                              << "Execution time : " << total_time << "s"
                              << std::endl
                              ;
                }
            }
        }
    }

    void print_shell_headers()
    {
        std::cout << "Hitoban " << VER_FULLVERSION_STRING
                        << " (last build on " << VER_DATE << "/" << VER_MONTH << "/" << VER_YEAR << ")"
                        << " [status " << VER_STATUS << "]"
                        << std::endl;
    }

}  // namespace htb

