#ifndef __SIMPLE_REFLECT_HEADER__
#define __SIMPLE_REFLECT_HEADER__

#include <tuple>
#include <functional>

#include "Defines.hpp"
#include "TypeTraits.hpp"

#define MEMBER_TYPE_INFO_TUPLE MEMBER_TYPE_INFO_TUPLE_

NAMESPACE_BEGIN(NS_REFLECT)

NAMESPACE_BEGIN(NS_DETAIL)

//////////////////////////////////////////////////////////////

// This class holds the actual class member pointer and it's name.
template<StaticString Name, typename MemberPointer>
	requires std::is_member_pointer_v<MemberPointer>
struct MemberTypeInfo
{
	constexpr static bool is_function_pointer = std::is_member_function_pointer_v<MemberPointer>;
	constexpr static bool is_object_pointer   = std::is_member_object_pointer_v  <MemberPointer>;

	using class_type  = NS_DETAIL::MemberPointerClass<MemberPointer>;
	using member_type = NS_DETAIL::MemberPointerType <MemberPointer>;

	using member_ref  = std::add_lvalue_reference_t<member_type>;

	using char_type = typename decltype(Name)::value_type;
	using string_view_type = std::basic_string_view<char_type>;

	constexpr static string_view_type name = Name;
	MemberPointer member;

	template<typename Cls>
	auto& to_real_variable(Cls* ptr)
	{
		if constexpr (std::is_member_function_pointer_v<MemberPointer>)
			return member;
		else
			return ptr->*member;
	}
};

// This class is the MEMBER_TYPE_INFO_TUPLE wrapper in global.
// It holds the MEMBER_TYPE_INFO_TUPLE for Cls.
template<typename Cls>
	requires (!requires { Cls::MEMBER_TYPE_INFO_TUPLE; })
struct GlobalMemberInfoTupleWrapper;

template<typename Cls>
struct GlobalMemberInfoTupleWrapperBase {
	using ThisClass = Cls;
};

//////////////////////////////////////////////////////////////

// Get type that actually holds MEMBER_TYPE_INFO_TUPLE.
// It's either the Cls itself, or GlobalMemberInfoTupleWrapper<Cls>.
template<typename Cls>
struct MemberInfoWrapper {
	using type = void;
};

template<typename Cls>
	requires requires {
		Cls::MEMBER_TYPE_INFO_TUPLE;
		is_specialization<decltype(Cls::MEMBER_TYPE_INFO_TUPLE), std::tuple>::value;
	}
struct MemberInfoWrapper<Cls> {
	using type = Cls;
};
template<typename Cls>
	requires requires {
		GlobalMemberInfoTupleWrapper<Cls>::MEMBER_TYPE_INFO_TUPLE;
		is_specialization<
			decltype(GlobalMemberInfoTupleWrapper<Cls>::MEMBER_TYPE_INFO_TUPLE), std::tuple
		>::value;
	}
struct MemberInfoWrapper<Cls> {
	using type = GlobalMemberInfoTupleWrapper<Cls>;
};

template<typename Cls>
using MemberInfoWrapperType = typename MemberInfoWrapper<std::remove_cvref_t<Cls>>::type;

//////////////////////////////////////////////////////////////

// This class is used to create overloaded function pointer
template<typename T>
struct Overload;

template<typename Ret, typename ...Args>
struct Overload<Ret(Args...)>
{
	constexpr auto operator()(Ret (*ptr)(Args...)) const
	 	-> decltype(ptr)
	{ return ptr; }

	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...)) const
	 	-> decltype(ptr)
	{ return ptr; }

	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...) const) const
	 	-> decltype(ptr)
	{ return ptr; }
};


// This class is used to create overloaded member function pointer reflection
template<StaticString Name, typename T>
struct ReflectMethodOverloadImpl;

template<StaticString Name, typename Ret, typename ...Args>
struct ReflectMethodOverloadImpl<Name, Ret(Args...)>
{
	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...)) const
		-> MemberTypeInfo<Name, decltype(ptr)>
	{ return MemberTypeInfo<Name, decltype(ptr)>{ ptr }; }

	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...) const) const
		-> MemberTypeInfo<Name, decltype(ptr)>
	{ return MemberTypeInfo<Name, decltype(ptr)>{ ptr }; }
};

template<typename FuncT, StaticString Name>
inline constexpr ReflectMethodOverloadImpl<Name, FuncT> ReflectMethodImpl = {};

//////////////////////////////////////////////////////////////

NAMESPACE_END(NS_DETAIL)

template<typename FuncT>
inline constexpr NS_DETAIL::Overload<FuncT> overload = {};

//////////////////////////////////////////////////////////////

