#include "dfa.h"

#include <utility>

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
	while(ch >= '0' && ch <= '9')
	{
		number *= 10;
		number += static_cast<unsigned int>(ch-'0');
		ch = *str++;
	}
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

size_t Nfa::parse_element(const char*& str, size_t in_state)
{
	char ch = *str++;
	size_t out_state;
	if(ch == '(') //regex
	{
		out_state = parse_regex(str, in_state);
		if(*str++ != ')') throw Regex_Exception("expected ')' to close '('");
		return out_state;
	}
	out_state = emplace_new_state();
	switch(ch) //parse single element
	{
	case '.': //any char
		m_states[in_state].omega_transitions.emplace(out_state);
		break;
	case '[': //range
		do
		{
			char min = *str++;
			if(*str++ != '-') throw Regex_Exception("'-' required in character range");
			char max = *str++;
			if(max < min) std::swap(min, max);
			for(ch = min; ch <= max; ch++)
			{
				m_states[in_state].ch_transitions.emplace(ch, out_state);
			}
		}while(*str != ']');
		str++;
		break;
	case '/': //escape sequence (intentional fallthrough)
		switch(ch = *str++)
		{
		case '.':
		case '*':
		case '+':
		case '?':
		case '|':
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
		case '-':
		case '/':
			break;
		case 'n':
			ch = '\n';
			break;
		case 'r':
			ch = '\r';
			break;
		case 't':
			ch = '\t';
			break;
		case 'v':
			ch = '\v';
			break;
		case 'f':
			ch = '\f';
			break;
		case 'b':
			ch = '\b';
			break;
		case 'a':
			ch = '\a';
			break;
		case 's':
			ch = ' ';
			break;
		case 'z':
			ch = 0;
			break;
		case 'x':
			ch = lex_hex(str);
			break;
		default: throw Regex_Exception("invalid escape sequence");
		}
	default: //specific char
		m_states[in_state].ch_transitions.emplace(ch, out_state);
		break;
	//error conditions
	case ']': throw Regex_Exception("unexpected character ']'");
	case ')': throw Regex_Exception("unexpected character ')'");
	case '*': throw Regex_Exception("unexpected character '*'");
	case '+': throw Regex_Exception("unexpected character '+'");
	case '?': throw Regex_Exception("unexpected character '?'");
	case '{': throw Regex_Exception("unexpected character '{'");
	case '}': throw Regex_Exception("unexpected character '}'");
	case '-': throw Regex_Exception("unexpected character '-'");
	}
	return out_state;
}

size_t Nfa::parse_chunk(const char*& str, size_t in_state)
{
	size_t working_state = in_state;
	while(true)
	{
		const char* save_ptr = str;
		if(char ch = *str; ch == '|' || ch == 0) return working_state;
		size_t next_state = parse_element(str, working_state);
		switch(*str++) //parse operator
		{
		case '*': //0 or more of the element
			m_states[working_state].epsilon_transitions.emplace(next_state); //intentional fallthrough
		case '+': //1 or more of the element
			m_states[next_state].epsilon_transitions.emplace(working_state);
			break;
		case '?': //0 or 1 of the element
			m_states[working_state].epsilon_transitions.emplace(next_state);
			break;
		case '{':
		{
			unsigned int min = lex_number(str);
			for(unsigned int i = 0; i < min; i++)
			{
				working_state = next_state;
				next_state = parse_element(save_ptr, working_state);
			}
			switch(*str++)
			{
			case '}': break;
			case '+': //min or more of the element
				m_states[next_state].epsilon_transitions.emplace(working_state);
				if(*str++ != '}') throw Regex_Exception("expected } to close {");
				break;
			case '-': //min to max of the element
			{
				unsigned int max = lex_number(str);
				if(max < min) throw Regex_Exception("max is less than min in {} expression");
				if(max == min) break;
				size_t end_state = emplace_new_state();
				for(unsigned int i = 0; i < max; i++)
				{
					working_state = next_state;
					m_states[working_state].epsilon_transitions.emplace(end_state);
					next_state = parse_element(save_ptr, working_state);
				}
				m_states[next_state].epsilon_transitions.emplace(end_state);
				next_state = end_state;
				if(*str++ != '}') throw Regex_Exception("expected } to close {");
				break;
			}
			default: throw Regex_Exception("unexpected character in {} expression");
			}
		}
		default: break;
		}
		working_state = next_state;
	}
}

size_t Nfa::parse_regex(const char*& str, size_t in_state)
{
	size_t out_state = emplace_new_state();
	do
	{
		size_t chunk_out = parse_chunk(str, in_state);
		m_states[chunk_out].epsilon_transitions.emplace(out_state);
	}while(*str++ == '|');
	return out_state;
}

Nfa::Nfa(const char* regex) : m_states(1)
{
	//first state is starting state
	size_t acc_state = parse_regex(regex, 0);
	m_states[acc_state].is_accepting = true;
}

Nfa::Nfa(const std::string& regex) : Nfa(regex.c_str()) {}

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
