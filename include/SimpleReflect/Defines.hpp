#ifndef __SIMPLE_REFLECT_DEFINES_HEADER__
#define __SIMPLE_REFLECT_DEFINES_HEADER__

#include <string>
#include <type_traits>
#include <source_location>

#ifndef NAMESPACE_BEGIN
#define NAMESPACE_BEGIN(ns) namespace ns {
#endif

#ifndef NAMESPACE_END
#define NAMESPACE_END(ns) }
#endif

#ifndef NS_DETAIL
#define NS_DETAIL detail
#endif

#ifndef NS_REFLECT
#define NS_REFLECT Reflect
#endif

NAMESPACE_BEGIN(NS_REFLECT)

#ifdef _L
#undef _L
#endif

#if defined(UNICODE) || defined(_UNICODE) || defined(USE_WCHAR)
#ifndef USE_WCHAR
#define USE_WCHAR 1
#endif

#define _L_IMPL_R(x) L ## x
#define _L(x) _L_IMPL_R(x)

using String = std::wstring;
using StringView = std::wstring_view;
#else

#define _L_IMPL_R(x) x
#define _L(x) x

using String = std::string;
using StringView = std::string_view;
#endif


template<
	std::size_t N,
#ifdef USE_WCHAR
	typename CharT = wchar_t
#else
	typename CharT = char
#endif
>
struct StaticString
{
	constexpr StaticString() noexcept
		: content{ 0 } {}

	constexpr StaticString(std::basic_string_view<CharT> sv) noexcept
		: content{ 0 }
	{
		auto begin = sv.begin();
		auto size = N <= sv.size() ? N : sv.size();
		std::copy(begin, begin + size, content);
	}

	constexpr operator std::basic_string_view<CharT>() const noexcept
	{
		return std::basic_string_view<CharT>{ content, N };
	}

	constexpr CharT* data() noexcept { return content; }
	constexpr const CharT* data() const noexcept { return content; }

private:
	CharT content[N + 1];

public:
	template<std::size_t Len>
	constexpr StaticString<N + Len, CharT> operator+(const StaticString<Len, CharT>& str) const
	{
		StaticString<N + Len, CharT> ret;
		std::copy(content, content + N, ret.data());
		std::copy(str.data(), str.data() + Len, ret.data() + N + 1);
		return ret;
	}
};

NAMESPACE_BEGIN(NS_DETAIL)

template<typename T>
inline constexpr std::string_view type_name() noexcept;

template<> inline constexpr std::string_view type_name<void>() noexcept
{ return "void"; }

template<typename T>
inline constexpr std::string_view wrapped_type_name() noexcept
{ return std::source_location::current().function_name(); }

inline constexpr std::size_t wrapped_type_name_prefix_length() noexcept
{ return wrapped_type_name<void>().find(type_name<void>()); }

constexpr std::size_t wrapped_type_name_suffix_length() noexcept
{
	return wrapped_type_name<void>().length()
	     - wrapped_type_name_prefix_length()
	     - type_name<void>().length();
}

template<typename T>
inline constexpr std::string_view type_name() noexcept
{
	constexpr auto wrapped_name = wrapped_type_name<T>();
	constexpr auto prefix_length = wrapped_type_name_prefix_length();
	constexpr auto suffix_length = wrapped_type_name_suffix_length();
	constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
	return wrapped_name.substr(prefix_length, type_name_length);
}
NAMESPACE_END(NS_DETAIL)

template<typename T>
inline constexpr auto TypeName = NS_DETAIL::type_name<T>();

NAMESPACE_END(NS_REFLECT)

#endif