template<typename T>
struct is_reflectable : std::false_type {};

template<typename T>
	requires requires (T t) {
		NS_DETAIL::MemberInfoWrapperType<T>::MEMBER_TYPE_INFO_TUPLE;
		is_specialization<decltype(NS_DETAIL::MemberInfoWrapperType<T>::MEMBER_TYPE_INFO_TUPLE), std::tuple>::value;
	}
struct is_reflectable<T> : std::true_type {};

template<typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

template<typename T>
concept reflectable = is_reflectable_v<T>;

//////////////////////////////////////////////////////////////

NAMESPACE_BEGIN(NS_DETAIL)

NAMESPACE_END(NS_DETAIL)

// Type trait class that determine if Func can be invoked as
// Func(Cls*, StringT, MemberT&).
// Cls and MemberT can be const.
template<typename Func, typename Cls, typename MemberT, typename StringT = StringView>
struct is_for_each_member_invokable : std::false_type {};

template<typename Func, typename Cls, typename MemberT, typename StringT>
	requires std::is_invocable_v<
		Func,
		Cls*, StringT, std::remove_reference_t<MemberT>&
	>
struct is_for_each_member_invokable<Func, Cls, MemberT, StringT> : std::true_type {};

template<typename Func, typename Cls, typename MemberT, typename StringT = StringView>
inline constexpr bool is_for_each_member_invokable_v =
	is_for_each_member_invokable<Func, Cls, MemberT, StringT>::value;

template<typename Func, typename Cls, typename MemberT, typename StringT = StringView>
concept for_each_member_invokable =
	is_for_each_member_invokable_v<Func, Cls, StringT, MemberT>;

//////////////////////////////////////////////////////////////

NAMESPACE_BEGIN(NS_DETAIL)

template<typename Func, typename Cls, typename MemberInfo>
inline constexpr bool member_type_info_invokable = is_for_each_member_invokable<
	Func, Cls,
	typename MemberInfo::member_type, typename MemberInfo::string_view_type
>::value;

template<typename Cls, typename Func, typename ...MemberInfoT>
	requires (member_type_info_invokable<Func, Cls, MemberInfoT> && ...)
void for_each_member_impl_expand(Cls* ptr, Func&& func, MemberInfoT& ...pair)
{
	(std::invoke(
		std::forward<Func>(func), ptr, pair.name, pair.to_real_variable(ptr)
	), ...);
}

template<typename Cls, typename Func, std::size_t ...Indices>
void for_each_member_impl(Cls* ptr, Func&& func, std::index_sequence<Indices...>)
{
	for_each_member_impl_expand(
		ptr, std::forward<Func>(func),
		std::get<Indices>(MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)...
	);
}

template<typename Cls, typename Func, std::size_t ...Indices>
void visit_member_impl(Func&& func, std::index_sequence<Indices...>)
{
	(std::invoke(
		std::forward<Func>(func), std::get<Indices>(MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)
	), ...);
}

NAMESPACE_END(NS_DETAIL)

// Iterate through all reflect members of Cls, in order of they were declared
template<reflectable Cls, typename Func>
constexpr void for_each_member(Cls* ptr, Func&& func)
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)>
	>;
	NS_DETAIL::for_each_member_impl(ptr, std::forward<Func>(func), TupleIndex{});
}


// Because function parameters are always considered runtime variable, Func will be instantiate for all member(type)s
// that it can be invoked, but is only invoked when name is equal.
// If no member in Cls with specified name exists, this function does nothing (and will not cause compile error).
template<reflectable Cls, typename Func, typename StringT>
constexpr void visit_member(Cls* ptr, const StringT& name, Func&& visitor)
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)>
	>;
	NS_DETAIL::visit_member_impl<Cls>(
		[&name, visitor, ptr](auto& pair) {
			using MemberType = std::decay_t<decltype(pair.to_real_variable(ptr))>;
			if constexpr (std::is_invocable_v<Func, Cls*, MemberType&>)
			{
				if (pair.name != name)
					return;
				visitor(ptr, pair.to_real_variable(ptr));
			}
		},
		TupleIndex{}
	);
}
// This is the static version of `visit_member`, Func will only be instantiate for member with
// specified name.
// If no member in Cls with specified name exists, this function does nothing (and will not cause compile error).
template<StaticString Name, reflectable Cls, typename Func>
constexpr void visit_member(Cls* ptr, Func&& visitor)
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)>
	>;
	NS_DETAIL::visit_member_impl<Cls>(
		[visitor, ptr](auto& pair) {
			if constexpr (pair.name == Name) {
				visitor(ptr, pair.to_real_variable(ptr));
			}
		},
		TupleIndex{}
	);
}

