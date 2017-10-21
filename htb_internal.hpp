/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#pragma once

#include "htb_includes.hpp"
#include "htb_types.hpp"

namespace htb
{

    // forward declarations
    struct cell;
    struct environment;

    namespace internal
    {

        // simple tools
        template <typename T>
        std::string str(T v)
        {
            std::ostringstream os;
            os << v;

            return os.str();
        }

        template <typename T>
        T str_to(const std::string& v)
        {
            T t; std::stringstream b(v);
            b >> t;
            return t;
        }

        std::vector<std::string> split_string(const std::string& str, const std::string& delimiter);

        // related to the eval loop
        bool isdig(char c);
        std::string convert_htbtype(cell_type type);

        // related to the tokenizer
        bool contains_only(const std::string& s, char t);

        // about files
        bool check_if_file_exists(const std::string& filename);
        std::string read_file(const std::string& filename);
        std::string get_filename(const std::string& path);
        std::string get_fullpath(const std::string& name, environment* base);

        cell read_htb_file(cell name, environment* baseenv, environment* ns=0);
        std::string load_htb_file(const std::string& name, environment* baseenv);

        void raise_tokenizing_error(const std::string& str, const std::string& s);
        std::list<std::string> tokenize(const std::string& str);
        cell atom(const std::string& token);
        cell read_from(std::list<std::string>& tokens);
        cell read(const std::string& s);
        std::string normalize_path(const std::string& path);

    }  // namespace internal

    std::string to_string(const cell& exp, bool from_htb=false);

} // namespace htb
