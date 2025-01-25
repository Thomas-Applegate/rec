#pragma once
#include "dfa.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <variant>
#include <cstdint>

struct token_data
{
	enum class lex_mode : uint8_t {
		standard, save, ignore, error
	} mode;
	std::variant<std::string, Dfa> regex;
};

typedef std::pair<std::vector<std::string>, std::unordered_map<std::string, token_data>> token_data_map;

token_data_map parse_input(int argc, const char** argv);
