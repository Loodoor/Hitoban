/*
* Code by Folaefolc
* A Lisp-like done just to concurrence Lisp itself (kind of crazy game for me)
* Interpreted programming language, C++14 ; main purpose is for video games
* License MIT
*/

#include "htb_internal.hpp"
#include "htb_cell.hpp"
#include "hitoban.hpp"

namespace htb
{

    extern std::vector<std::string> loaded_files;

    namespace internal
    {

        const std::vector<std::regex> regexs = {
            std::regex("^\"[^\"]*\"")  // strings
            , std::regex("^:")  // dict key symbol
            , std::regex("^[\\(\\)]")  // parenthesis
            , std::regex("^((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?((e|E)((\\+|-)?)[[:digit:]]+)?")  // numbers
            , std::regex("^[#@$':_!?\\-\\w]+")  // words
            , std::regex("^(\\+|-|\\*|/|%|<=|>=|!=|<|>|=|\\^)")  // operators
            , std::regex("^\\s+")  // whitespaces
            , std::regex("^;.*")  // comments
        };

        std::vector<std::string> split_string(const std::string& str, const std::string& delimiter)
        {
            std::vector<std::string> strings;

            std::string::size_type pos = 0;
            std::string::size_type prev = 0;
            while ((pos = str.find(delimiter, prev)) != std::string::npos)
            {
                strings.push_back(str.substr(prev, pos - prev));
                prev = pos + 1;
            }

            // to get the last substring (or only, if delimiter is not found)
            strings.push_back(str.substr(prev));

            return strings;
        }

        bool isdig(char c)
        {
            return isdigit(static_cast<unsigned char>(c)) != 0;
        }

        std::string convert_htbtype(cell_type type)
        {
            std::string out = "";

            if (type == Symbol)
                out = "Symbol";
            else if (type == Number)
                out = "Number";
            else if (type == String)
                out = "String";
            else if (type == List)
                out = "List";
            else if (type == Dict)
                out = "Dict";
            else if (type == Proc)
                out = "Proc";
            else if (type == Lambda)
                out = "Lambda";
            else if (type == Exception)
                out = "Exception";

            return out;
        }

        bool contains_only(const std::string& s, char t)
        {
            bool ok = true;
            for (auto& c: s)
            {
                if (c != t)
                {
                    ok = false;
                    break;
                }
            }
            return ok;
        }

        bool check_if_file_exists(const std::string& filename)
        {
            std::ifstream f(filename.c_str());
            return f.good();
        }

        std::string read_file(const std::string& filename)
        {
            std::ifstream file(filename.c_str());
            std::stringstream ss;

            ss << file.rdbuf();

            return ss.str();
        }

        std::string get_filename(const std::string& path)
        {
            std::size_t found = path.find_last_of("/\\");
            return path.substr(found + 1);
        }

        std::string get_fullpath(const std::string& name, environment* base)
        {
            std::string o = base->get_parent_file();

            // get base directory of both file
            std::size_t pos = o.find_last_of("/\\");
            std::string bo = (pos != std::string::npos) ? o.substr(0, pos) : o;
            // ---
            std::size_t pos2 = name.find_last_of("/\\");
            std::string bn = (pos2 != std::string::npos) ? name.substr(0, pos2) : name;

            if (o == "")  // we are in the root directory
                o = name;
            else if (name.length() > 3 && name.substr(0, 3) == "lib")  // for the lib, we must extend the name differently
                o = name;
            else if (bo == bn)  // files are in the same directory
                o = name;
            else  // we must crop the filename from the output given by base->get_parent_file()
                o = bo + "/" + name;

            return o;
        }

        cell read_htb_file(cell name, environment* baseenv, environment* ns)
        {
            HTB_RAISE_IF(name.type != String, "'require' needs strings, not " << convert_htbtype(name.type))
            std::string content = load_htb_file(name.val.get<std::string>(), baseenv);
            HTB_RAISE_IF(content == HTB_FILE_NOT_FOUND, "Can not find the required file '" << name.val.get<std::string>() << "'")

            std::string _filename = to_string(name, true);
            std::string _fullname = get_fullpath(name.val.get<std::string>(), baseenv);

            loaded_files.push_back(normalize_path(_fullname));

            if (ns == 0)
            {
                baseenv->isfile = true;
                baseenv->fname = _fullname;
            }
            else
            {
                ns->isfile = true;
                ns->fname = _fullname;
            }

            if (ns == 0)
            {
                // if lib in filename, do not add sub env, the lib is creating its own namespaces
                if (_fullname.length() > 3 && _fullname.substr(0, 3) == "lib")
                {
                    HTB_HANDLE_EXCEPTION(run_string(content, baseenv))
                }
                else
                {
                    environment* sub = baseenv->get_namespace(get_filename(_filename));
                    // use sub to run the file because it isn't a file from the lib
                    HTB_HANDLE_EXCEPTION(run_string(content, sub))
                }
            }
            else
            {
                HTB_HANDLE_EXCEPTION(run_string(content, ns))
            }

            return nil;
        }

