/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <exception>
#include <stdexcept>

#include <regex>
#include <algorithm>
#include <cctype>

#include <fstream>

#include <cmath>
#include <stdlib.h>
#include <cstdio>

#include <random>
#include <unordered_map>

#include <chrono>

#include "external/termcolor/termcolor.hpp"
#include "external/fmt/format.hpp"

#define EXITSUCCESS 0x0
#define EXITFAILURE 0x1

#define log(color, msg)  color << msg << termcolor::reset

#define HTB_RAISE(msg) {std::stringstream ss; ss << msg; return cell(Exception, ss.str());}
#define HTB_RAISE_IF(cd, msg) if (cd) { HTB_RAISE(msg) }
#define HTB_HANDLE_EXCEPTION(c) { if (c.type == Exception) return c; }

#define HTB_COPY(base, dest) cell dest(base.type); \
        if (dest.type == List)  dest.list = base.list; \
        else if (dest.type == Dict)  dest.dict = base.dict; \
        else if (dest.type == Proc) dest.proc = base.proc; \
        else  dest.val = base.val; \
        dest.env = base.env; \
        dest.const_expr = base.const_expr; \
        dest.number_of_args = base.number_of_args;

#define HTB_FILE_NOT_FOUND std::string("<file not found>")

