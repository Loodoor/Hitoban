/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#pragma once

#include "htb_includes.hpp"
#include "htb_cell.hpp"
#include "htb_stdlib.hpp"
#include "htb_internal.hpp"
#include "htb_tests.hpp"
#include "htb_version.hpp"
#include "htb_types.hpp"

namespace htb
{

    void add_globals(environment& env);
    environment init_environment();
    cell eval(cell x, environment* env);
    cell run_string(const std::string& code, environment* env);
    void repl(const std::string& prompt, environment* env);
    void print_shell_headers();

}  // namespace htb
