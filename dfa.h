#pragma once

#include <string>
#include <exception>

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
	Nfa(const std::string& regex);
private:	
};

class Dfa
{
public:
	Dfa(const Nfa& nfa);
private:
};
