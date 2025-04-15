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

static unsigned int lex_number(std::string_view& str)
{
	if(str.empty()) throw Regex_Exception("encountered end of string too early");
	unsigned int number = 0;
	char ch = str.front();
	if(!(ch >= '0' && ch <= '9')) throw Regex_Exception("failed to read number expected digit");
	do
	{
		number *= 10;
		number += static_cast<unsigned int>(ch-'0');
		str.remove_prefix(1);
		if(str.empty()) throw Regex_Exception("encountered end of string too early");
		ch = str.front();
	}while(ch >= '0' && ch <= '9');
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

Nfa::Nfa(std::string_view regex) : m_states()
{
	emplace_new_state();
	size_t fin_state = parse_regex(regex, 0);
	if(!regex.empty())
	{
		throw Regex_Exception("string not empty at end of parse");
	}
	m_states[fin_state].is_accepting = true;
}

size_t Nfa::emplace_new_state()
{
	size_t ret = m_states.size();
	m_states.emplace_back();
	return ret;
}

size_t Nfa::parse_regex(std::string_view& str, size_t in_state)
{
	size_t fin_state = emplace_new_state();
	bool loop = true;
	while(loop) //loop through all chunks seperated by alternation operator
	{
		size_t next_chunk = parse_chunk(str, in_state);
		m_states[next_chunk].epsilon_transitions.emplace(fin_state);
		if(str.empty()) break;
		switch(str.front())
		{
		case ')': loop = false; //intentional fallthrough
		case '|': str.remove_prefix(1); break;
		default: throw Regex_Exception("unexpected charachter at end of chunk, expected '|' or ')'");
		}
	}
	return fin_state;
}

size_t Nfa::parse_chunk(std::string_view& str, size_t in_state)
{
	size_t working_state = in_state;
	while(!str.empty())
	{
		if(str.front() == '|' || str.front() == ')') break;
		std::string_view scpy = str;
		size_t efin_state = parse_element(str, working_state);
		if(!str.empty())
		{
			switch(str.front())
			{
			default: break;
			case '*':
				m_states[working_state].epsilon_transitions.emplace(efin_state);
			case '+': //intentional fallthrough
				m_states[efin_state].epsilon_transitions.emplace(working_state);
				str.remove_prefix(1);
				break;
			case '?':
				m_states[working_state].epsilon_transitions.emplace(efin_state);
				str.remove_prefix(1);
				break;
			case '{':
			{
				str.remove_prefix(1);
				unsigned int min = lex_number(str);
				unsigned int max;
				std::vector<size_t> save_states;
				switch(min)
				{
				case 0: m_states[working_state].epsilon_transitions.emplace(efin_state);
				case 1: break;
				default:
					for(unsigned int i = 1; i < min; i++)
					{
						working_state = efin_state;
						std::string_view scpy2 = scpy;
						efin_state = parse_element(scpy2, working_state);
					}
				}
				if(str.empty()) throw Regex_Exception("encountered end of string too early - expected '+', '-', or '}'");
				switch(str.front())
				{
				default: throw Regex_Exception("encountered unexprected character in '{}' operator");
				case '+':
					m_states[efin_state].epsilon_transitions.emplace(working_state);
				case '}':
					str.remove_prefix(1);
					break;
				case '-':
					str.remove_prefix(1);
					max = lex_number(str);
					if(max <= min) throw Regex_Exception("max is less than or equal to min inside '{}' operator");
					for(unsigned int i = min; i < max; i++)
					{
						working_state = efin_state;
						std::string_view scpy2 = scpy;
						save_states.emplace_back(working_state);
						efin_state = parse_element(scpy2, working_state);
					}
					for(size_t s : save_states)
					{
						m_states[s].epsilon_transitions.emplace(efin_state);
					}
					if(str.empty()) throw Regex_Exception("encountered end of string too early");
					if(str.front() != '}') throw Regex_Exception("expected '}' in '{}' operator");
					str.remove_prefix(1);
				}
			}}
		}
		working_state = efin_state;
	}
	return working_state;
}

size_t Nfa::parse_element(std::string_view& str, size_t in_state)
{
	if(str.empty()) throw Regex_Exception("encountered end of string too early");
	size_t e_state;
	char ch = str.front();
	str.remove_prefix(1);
	switch(ch)
	{
	case '.': //any charachter
		e_state = emplace_new_state();
		m_states[in_state].omega_transitions.emplace(e_state);
		break;
	case '/': //escaped charachter
		if(str.empty()) throw Regex_Exception("encountered end of string too early");
		e_state = emplace_new_state();
		ch = str.front();
		str.remove_prefix(1);
		switch(ch)
		{
		default:
			m_states[in_state].ch_transitions.emplace(ch, e_state);
			break;
		case 'n':
		case 'N':
			m_states[in_state].ch_transitions.emplace('\n', e_state);
			break;
		case 't':
		case 'T':
			m_states[in_state].ch_transitions.emplace('\t', e_state);
			break;
		case 'r':
		case 'R':
			m_states[in_state].ch_transitions.emplace('\r', e_state);
			break;
		case 'v':
		case 'V':
			m_states[in_state].ch_transitions.emplace('\v', e_state);
			break;
		case 'f':
		case 'F':
			m_states[in_state].ch_transitions.emplace('\r', e_state);
			break;
		case 'a':
		case 'A':
			m_states[in_state].ch_transitions.emplace('\a', e_state);
			break;
		case 'b':
		case 'B':
			m_states[in_state].ch_transitions.emplace('\b', e_state);
			break;
		case 'z':
		case 'Z':
			m_states[in_state].ch_transitions.emplace(0, e_state);
			break;
		case 'S':
			m_states[in_state].ch_transitions.emplace('\t', e_state);
			m_states[in_state].ch_transitions.emplace('\v', e_state);
			m_states[in_state].ch_transitions.emplace('\r', e_state);
		case 's':
			m_states[in_state].ch_transitions.emplace(' ', e_state);
			break;
		}
		break;
	default: //a specific charachter
		e_state = emplace_new_state();
		m_states[in_state].ch_transitions.emplace(ch, e_state);
		break;
	case '(': //regex
		e_state = parse_regex(str, in_state);
		break;
	case '[': //range of charachters
		throw Regex_Exception("charachter ranges not yet implemented");
	case '*': throw Regex_Exception("unexpected charachter '*'");
	case '+': throw Regex_Exception("unexpected charachter '+'");
	case '-': throw Regex_Exception("unexpected charachter '-'");
	case '?': throw Regex_Exception("unexpected charachter '?'");
	case ']': throw Regex_Exception("unexpected charachter ']'");
	case '{': throw Regex_Exception("unexpected charachter '{'");
	case '}': throw Regex_Exception("unexpected charachter '}'");
	}
	return e_state;
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
