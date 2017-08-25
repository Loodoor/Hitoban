#include "hitoban.hpp"

///////////////////////////////////////////////////// shell

namespace htb
{

// the default read-eval-print-loop
void repl(const std::string& prompt, environment* env)
{
    for (;;)
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
            if (check_if_file_exists("LICENSE"))
                std::cout << read_file("LICENSE") << std::endl;
            else
                std::cout << "Can not find LICENSE file. Please only download Hitoban from the official repository : https://github.com/Loodoor/Hitoban/releases" << std::endl;
        }
        else
            std::cout << to_string(run_string(line, env)) << std::endl;
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

