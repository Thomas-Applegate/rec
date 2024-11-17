#pragma once
#include <utility>

template <typename T>
struct named
{
	std::string name;
	T value;
	
	template <typename U, typename... Args>
	named(U&& name, Args&&... args)
		: name(std::forward<U>(name)), value(std::forward<Args>(args)...) {}
	template <typename U>
	named(const named<U>& oth) : name(oth.name), value(oth.value) {}
	template <typename U>
	named(named<U>&& oth) : name(std::move(oth.name)), value(std::move(oth.value)) {}
};
