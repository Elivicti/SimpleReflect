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
struct Reflect::Enums::ReflectConfig<HttpStatus>
	: Reflect::Enums::ConfigBase<(std::size_t)HttpStatus::NotFound> {};
// Inheritance of Reflect::Enums::Config class is not needed,
// just make sure specialized ReflectConfig class has required static members.

int main()
{
	Reflect::Enums::EntryArray arr{ Reflect::Enums::entries<Colors>() };
	for (const auto& [ name, value ] : arr)
	{
		print("{} {}\n", name, (int)value);
	}

	print("----------------------------\n");
	using Reflect::Enums::to_string;
	print("{}\n", to_string(Colors::Red));
	print("{}\n", to_string(Colors::Green));
	print("{}\n", to_string(Colors::Blue));

	HttpStatus code;
	code = (HttpStatus)404;

	print("{}\n", to_string(code));
	return 0;
}