        std::string load_htb_file(const std::string& name, environment* baseenv)
        {
            std::string content = "";
            // extend path regarding to the environment
            std::string fullname = get_fullpath(name, baseenv);
            // check in user code or in the lib
            bool exists = check_if_file_exists(fullname) | check_if_file_exists(name);
            if (exists)
            {
                if (name.length() > 3 && name.substr(0, 3) == "lib")
                    content = read_file(name);
                else
                    content = read_file(fullname);
            }
            else
                content = HTB_FILE_NOT_FOUND;

            return content;
        }

        ///////////////////////////////////////////////////// parse, read and user interaction

        void raise_tokenizing_error(const std::string& str, const std::string& s)
        {
            std::cout << "Could not tokenize correctly: " << std::endl;
            std::vector<std::string> subs = internal::split_string(str, "\n");
            // take off the newline char in s
            std::string s2;
            s2.reserve(s.size());
            for (unsigned int j=0; j < s.size(); j++)
            {
                if (s[j] != '\n' && s[j] != '\r')
                    s2 += s[j];
            }

            // print each line while it does not contain the error
            // then print a well placed "^" to indicate what could not be tokenized
            for (auto& element: subs)
            {
                std::cout << element << std::endl;

                std::string::size_type p = element.find(s2);
                if (p != std::string::npos)
                {
                    // we found the line where the error is at
                    for (std::string::size_type i=0; i < p; i++)
                        std::cout << " ";
                    std::cout << "^" << std::endl;

                    break;
                }
            }

            throw std::runtime_error("Tokenizing failed");
        }

        void check_integrity_of(std::list<std::string>& tokens)
        {
            long lparen_count = 0, rparen_count = 0;
            for (std::list<std::string>::iterator it=tokens.begin(); it != tokens.end(); ++it)
            {
                if ((*it) == "(")
                    ++lparen_count;
                if ((*it) == ")")
                    ++rparen_count;
            }

            if (lparen_count > rparen_count)
            {
                std::string msg = "Missing " + str(lparen_count - rparen_count) + " closing brace" + (lparen_count - rparen_count == 1 ? "" : "s");
                throw std::runtime_error(msg);
            }
            else if (rparen_count > lparen_count)
            {
                std::string msg = "Missing " + str(rparen_count - lparen_count) + " opening brace" + (rparen_count - lparen_count == 1 ? "" : "s");
                throw std::runtime_error(msg);
            }
        }
/*
        // reading a list of macros and putting them into the code
        void tokenize_macros_and_insert(std::vector<std::string> macros, std::string& s)
        {
            std::vector<std::vector<std::string>> tokenized_macros;
            std::vector<std::string> v_cur;
            // splitting macros into sub lists
            for (std::size_t i=0; i < macros.size(); ++i)
            {
                if (macros[i] == "#def")
                {
                    if (i != 0)
                        tokenized_macros.push_back(v_cur);
                    v_cur.clear();
                }
                else
                    v_cur.push_back(macros[i]);
            }

            // iterating through the available macros and doing stuff
            for (std::size_t i=0; i < tokenized_macros.size(); ++i)
            {
                // NAME [ARGS...] : CODE
                if (tokenized_macros[i].size() > 2)
                {
                    std::string name = pop(&tokenized_macros[i], 0);
                    // searching for the ':'
                    std::size_t delim_index;
                    for (delim_index=0; tokenized_macros[i][delim_index] != ":" && delim_index < tokenized_macros[i].size(); ++delim_index);
                    if (delim_index == tokenized_macros[i].size() - 1 && tokenized_macros[i][delim_index] != ":")
                        std::cout << "Bad macro definition for `" << name << "`" << std::endl;
                    else
                    {
                        // we need to search for all the occurrences of name [args...] and replace it with
                        // CODE.replace(ARGS..., args...)
                        std::string _s = "";
                        for (std::size_t j=0; j < delim_index; ++j)
                            _s += ".+ ";
                        // creating the search macro
                        std::regex find_and_replace_macro(name + _s);
                        // replacing occurrences in the code
                        std::smatch m;
                        while (std::regex_search(s, m, find_and_replace_macro))
                        {
                            std::string m0(m[0]);
                            std::vector<std::string> pieces = split_string(m0, " ");
                            pop(&pieces, 0); // throw the name, we just need the arguments
                            if (pieces.size() == delim_index)
                            {
                                // replacing arguments name in the macro code by their values
                                std::string _code = "";
                                for (std::size_t k=delim_index + 1; k < tokenized_macros[i].size(); ++k)
                                    _code += tokenized_macros[i][k];
                                for (std::size_t k=0; k < tokenized_macros[i].size() - delim_index; ++k)
                                {
                                    // we are putting the arguments given to the macro into a temp copy of the macro code
                                    std::smatch m;
                                    std::regex reg(tokenized_macros[i][k]);
                                    if (std::regex_search(_code, m, reg))
                                        _code = std::regex_replace(_code, reg, pieces[k]);
                                    else
                                        std::cout << "Bad macro conception. Can not find your mom in it" << std::endl;
                                }
                                s = std::regex_replace(s, find_and_replace_macro, _code, std::regex_constants::format_first_only);
                            }
                            else
                                std::cout << "Bad macro call, this one (" << name << ") seems to need " << delim_index << " arguments, not " << pieces.size() << std::endl;
                        }
                    }
                }
                else
                    std::cout << "Bad macro definition, expecting at least 3 elements : `NAME [ARGS...] : CODE`" << std::endl;
            }
        }
*/

