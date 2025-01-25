#include "input_parse.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <cstring>
#include <cctype>
#include <limits>
#include <cmath>

static inline void check_stream_should_close(std::istream& is)
{
	std::ifstream* fp = dynamic_cast<std::ifstream*>(&is);
	if(fp != nullptr) fp->close();
}

static bool valid_name_ch(char ch)
{
	switch(ch)
	{
	default: return false;
	case '_':
	case '-':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z': 
	case '0':
	case '1': 
	case '2': 
	case '3':
	case '4': 
	case '5': 
	case '6': 
	case '7': 
	case '8': 
	case '9': return true;
	}
}

static token_data_map read_stream(std::istream& is)
{
	if(is.eof())
	{
		std::cerr << "error: the input is empty\n";
		check_stream_should_close(is);
		std::exit(1);
	}
	token_data_map ret;
	
	unsigned int lineno = 1;
	while(!is.eof())
	{
		int ilex_state = 0; //controls which part of each line we are scanning (mode, name, or regex)
		std::string tk_name;
		token_data tk_data;
		std::string& rstr = std::get<std::string>(tk_data.regex);
		//loop though lines of file
		for(int ch = is.get(); ch != '\n' && ch != EOF; ch=is.get())
		{
			//ignore comments
			if(ch == '#')
			{
				is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				lineno++;
				continue;
			}
			if(ilex_state < 0) //remove leading whitespace while preserving ilex_state
			{
				switch(ch)
				{
				case ' ':
				case '\r':
				case '\t':
				case '\f':
				case '\v': break;
				default:
					ilex_state *= -1;
					is.unget();
				}
				continue;
			}
			switch(ilex_state)
			{
			case 0: //token mode
				if(isspace(ch)) break;
				switch(ch)
				{
				default: is.unget(); //no special character treated as standard mode
				case '.': tk_data.mode = token_data::lex_mode::standard; break;
				case '-': tk_data.mode = token_data::lex_mode::ignore; break;
				case '+': tk_data.mode = token_data::lex_mode::save; break;
				case '!': tk_data.mode = token_data::lex_mode::error; break;
				}
				ilex_state = -1;
				break;
			case 1: //name
				if(valid_name_ch(ch))
				{
					tk_name += ch;
					if(isspace(is.peek())) ilex_state = -2;
					break;
				}
				std::cerr << "error: invalid token name on line " << lineno << '\n';
				std::exit(1);
			case 3: //regex step 2
				rstr += '|';
			case 2: //regex step 1
				rstr += ch;
				if(isspace(is.peek())) ilex_state = -3;
				break;
			}
		}
		if(tk_name.empty())
		{
			std::cerr << "error: no token name provided on line " << lineno << '\n';
			std::exit(1);
		}
		if(rstr.empty())
		{
			std::cerr << "error: no regex provided on line " << lineno << '\n';
			std::exit(1);
		}
		//try emplace into map
		auto [iter, did_insert] = ret.second.try_emplace(tk_name, std::move(tk_data));
		if(did_insert)
		{
			ret.first.push_back(std::move(tk_name));
		}else
		{
			std::cerr << "error: duplicate token '" << tk_name << "' on line " << lineno << '\n';
			std::exit(1);
		}
		lineno++;
	}
	return ret;
}

static token_data_map read_input(int argc, const char** argv)
{
	if(argc >= 2) //input file
	{
#ifdef DEBUG
		std::cout << "debug: opening file: " << argv[1] << '\n';
#endif
		std::ifstream file(argv[1]);
		if (!file.is_open()) {
			std::cerr << "error: could not open file: " << argv[1] << '\n';
			if (file.bad()) {
				std::cerr << "badbit is set.\n";
			}
			if (file.fail()) {
				std::cerr << std::strerror(errno) << '\n';
			}
			std::exit(1);
		}
		return read_stream(file);
	}else //stdin
	{
#ifdef DEBUG
		std::cout << "debug: using stdin\n";
#endif
		return read_stream(std::cin);
	}
}

token_data_map parse_input(int argc, const char** argv)
{
	token_data_map token_map = read_input(argc, argv);
	for(auto& [k, v] : token_map.second)
	{
		try
		{
#ifdef DEBUG
			std::cout << "debug: constructing nfa for token '" << k << "'\n";
#endif
			Nfa nfa(std::get<std::string>(v.regex));
#ifdef DEBUG
			std::cout << nfa << '\n';
			std::cout << "debug: constructing dfa for token '" << k << "'\n";
#endif
			v.regex.emplace<Dfa>(nfa);
#ifdef DEBUG
			std::cout << std::get<Dfa>(v.regex) << '\n';
#endif
		}catch(const std::exception& e)
		{
			std::cerr << "error: failed to parse regex: token '" << k;
			std::cerr << "':" << e.what() << '\n';
			std::exit(2);
		}
	}
	return token_map;
}
