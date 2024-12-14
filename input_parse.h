#pragma once
#include "dfa.h"

#include <unordered_map>
#include <string>

std::unordered_map<std::string, Dfa> parse_input(int argc, const char** argv);
