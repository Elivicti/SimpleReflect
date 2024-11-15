#include <iostream>
#include <format>
#include <functional>

template<typename ...Args>
void print(std::format_string<Args...> fmt, Args&& ...args)
{
	std::format_to(std::ostreambuf_iterator<char>(std::cout), fmt, std::forward<Args>(args)...);
}

#ifdef __GNUC__
#include <cxxabi.h>
#endif

template<typename T>
std::string TypeName()
{
	#ifdef __GNUC__
	std::size_t len = 0;
	char* buf = __cxxabiv1::__cxa_demangle(typeid(T).name(), nullptr, &len, nullptr);
	std::string ret{ buf, len };
	std::free(buf);
	#else
	std::string ret{ typeid(T).name() };
	#endif

	return ret;
}

#include "SimpleReflect/Reflect.hpp"

struct A
{
	double A_f;
	std::string A_name;

	// define reflection inside of a class
	REFLECT_DEFINE(A) {
		REFLECT_MEMBER(A_f),
		REFLECT_MEMBER(A_name)
	};
};

struct X
{
	int a;
	double b;
	std::string s;
	A aaa;

	void func()
	{ ::print("this = {}\n", (void*)this); }

public:
	REFLECT_DEFINE(X) {
		REFLECT_MEMBER(a),
		REFLECT_MEMBER(b),
		REFLECT_MEMBER(aaa), // recursive
		REFLECT_MEMBER(s),
		REFLECT_MEMBER(func) // functions can also be reflected
	};
};

struct B
{
	int a;
	int b;

	A aaa;

	REFLECT_DEFINE(B) {
		REFLECT_MEMBER("B_a", a),
		REFLECT_MEMBER("aaa", aaa),
		REFLECT_MEMBER("B_b", b)
	};
};

struct Y
{
	int a;
	double b;
	B bbb;

	void func()
	{ ::print("this = {}\n", (void*)this); }

	void func(int a)
	{ ::print("this = {} with param {}\n", (void*)this, a); }

public:
};

// define reflection outside of a class
REFLECT_DEFINE_GLOBAL(Y) {
	REFLECT_DEFINE() {
		REFLECT_MEMBER("a", a),
		REFLECT_MEMBER("bbb", bbb),
		REFLECT_MEMBER("b", b),
		REFLECT_METHOD<void(void)>("func", &Y::func),	// reflect overloaded functions
		REFLECT_METHOD<void(int)>("func_int", &ThisClass::func) // you can use "ThisClass" to
		                                                        // reference reflected class
		                                                        // in case class name is too long
	};
};

template<typename T, typename CharT = char>
concept formattable = requires (T& v, std::format_context ctx) {
	std::formatter<std::remove_cvref_t<T>, CharT>().format(v, ctx);
};

struct print_member
{
	int indent = 0;

	template<typename Cls, typename Member>
	void operator()(Cls* ptr, const std::string& name, Member& mbr)
	{
		for (int i = 1; i < indent; i++)
			::print("    ");
		if (indent > 0)
			::print("  - ");

		if constexpr (Reflect::is_reflectable<Member>)
		{
			::print("{}: ({})\n", name, TypeName<Member>());
			Reflect::for_each_member(&mbr, print_member{ indent + 1 });
		}
		else if constexpr (std::is_invocable_v<Member, Cls*>)
		{
			::print("{}: {}; ", name, TypeName<Member>());
			std::invoke(mbr, ptr);
		}
		else if constexpr (std::is_invocable_v<Member, Cls*, int>)
		{
			::print("{}: {}; ", name, TypeName<Member>());
			std::invoke(mbr, ptr, 255);
		}
		else if constexpr (std::is_member_function_pointer_v<Member> || !::formattable<Member>)
		{
			::print("{}: {}\n", name, TypeName<Member>());
		}
		else if constexpr (std::is_same_v<Member, std::uint32_t>)
		{
			::print("{}: {:#010x}\n", name, mbr);
		}
		else
			::print("{}: {}\n", name, mbr);
	}
};

int main()
{
	X x{
		.a = 42,
		.b = 3.14,
		.s = "a string",
		.aaa = {
			.A_f = 5.5,
			.A_name = "struct A"
		}
	};

	Y y{
		.a = 12,
		.b = 9999,
		.bbb = {
			.a = 22,
			.b = 33,
			.aaa = {
				.A_f = 12.7,
				.A_name = "struct A in B in Y"
			}
		}
	};

	::print("--- Displaying members of x:\n");
	Reflect::for_each_member(&x, print_member{});

	::print("\n");
	
	::print("--- Displaying members of y:\n");
	Reflect::for_each_member(&y, print_member{});

	::print("------------------\n");
	Reflect::visit_member(&y, "func_int",
		[](decltype(y)* ptr, Reflect::MemberFunctionPointer<Y, void(int)>& func_int) {
			std::invoke(func_int, ptr, 123456);
		}
	);

	return 0;
}