#ifndef __SIMPLE_REFLECT_ENUMS_HEADER__
#define __SIMPLE_REFLECT_ENUMS_HEADER__

#include "Defines.hpp"

#include <array>
#include <cstdint>
#include <algorithm>

#ifndef NS_ENUMS
#define NS_ENUMS Enums
#endif

#ifdef USE_WCHAR
#warning "Enum reflection depends on std::source_location, which can not return wchar_t string"
#endif

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
enum class EnumNameHelper { VOID };

template<typename Enum, Enum V>
inline constexpr std::string_view enum_value_name() noexcept;

template<>
inline constexpr std::string_view enum_value_name<EnumNameHelper, EnumNameHelper::VOID>() noexcept
{
#if defined(_MSC_VER)
#define EXPAND(x) #x
#define CONCAT_NS_STR(ns1, ns2, ns3, str) EXPAND(ns1) "::" EXPAND(ns2) "::" EXPAND(ns3) "::" str
	return CONCAT_NS_STR(NS_REFLECT, NS_ENUMS, NS_DETAIL, "EnumNameHelper::VOID");
#undef EXPAND
#undef CONCAT_NS_STR
#else
	return "EnumNameHelper::VOID";
#endif
}

template<typename Enum, Enum V>
inline constexpr std::string_view wrapped_enum_value_name() noexcept
{ return std::source_location::current().function_name(); }

template<typename Enum>
inline constexpr std::size_t wrapped_enum_value_name_prefix_length() noexcept
{
    constexpr auto prefix_len = wrapped_enum_value_name<EnumNameHelper, EnumNameHelper::VOID>()
            .find(enum_value_name<EnumNameHelper, EnumNameHelper::VOID>());
    constexpr auto real_prefix_len = prefix_len
        - (TypeName<EnumNameHelper>.length() - TypeName<Enum>.length());
	return real_prefix_len;
}
inline constexpr std::size_t wrapped_enum_value_name_suffix_length() noexcept
{
	return wrapped_enum_value_name<EnumNameHelper, EnumNameHelper::VOID>().length()
		- wrapped_enum_value_name_prefix_length<EnumNameHelper>()
		- enum_value_name<EnumNameHelper, EnumNameHelper::VOID>().length();
}
template<typename Enum, Enum V>
inline constexpr std::string_view enum_value_name() noexcept
{
	constexpr auto wrapped_name = wrapped_enum_value_name<Enum, V>();
	constexpr auto prefix_length = wrapped_enum_value_name_prefix_length<Enum>();
	constexpr auto suffix_length = wrapped_enum_value_name_suffix_length();
	constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
	return wrapped_name.substr(prefix_length, type_name_length);
}

template<typename Enum, Enum V>
constexpr bool is_valid() noexcept
{
	constexpr std::string_view name{ enum_value_name<Enum, V>() };
	if (name.find("(") != std::string_view::npos)
		return false;
	return !name.empty();
}

template<typename Enum>
constexpr auto get_enum_value(std::size_t v)
{
	using Config = ReflectConfig<Enum>;
	return static_cast<Enum>(Config::min + v);
}

template<typename Enum, std::size_t... I>
constexpr std::size_t valid_value_count(std::index_sequence<I...>)
{
	constexpr bool valid[sizeof...(I)] = { is_valid<Enum, get_enum_value<Enum>(I)>()... };
	return std::count_if(valid, valid + sizeof...(I), [](bool v) { return v; });
}

template<typename Enum, std::size_t... I>
constexpr auto valid_values(std::index_sequence<I...>)
{
	constexpr bool valid[sizeof...(I)] = { is_valid<Enum, get_enum_value<Enum>(I)>()... };
	constexpr std::size_t valid_count = std::count_if(valid, valid + sizeof...(I), [](bool v) { return v; });
	
	std::array<Enum, valid_count> values{};
	for(std::size_t offset = 0, n = 0; n < valid_count; ++offset) {
		if (valid[offset])
		{
			values[n] = get_enum_value<Enum>(offset);
			++n;
		}
	}
	return values;
}

template<typename Enum>
constexpr auto values() noexcept
{
	using Config = ReflectConfig<Enum>;
	return valid_values<Enum>(std::make_index_sequence<Config::size()>{});
}

template<typename Enum>
inline constexpr auto enum_values_v = values<Enum>();

NAMESPACE_END(NS_DETAIL)

template<typename Enum>
struct Entry
{
	std::string_view name;
	Enum value;	
};

template<typename Enum, std::size_t N>
using EntryArray = std::array<Entry<Enum>, N>;

NAMESPACE_BEGIN(NS_DETAIL)

template<typename Enum, std::size_t... I>
constexpr auto entries_impl(std::index_sequence<I...>) noexcept
	-> EntryArray<Enum, enum_values_v<Enum>.size()>
{
    return EntryArray<Enum, enum_values_v<Enum>.size()>{
		{{ enum_value_name<Enum, enum_values_v<Enum>[I]>(), enum_values_v<Enum>[I] }...}
    };
}

NAMESPACE_END(NS_DETAIL)

template<typename Enum>
constexpr std::size_t valid_entry_count() noexcept
{
	using Idx = std::make_index_sequence<ReflectConfig<Enum>::size()>;
	return (std::size_t)NS_DETAIL::valid_value_count<Enum>(Idx{});
}

template<typename Enum>
constexpr auto entries() noexcept
	-> EntryArray<Enum, NS_DETAIL::enum_values_v<Enum>.size()>
{
	using Idx = std::make_index_sequence<NS_DETAIL::enum_values_v<Enum>.size()>;
    return NS_DETAIL::entries_impl<Enum>(Idx{});
}
template<typename Enum>
constexpr std::string_view to_string(Enum v) noexcept
{
	for (const auto& [ name, value ]: entries<Enum>()) {
        if (v == value) return name;
    }
	return {};
}

NAMESPACE_END(NS_ENUMS)
NAMESPACE_END(NS_REFLECT)


#endif //! __SIMPLE_REFLECT_ENUMS_HEADER__
