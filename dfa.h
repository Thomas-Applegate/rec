#pragma once
#include "regex_parse.h"

class Nfa
{
public:
	Nfa() noexcept = default;
	Nfa(const Regex_Parse& regex);
private:	
};

class Dfa
{
public:
	Dfa() noexcept = default;
	Dfa(const Nfa& nfa);
private:
};
