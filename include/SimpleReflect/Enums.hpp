#ifndef __SIMPLE_REFLECT_ENUMS_HEADER__
#define __SIMPLE_REFLECT_ENUMS_HEADER__

#include "Reflect.hpp"

#include <cstdint>
#include <algorithm>
#include <source_location>

#ifndef NS_ENUMS
#define NS_ENUMS   enums
#endif

NAMESPACE_BEGIN(NS_REFLECT)

template<std::size_t N, typename CharT = char>
struct static_string
{
	constexpr static_string() noexcept
		: content{ 0 } {}

	constexpr static_string(std::basic_string_view<CharT> sv) noexcept
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
	constexpr static_string<N + Len, CharT> operator+(const static_string<Len, CharT>& str) const
	{
		static_string<N + Len, CharT> ret;
		std::copy(content, content + N, ret.data());
		std::copy(str.data(), str.data() + Len, ret.data() + N + 1);
		return ret;
	}
};

NAMESPACE_END(NS_REFLECT)


NAMESPACE_BEGIN(NS_REFLECT)
NAMESPACE_BEGIN(NS_ENUMS)

template<std::size_t MAX = 64, std::size_t MIN = 0>
struct Config
{
	constexpr inline static const std::size_t max = MAX;
	constexpr inline static const std::size_t min = MIN;

	constexpr static std::size_t size()
	{ return max - min + 1; }
};

template<typename Enum>
struct ReflectConfig : Config<> {};

NAMESPACE_BEGIN(NS_DETAIL)
template<typename Enum, Enum V>
constexpr std::string_view pretty_name(std::source_location loc = std::source_location::current()) noexcept
{
	constexpr auto& npos = std::string_view::npos;

	std::string_view name{ loc.function_name() };
	auto begin = name.find("Enum V = ") + 9;
	auto end = name.find_first_of(';', begin);
	
	std::string_view pretty{ name.substr(begin, end - begin) };

	if (pretty.find('(') != npos || pretty.find(')') != npos)
		return "";
	
	if (auto pos = pretty.find_last_of(':'); pos != npos)
		pretty.remove_prefix(pos + 1);

	return pretty;
}
template<typename Enum, Enum V>
constexpr std::string_view name_sv() noexcept
{
	return pretty_name<Enum, V>();
}
NAMESPACE_END(NS_DETAIL)

template<typename Enum, Enum V>
constexpr auto name() noexcept
{
	constexpr std::string_view sv{ NS_DETAIL::name_sv<Enum, V>() };
	return static_string<sv.size()>{ sv };
}
template<typename Enum, Enum V>
inline constexpr auto name_v = NS_DETAIL::name_sv<Enum, V>();

NAMESPACE_BEGIN(NS_DETAIL)
template<typename Enum, Enum V>
constexpr auto is_valid()
{
	constexpr Enum v = static_cast<Enum>(V);
	return !name_v<Enum, V>.empty();
}

template<typename Enum>
constexpr auto ualue(std::size_t v)
{
	using Config = ReflectConfig<Enum>;
	return static_cast<Enum>(Config::min + v);
}

template<typename Enum, std::size_t... I>
constexpr auto valid_count(std::index_sequence<I...>)
{
	constexpr bool valid[sizeof...(I)] = { is_valid<Enum, ualue<Enum>(I)>()... };
	return std::count_if(valid, valid + sizeof...(I), [](bool v) { return v; });
}

template<typename Enum, std::size_t... I>
constexpr auto values_impl(std::index_sequence<I...>) noexcept
{
	constexpr bool valid[sizeof...(I)] = { is_valid<Enum, ualue<Enum>(I)>()... };
	constexpr auto num_valid = std::count_if(valid, valid + sizeof...(I), [](bool v) { return v; });
	static_assert(num_valid > 0, "no support for empty enums");
	
	std::array<Enum, num_valid> values{};
	for(std::size_t offset = 0, n = 0; n < num_valid; ++offset) {
		if (valid[offset]) {
			values[n] = ualue<Enum>(offset);
			++n;
		}
	}
	return values;
}
template<typename Enum>
constexpr auto values() noexcept
	-> std::array<Enum, valid_count<Enum>(std::make_index_sequence<ReflectConfig<Enum>::max - ReflectConfig<Enum>::min + 1>{})>
{
	using Conf = ReflectConfig<Enum>;
	constexpr auto enum_size = Conf::size();
	return values_impl<Enum>(std::make_index_sequence<enum_size>{});
}
NAMESPACE_END(NS_DETAIL)

template<typename Enum>
using Entry = std::pair<Enum, std::string_view>;

template<typename Enum, std::size_t N>
using EntryArray = std::array<Entry<Enum>, N>;

template<typename Enum>
inline constexpr auto values_v = NS_DETAIL::values<Enum>();

NAMESPACE_BEGIN(NS_DETAIL)
template<typename Enum, std::size_t... I>
constexpr auto entries_impl(std::index_sequence<I...>) noexcept
	-> EntryArray<Enum, values_v<Enum>.size()>
{
    return EntryArray<Enum, values_v<Enum>.size()>{
		{{ values_v<Enum>[I], name_v<Enum, values_v<Enum>[I]>}...}
    };
}
NAMESPACE_END(NS_DETAIL)

template<typename Enum>
constexpr auto entries() noexcept
	-> EntryArray<Enum, values_v<Enum>.size()>
{
    return NS_DETAIL::entries_impl<Enum>(std::make_index_sequence<values_v<Enum>.size()>());
}
template<typename Enum>
constexpr std::string_view to_string(Enum value) noexcept
{
	for (const auto& [ key, name ]: entries<Enum>()) {
        if (value == key) return name;
    }
	return {};
}

NAMESPACE_END(NS_ENUMS)
NAMESPACE_END(NS_REFLECT)


#endif //! __SIMPLE_REFLECT_ENUMS_HEADER__
