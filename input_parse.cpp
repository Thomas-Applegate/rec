#include "input_parse.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <string>
#include <cstring>
#include <iterator>

static inline void check_stream_should_close(std::istream& is)
{
	std::ifstream* fp = dynamic_cast<std::ifstream*>(&is);
	if(fp != nullptr) fp->close();
}

static inline void check_sstream_fail(std::stringstream& ss, std::istream& is, unsigned int lineno)
{
	if(ss.fail() || ss.bad())
	{
		std::cerr << "error: stringstream error wile reading input at line " << lineno << '\n';
		check_stream_should_close(is);
		std::exit(1);
	}
}

static std::unordered_map<std::string, std::string> read_stream(std::istream& is)
{
	std::unordered_map<std::string, std::string> ret;
	if(is.eof())
	{
		std::cerr << "error: the input is empty\n";
		check_stream_should_close(is);
		std::exit(1);
	}
	for(unsigned int lineno = 1; !is.eof(); lineno++)
	{
		//iterate over lines of input
		std::string line;
		std::getline(is, line);
		if(line.empty()) continue; //skip empty line
		else if(is.fail())
		{
			std::cerr << "error: failed to read from the input at line " << lineno << '\n';
			check_stream_should_close(is);
			std::exit(1);
		}
		std::stringstream liness(line);
		check_sstream_fail(liness, is, lineno);
		
		//get token name
		std::string name;
		liness >> name; //the first string is the token name
		auto [it, first] = ret.try_emplace(std::move(name));
		std::string& rs = it->second;
		
		if(liness.eof())
		{
			std::cerr << "error: no regex on line " << lineno << " for token '";
			std::cerr << it->first << "'\n";
			check_stream_should_close(is);
			std::exit(1);
		}
		while (!liness.eof()) //read all regex strings on this line
		{
			std::string rstr;
			liness >> rstr;
			check_sstream_fail(liness, is, lineno);
			if(!first)
			{
				rs.push_back('|'); //alternation with previous regex
			}
			first=false;
			rs.append(rstr);
		}
#ifdef DEBUG
		std::cout << "debug: token '" << it->first << "':" << rs << '\n';
#endif
	}
	return ret;
}

static std::unordered_map<std::string, std::string> read_input(int argc, const char** argv)
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

static std::unordered_map<std::string, Nfa> parse_regexes(int argc, const char** argv)
{
	std::unordered_map<std::string, std::string> s_map = read_input(argc, argv);
	std::unordered_map<std::string, Nfa> ret(s_map.bucket_count());
	for(auto it = s_map.begin(); it != s_map.end(); it = s_map.erase(it))
	{
		const auto& [k,v] = *it;
		try
		{
#ifdef DEBUG
			std::cout << "debug: constructing Nfa for token '" << k << "'\n";
#endif
			ret.emplace(k, v);
		}catch(const std::exception& e)
		{
			std::cerr << "Failed to parse regex. Exception thrown for token '" << k;
			std::cerr << "':" << e.what() << '\n';
			std::exit(2);
		}
	}
	return ret;
}

std::unordered_map<std::string, Dfa> parse_input(int argc, const char** argv)
{
	std::unordered_map<std::string, Nfa> nfa_map = parse_regexes(argc, argv);
	auto begin = std::make_move_iterator(nfa_map.begin());
	auto end = std::make_move_iterator(nfa_map.end());
	return std::unordered_map<std::string, Dfa>(begin, end, nfa_map.bucket_count());
}
