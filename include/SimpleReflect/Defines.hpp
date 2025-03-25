#ifndef __SIMPLE_REFLECT_DEFINES_HEADER__
#define __SIMPLE_REFLECT_DEFINES_HEADER__

#include <string>
#include <algorithm>
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

template<std::size_t N, typename CharT = String::value_type>
struct StaticString
{
	using value_type = CharT;

	constexpr StaticString() noexcept
		: content{ 0 } {}

	constexpr StaticString(std::basic_string_view<CharT> sv) noexcept
		: content{ 0 }
	{
		auto begin = sv.begin();
		auto size = N <= sv.size() ? N : sv.size();
		std::ranges::copy(begin, begin + size, content);
	}
	constexpr StaticString(const CharT (&str)[N + 1])
    {
        std::ranges::copy_n(str, N + 1, content);
    }

	constexpr operator std::basic_string_view<CharT>() const noexcept
	{
		return std::basic_string_view<CharT>{ content, N };
	}

	constexpr CharT* data() noexcept { return content; }
	constexpr const CharT* data() const noexcept { return content; }
	consteval std::size_t size() const noexcept { return N; }
	consteval std::size_t length() const noexcept { return N; }

	CharT content[N + 1];

	template<std::size_t Len>
	constexpr StaticString<N + Len, CharT> operator+(const StaticString<Len, CharT>& str) const
	{
		StaticString<N + Len, CharT> ret;
		std::ranges::copy(content, content + N, ret.data());
		std::ranges::copy(str.data(), str.data() + Len, ret.data() + N + 1);
		return ret;
	}

	template<typename StringT>
	constexpr auto operator<=>(const StringT& other) const
	{
		return std::basic_string_view<CharT>{ content } <=> other;
	}
};
template<std::size_t N, typename CharT = char>
StaticString(const CharT(&)[N]) -> StaticString<N - 1ull, CharT>;

NAMESPACE_BEGIN(NS_DETAIL)

template<typename T>
inline constexpr std::string_view type_name_impl() noexcept;

template<> inline constexpr std::string_view type_name_impl<void>() noexcept
{ return "void"; }

template<typename T>
inline constexpr std::string_view wrapped_type_name() noexcept
{ return std::source_location::current().function_name(); }

inline constexpr std::size_t wrapped_type_name_prefix_length() noexcept
{ return wrapped_type_name<void>().find(type_name_impl<void>()); }

constexpr std::size_t wrapped_type_name_suffix_length() noexcept
{
	return wrapped_type_name<void>().length()
	     - wrapped_type_name_prefix_length()
	     - type_name_impl<void>().length();
}

template<typename T>
inline constexpr std::string_view type_name_impl() noexcept
{
	constexpr auto wrapped_name = wrapped_type_name<T>();
	constexpr auto prefix_length = wrapped_type_name_prefix_length();
	constexpr auto suffix_length = wrapped_type_name_suffix_length();
	constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
	return wrapped_name.substr(prefix_length, type_name_length);
}
NAMESPACE_END(NS_DETAIL)

template<typename T>
struct type_name
{
	inline static constexpr const auto raw_sv = NS_DETAIL::type_name_impl<T>();
	inline static constexpr const StaticString<raw_sv.size(), char> value{ raw_sv };
};

template<typename T>
inline constexpr auto type_name_v = type_name<T>::value;

NAMESPACE_END(NS_REFLECT)

#endif