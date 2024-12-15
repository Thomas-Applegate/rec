#include "input_parse.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <cstring>

static inline void check_stream_should_close(std::istream& is)
{
	std::ifstream* fp = dynamic_cast<std::ifstream*>(&is);
	if(fp != nullptr) fp->close();
}

static inline void check_sstream_fail(std::stringstream& ss, std::istream& is)
{
	if(ss.fail() || ss.bad())
	{
		std::cerr << "an error occured reading from the input\n";
		check_stream_should_close(is);
		std::exit(1);
	}
}

static std::unordered_map<std::string, std::stringstream> read_stream(std::istream& is)
{
	std::unordered_map<std::string, std::stringstream> ret;
	if(is.eof())
	{
		std::cerr << "the input is empty\n";
		check_stream_should_close(is);
		std::exit(1);
	}
	for(unsigned int lineno = 1; !is.eof(); lineno++)
	{
		//iterate over lines of input
		std::string line;
		std::getline(is, line);
		if(is.fail())
		{
			std::cerr << "an error occured reading from the input\n";
			check_stream_should_close(is);
			std::exit(1);
		}
		std::stringstream liness(line);
		check_sstream_fail(liness, is);
		if(liness.eof())
		{
			std::cerr << "error: empty line " << lineno << '\n';
			check_stream_should_close(is);
			std::exit(1);
		}
		
		//get token name
		std::string name;
		liness >> name; //the first string is the token name
		auto [it, first] = ret.try_emplace(std::move(name), std::stringstream());
		std::stringstream& rss = it->second;
		
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
			check_sstream_fail(liness, is);
			if(!first)
			{
				rss << '|'; //alternation with previous regex
			}
			first=false;
			rss << rstr;
		}
#ifdef DEBUG
		std::cout << "debug: token '" << it->first << "':" << rss.str() << '\n';
#endif
	}
	return ret;
}

static std::unordered_map<std::string, std::stringstream> read_input(int argc, const char** argv)
{
	if(argc >= 2) //input file
	{
#ifdef DEBUG
		std::cout << "debug: opening file: " << argv[1] << '\n';
#endif
		std::ifstream file(argv[1]);
		if (!file.is_open()) {
			std::cerr << "Error opening file: " << argv[1] << '\n';
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
	std::unordered_map<std::string, std::stringstream> ss_map = read_input(argc, argv);
	std::unordered_map<std::string, Nfa> ret(ss_map.bucket_count());
	for(auto it = ss_map.begin(); it != ss_map.end(); it = ss_map.erase(it))
	{
		auto& [k,v] = *it;
		try
		{
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
	return convert_map_to<Dfa>(parse_regexes(argc, argv));
}
