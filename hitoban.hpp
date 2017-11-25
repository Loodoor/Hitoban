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
#include "htb_version.hpp"
#include "htb_types.hpp"

namespace htb
{
    extern std::vector<std::string> loaded_files;

    void add_globals(environment& env);
    environment init_environment();
    cell eval(cell x, environment* env);
    cell run_string(const std::string& code, environment* env);
    void repl(const std::string& prompt, environment* env, bool benchmark=false);
    void print_shell_headers();
    cell create_function(proc_type p, long n=INF_NB_ARGS);

}  // namespace htb
