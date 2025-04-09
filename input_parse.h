#pragma once
#include "dfa.h"
#include "insert_order_map.h"

#include <string>
#include <variant>

struct token_data
{
	enum class lex_mode 
	{
		standard, save, ignore, error
	} mode;
	std::variant<std::string, Dfa> regex;
};

insert_order_map<std::string, token_data> parse_input(int argc, const char** argv);
