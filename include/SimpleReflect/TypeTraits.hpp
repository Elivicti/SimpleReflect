#ifndef __SIMPLE_TYPE_TRAITS_HEADER__
#define __SIMPLE_TYPE_TRAITS_HEADER__

#include <type_traits>
#include "Defines.hpp"

NAMESPACE_BEGIN(NS_REFLECT)

template <typename T, template <typename...> typename Template>
struct is_specialization : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> typename Template>
inline constexpr bool is_specialization_v = is_specialization<T, Template>::value;

NAMESPACE_BEGIN(NS_DETAIL)

template<typename MemberPtr>
struct MemberPointerInfo;

template<typename Cls, typename T>
struct MemberPointerInfo<T Cls::*>
{
	using class_type  = std::remove_cvref_t<Cls>;
	using member_type = T;
	using raw_member_type = std::remove_cvref_t<member_type>;
};

template<typename Cls, typename T>
	requires std::is_member_function_pointer_v<T Cls::*>
struct MemberPointerInfo<T Cls::*>
{
	using class_type  = std::remove_cvref_t<Cls>;
	using member_type = T Cls::*;
	using raw_member_type = T;
};

template<typename MemberPtr>
using MemberPointerClass = typename MemberPointerInfo<MemberPtr>::class_type;
template<typename MemberPtr>
using MemberPointerType  = typename MemberPointerInfo<MemberPtr>::member_type;

NAMESPACE_END(NS_DETAIL)

template<typename MemberPtr, typename Cls>
concept is_member_pointer_of = std::is_same_v<
	std::remove_cvref_t<Cls>,
	std::remove_cvref_t<NS_DETAIL::MemberPointerClass<MemberPtr>>
>;

template<typename Cls, typename FuncT>
struct member_function_pointer;

template<typename Cls, typename Ret, typename ...Args>
struct member_function_pointer<Cls, Ret(Args...)>
{
	using type = Ret (Cls::*)(Args...);
};

template<typename Cls, typename FuncT>
using member_function_pointer_t = typename member_function_pointer<Cls, FuncT>::type;


NAMESPACE_END(NS_REFLECT)

#endif