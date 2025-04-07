#include "dfa.h"

#include <utility>
#include <iterator>
#include <iostream>

Regex_Exception::Regex_Exception(const char* what) noexcept : m_what(what) {}
Regex_Exception::Regex_Exception(const Regex_Exception& oth) noexcept : m_what(oth.m_what) {}
Regex_Exception& Regex_Exception::operator=(const Regex_Exception& oth) noexcept
{
	m_what = oth.m_what;
	return *this;
}
const char* Regex_Exception::what() const noexcept { return m_what; }

static unsigned int lex_number(const char*& str)
{
	unsigned int number = 0;
	char ch = *str++;
	if(!(ch >= '0' && ch <= '9')) throw Regex_Exception("failed to read number expected digit");
	do
	{
		number *= 10;
		number += static_cast<unsigned int>(ch-'0');
		ch = *str++;
	}while(ch >= '0' && ch <= '9');
	if(ch == 0) throw Regex_Exception("encountered end of string too early");
	str--;
	return number;
}

static char lex_hex(const char*& str)
{
	char ret = 0;
	for(int i = 0; i < 2; i++)
	{
		ret *= 16;
		switch(*str++)
		{
		case '0': break;
		case '1': ret += 1; break;
		case '2': ret += 2; break;
		case '3': ret += 3; break;
		case '4': ret += 4; break;
		case '5': ret += 5; break;
		case '6': ret += 6; break;
		case '7': ret += 7; break;
		case '8': ret += 8; break;
		case '9': ret += 9; break;
		case 'a':
		case 'A': ret += 10; break;
		case 'b':
		case 'B': ret += 11; break;
		case 'c':
		case 'C': ret += 12; break;
		case 'd':
		case 'D': ret += 13; break;
		case 'e':
		case 'E': ret += 14; break;
		case 'f':
		case 'F': ret += 15; break;
		case 0: throw Regex_Exception("encountered end of string too early");
		}
	}
	return ret;
}

/* regex cfg
S  -> G S' $
S' -> pipe S | eps
G  -> U O G | eps
U  -> ch | . | ( S ) | A
O -> * | + | ? | { I N } | eps
A  -> [ ch - ch A' ]
A' -> ch - ch A' | eps
I -> digit I'
I' -> digit I' | eps
N -> - I | + | eps
*/

Nfa::Nfa(std::string_view regex) : m_states() {}

size_t Nfa::emplace_new_state()
{
	size_t ret = m_states.size();
	m_states.emplace_back();
	return ret;
}

Nfa::operator const std::vector<Nfa::state>&() const noexcept { return m_states; }
const std::vector<Nfa::state>& Nfa::states() const noexcept { return m_states; }

Dfa::Dfa(const Nfa& nfa) {}

Dfa::operator const std::vector<Dfa::state>&() const noexcept { return m_states; }
const std::vector<Dfa::state>& Dfa::states() const noexcept { return m_states; }

//formated output for Nfa and Dfa

template<typename T, typename F>
static void format_delim(const T& container, F&& func, std::ostream& os, const char* delim = ", ")
{
	if(container.empty()) return;
	auto last = std::prev(container.cend());
	for(auto it = container.cbegin(); it != last; it++)
	{
		func(os, *it);
		os << delim;
	}
	func(os, *last);
}

template<typename T>
static void format_delim(const T& container, std::ostream& os, const char* delim = ", ")
{
	format_delim(container, [](std::ostream& os, const auto& e){os << e;}, os, delim);
}

std::ostream& operator<<(std::ostream& os, const Nfa::state& s)
{
	os << "ε→{";
	format_delim(s.epsilon_transitions, os);
	os << "} Ω→{";
	format_delim(s.omega_transitions, os);
	int last_ch = 256;
	for(const auto&[ch, i] : s.ch_transitions)
	{
		if(ch != last_ch)
		{
			os << "} " << ch << "->{" << i;
		}else
		{
			os << ", " << i;
		}
		last_ch = ch;
	}
	os << '}';
	if(s.is_accepting) os << " Accepting";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Dfa::state& s)
{
	if(const size_t* omegat = std::get_if<size_t>(&s.transitions))
	{
		os << "Ω→" << *omegat;
	}else
	{
		const std::map<char, size_t>* t = std::get_if<std::map<char, size_t>>(&s.transitions);
		if(!t->empty())
			format_delim(*t, [](std::ostream& os, const auto& p){os << p.first << "→" << p.second;}, os);
	}
	if(s.is_accepting) os << " Accepting";
	return os;
}
