#include <iostream>
#include <format>


template<typename ...Args>
void print(std::format_string<Args...> fmt, Args&& ...args)
{
	std::format_to(std::ostreambuf_iterator<char>(std::cout), fmt, std::forward<Args>(args)...);
}

#include "SimpleReflect/Enums.hpp"

enum class Colors
{
	Red,
	Green,
	Blue
};

enum class HttpStatus
{
	OK = 200,
	Accept = 202,
	NotFound = 404
};

// By default, enum reflection will only recognize enum value between 0 and 64
// By specializing this template class, you can specify the max value of enums
template<>
struct Reflect::enums::ReflectConfig<HttpStatus>
	: Reflect::enums::Config<(std::size_t)HttpStatus::NotFound> {};
// Inheritance of Reflect::enums::Config class is not needed,
// just make sure specialized ReflectConfig class has required static members.

int main()
{
	using Reflect::enums::to_string;
	std::cout << to_string(Colors::Red)   << '\n';
	std::cout << to_string(Colors::Green) << '\n';
	std::cout << to_string(Colors::Blue)  << '\n';

	HttpStatus code;
	code = (HttpStatus)404;

	std::cout << to_string(code) << '\n';
	return 0;
}