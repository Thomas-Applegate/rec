#pragma once

#include <string>
#include <exception>
#include <optional>
#include <map>
#include <set>
#include <vector>

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
		std::set<size_t> omega_transitions;
	};

	Nfa(const std::string& regex);
	
	
	operator const std::vector<state>&() const noexcept;
	const std::vector<state>& states() const noexcept;
private:
	std::vector<state> m_states;
};

class Dfa
{
public:

	struct state
	{
		bool is_accepting;
		std::optional<size_t> omega_transition;
		std::map<char, size_t> ch_transitions;
	};

	Dfa(const Nfa& nfa);
	
	operator const std::vector<state>&() const noexcept;
	const std::vector<state>& states() const noexcept;
private:
	std::vector<state> m_states;
};