NAMESPACE_BEGIN(NS_DETAIL)
template<typename Cls, typename Container, std::size_t ...Indices>
constexpr Container member_names_impl(std::index_sequence<Indices...>)
{
	constexpr auto extract_name = [](const auto& tuple) {
		return typename Container::value_type{ tuple.name };
	};
	return { extract_name(std::get<Indices>(MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE))... };
}
NAMESPACE_END(NS_DETAIL)

// Get all reflected member names from Cls, store them in Container.
// The name of each member will be cast to typename Container::value_type
template<typename Cls, typename Container>
constexpr Container member_names()
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::MemberInfoWrapperType<Cls>::MEMBER_TYPE_INFO_TUPLE)>
	>;
	return NS_DETAIL::member_names_impl<Cls, Container>(TupleIndex{});
}

// Get all reflected member names from Cls, store them in Container.
// The name of each member will be cast to typename Container::value_type.
// A helper function that can deduce Cls from parameter
template<typename Container, typename Cls>
constexpr Container member_names([[maybe_unused]] Cls* ptr)
{ return member_names<Cls, Container>(); }

// Get all reflected member names from Cls, store them in Container.
// The name of each member will be cast to typename Container::value_type.
// A helper function that can deduce Cls from parameter
template<typename Container, typename Cls>
constexpr Container member_names([[maybe_unused]] Cls& ptr)
{ return member_names<Cls, Container>(); }

// Get all reflected member names from Cls, store them in Container.
// The name of each member will be cast to Reflect::StringView.
// A helper function that can deduce Cls from parameter
template<template<class> class Container, typename Cls>
constexpr Container<StringView> member_names([[maybe_unused]] Cls* ptr)
{ return member_names<Cls, Container<StringView>>(); }

// Get all reflected member names from Cls, store them in Container.
// The name of each member will be cast to Reflect::StringView.
// A helper function that can deduce Cls from parameter
template<template<class> class Container, typename Cls>
constexpr Container<StringView> member_names([[maybe_unused]] Cls& ptr)
{ return member_names<Cls, Container<StringView>>(); }


NAMESPACE_END(NS_REFLECT)

#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#error "MSVC won't expand macros properly without /Zc:preprocessor flag"
#endif

#define REFLECT_DEFINE_IMPL_0()    \
	template<typename FuncT, NS_REFLECT::StaticString Name>       \
	static inline const constexpr auto& \
		REFLECT_METHOD = NS_REFLECT::NS_DETAIL::ReflectMethodImpl<FuncT, Name>;  \
	static inline auto MEMBER_TYPE_INFO_TUPLE = std::tuple
#define REFLECT_DEFINE_IMPL_1(cls) \
	using ThisClass = cls; REFLECT_DEFINE_IMPL_0()
#define REFLECT_DEFINE_IMPL_GET(_0, _1, NAME, ...) NAME

// usage: REFLECT_DEFINE() or REFLECT_DEFINE(Class)
#define REFLECT_DEFINE(...) \
	REFLECT_DEFINE_IMPL_GET(_0 __VA_OPT__(,) __VA_ARGS__, REFLECT_DEFINE_IMPL_1, REFLECT_DEFINE_IMPL_0)(__VA_ARGS__)


#define REFLECT_MEMBER_IMPL_2(name, member) \
	NS_REFLECT::NS_DETAIL::MemberTypeInfo<name, decltype(&ThisClass::member)>{ &ThisClass::member }
#ifdef USE_WCHAR
#define REFLECT_MEMBER_IMPL_1(member) REFLECT_MEMBER_IMPL_2(_L(#member), member)
#else
#define REFLECT_MEMBER_IMPL_1(member) REFLECT_MEMBER_IMPL_2(#member, member)
#endif
#define REFLECT_MEMBER_IMPL_GET(_1, _2, NAME, ...) NAME

// usage: REFLECT_MEMBER(name, member) or REFLECT_MEMBER(member)
#define REFLECT_MEMBER(...) \
	REFLECT_MEMBER_IMPL_GET(__VA_ARGS__, REFLECT_MEMBER_IMPL_2, REFLECT_MEMBER_IMPL_1)(__VA_ARGS__)

// use __VA_ARGS__ to handle templates, like: std::map<int, int>
#define REFLECT_DEFINE_GLOBAL(...)    \
	template<> struct NS_REFLECT::NS_DETAIL::GlobalMemberInfoTupleWrapper<__VA_ARGS__> \
		: NS_REFLECT::NS_DETAIL::GlobalMemberInfoTupleWrapperBase<__VA_ARGS__>


#endif //! __SIMPLE_REFLECT_HEADER__