        // convert given string to list of tokens
        std::list<std::string> tokenize(const std::string& str)
        {
            std::string s = str;
            std::list<std::string> tokens;

/*
            // getting macros
std::cout << "hi" << std::endl;
            std::regex reg_macro("\\#def");
            std::smatch m;
            std::vector<std::string> macros;
std::cout << "hello" << std::endl;
            while (std::regex_match(s, m, reg_macro))
            {
                macros.push_back(std::string("#def"));
std::cout << "friend" << std::endl;
                s = std::regex_replace(s, reg_macro, std::string(""), std::regex_constants::format_first_only);
                bool ok = false;

                for (const auto& r : regexs)
                {
                    std::smatch m;
                    bool found = std::regex_search(s, m, r);
                    if (found)
                    {
                        std::string m0(m[0]);
std::cout << m0 << " ";
                        // stripping blanks characters between instructions, and comments
                        if (std::string::npos != m0.find_first_not_of(" \t\v\r\n") && m0.substr(0, 1) != ";")
                            macros.push_back(m[0]);
                        s = std::regex_replace(s, r, std::string(""), std::regex_constants::format_first_only);
                        ok = true;
                        break;
                    }
                }
                if (!ok)
                {
                    raise_tokenizing_error(str, s);
                    break;
                }
            }
            tokenize_macros_and_insert(macros, s);
*/

            // getting all the code and tokenizing it
            while (!s.empty())
            {
                bool ok = false;

                for (const auto& r : regexs)
                {
                    std::smatch m;
                    bool found = std::regex_search(s, m, r);
                    if (found)
                    {
                        std::string m0(m[0]);
                        // stripping blanks characters between instructions, and comments
                        if (std::string::npos != m0.find_first_not_of(" \t\v\r\n") && m0.substr(0, 1) != ";")
                            tokens.push_back(m[0]);
                        s = std::regex_replace(s, r, std::string(""), std::regex_constants::format_first_only);
                        ok = true;
                        break;
                    }
                }
                if (!ok)
                {
                    raise_tokenizing_error(str, s);
                    break;
                }
            }
            check_integrity_of(tokens);

            return tokens;
        }

        // numbers become Numbers; every other token is a Symbol
        cell atom(const std::string& token)
        {
            if (isdig(token[0]) || (token[0] == '-' && isdig(token[1])))
                return cell(Number, token);
            if (token.substr(0, 1) == "\"" || token.substr(0, 1) == "'")
                return cell(String, token.substr(1, token.size() - 2));
            return cell(Symbol, token);
        }

        // return the Hitoban expression in the given tokens
        cell read_from(std::list<std::string>& tokens)
        {
            const std::string token(tokens.front());
            tokens.pop_front();
            if (token == "(")
            {
                cell c(List);
                while (tokens.front() != ")")
                    c.list.push_back(read_from(tokens));
                tokens.pop_front();

                return c;
            }
            else
                return atom(token);
        }

        // return the Hitoban expression represented by the given string
        cell read(const std::string& s)
        {
            std::list<std::string> tokens(tokenize(s));
            return read_from(tokens);
        }

        std::string normalize_path(const std::string& path)
        {
            std::string o = path;
            std::regex pattern("\\.\\./");
            o = std::regex_replace(o, pattern, "");
            return o;
        }

    }  // namespace internal

    // convert given cell to a Hitoban-readable string
    std::string to_string(const cell& exp, bool from_htb)
    {
        if (exp.type == List)
        {
            std::string s("(");
            for (cell::iter e = exp.val.get<cells>().begin(); e != exp.val.get<cells>().end(); ++e)
            {
                if ((*e) != exp)
                    s += to_string(*e) + ' ';
                else
                    s += "... ";
            }
            if (s[s.size() - 1] == ' ')
                s.erase(s.size() - 1);
            return s + ')';
        }
        else if (exp.type == Lambda)
            return "<Lambda>";
        else if (exp.type == Proc)
            return "<Proc>";
        else if (exp.type == Exception)
            return "<Exception> " + exp.val.get<std::string>();
        else if (exp.type == Dict)
        {
            if (exp.val.get<map>().empty())
                return "<Dict>";

            std::string s("(");
            for (auto& kv: exp.val.get<map>())
            {
                s += ':' + kv.first + ' ';
                s += to_string(kv.second) + ' ';
            }
            if (s[s.size() - 1] == ' ')
                s.erase(s.size() - 1);
            return s + ')';
        }
        else if (exp.type == String && !from_htb)
            return "\"" + exp.val.get<std::string>() + "\"";
        else if (exp.type == String && from_htb)
            return exp.val.get<std::string>();
        return "object @ " + internal::str(reinterpret_cast<std::size_t>(&exp.val));
    }

}  // namespace htb
