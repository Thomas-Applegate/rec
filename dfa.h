#pragma once

#include <string>
#include <exception>
#include <variant>
#include <map>
#include <set>
#include <vector>
#include <ostream>
#include <type_traits>

class Regex_Exception : public std::exception
{
public:
	Regex_Exception(const char* what) noexcept;
	~Regex_Exception() = default;
	Regex_Exception(const Regex_Exception& oth) noexcept;
	Regex_Exception& operator=(const Regex_Exception& oth) noexcept;
	const char* what() const noexcept override;
private:
	const char* m_what;
};

class Nfa
{
public:

	struct state
	{
		bool is_accepting;
		std::multimap<char, size_t> ch_transitions;
		std::set<size_t> epsilon_transitions;
		std::set<size_t> omega_transitions; //any character transition
	};

	Nfa(const std::string& regex);
	Nfa(const char* regex);
	
	operator const std::vector<state>&() const noexcept;
	const std::vector<state>& states() const noexcept;
private:
	std::vector<state> m_states;
	
	//emplaces empty state and returns its index
	size_t emplace_new_state();
	//recursively parses a regex, takes the string and input state
	//returns the index of the final state of the regex
	size_t parse_regex(const char*& str, size_t in_state);
	//recursively parses a chunk of a regex, takes the string and input state
	//returns the index of the final state of the chunk
	size_t parse_chunk(const char*& str, size_t in_state);
	//recursively parses a single element of a regex, takes the string and input state
	//returns the index of the final state of the chunk
	size_t parse_element(const char*& str, size_t in_state);
};

class Dfa
{
public:

	struct state
	{
		bool is_accepting;
		std::variant<size_t, std::map<char, size_t>> transitions;
	};

	Dfa(const Nfa& nfa);
	
	operator const std::vector<state>&() const noexcept;
	const std::vector<state>& states() const noexcept;
private:
	std::vector<state> m_states;
};

std::ostream& operator<<(std::ostream& os, const Nfa::state& state);
std::ostream& operator<<(std::ostream& os, const Dfa::state& state);

template <typename T>
typename std::enable_if_t<std::is_same_v<T, Nfa>||std::is_same_v<T,Dfa>, std::ostream&>
operator<<(std::ostream& os, const T& fsm)
{
	const std::vector<typename T::state>& states = fsm.states();
	for(size_t i = 0; i < states.size()-1; i++) os << i << '\t' << states[i] << '\n';
	return os << states.size()-1 << '\t' << states.back();
}
