#pragma once
#include <sstream>
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

class Regex_Parse
{
public:
	Regex_Parse() noexcept = default;
	Regex_Parse(std::stringstream& sstream);
private:
};
