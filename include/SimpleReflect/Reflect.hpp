#ifndef __SIMPLE_REFLECT_HEADER__
#define __SIMPLE_REFLECT_HEADER__

#include <tuple>
#include <functional>
#include "Defines.hpp"

NAMESPACE_BEGIN(NS_REFLECT)

template <typename T, template <typename...> typename Template>
struct is_specialization : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

template<typename MemberPtr>
struct MemberPointerTypeInfo;

template<typename Cls, typename T>
struct MemberPointerTypeInfo<T Cls::*>
{
	using ClassType = Cls;
	using MemberType = T;
};

template<typename Cls, typename MemberPtr>
concept is_member_pointer_of = std::is_same_v<
	std::remove_cvref_t<Cls>,
	std::remove_cvref_t<typename MemberPointerTypeInfo<MemberPtr>::ClassType>
>;

NAMESPACE_BEGIN(NS_DETAIL)

template<typename Tp>
	requires std::is_member_pointer_v<Tp>
struct ReflectMemberInfo
{
	String name;
	Tp member_ptr;

	template<typename Cls>
	auto& unwrap_member_pointer(Cls* ptr)
	{
		if constexpr (std::is_member_function_pointer_v<Tp>)
			return member_ptr;
		else
			return ptr->*member_ptr;
	}
};

template<typename MemberPtr>
struct ReflectMemberTypeHelper
{ using type = void; };

template<typename MemberPtr>
	requires std::is_member_object_pointer_v<MemberPtr>
struct ReflectMemberTypeHelper<MemberPtr> {
	using type = typename MemberPointerTypeInfo<MemberPtr>::MemberType;
};
template<typename MemberPtr>
	requires std::is_member_function_pointer_v<MemberPtr>
struct ReflectMemberTypeHelper<MemberPtr> {
	using type = MemberPtr;
};

template<typename MemberPtr>
using ReflectMemberType = ReflectMemberTypeHelper<MemberPtr>::type;
template<typename MemberPtr>
using ReflectMemberTypeRef = std::add_lvalue_reference_t<ReflectMemberType<MemberPtr>>;

template<typename Cls>
	requires (!requires { Cls::__member_ptr_tuple; })
struct GlobalMemberPtrTupleWrapper;
template<typename Cls>
struct GlobalMemberPtrTupleWrapperBase {
	using ThisClass = Cls;
};

template<typename Cls>
struct GetReflectorTypeHelper
{ using type = void; };

template<typename Cls>
	requires requires {
		Cls::__member_ptr_tuple;
		is_specialization<decltype(Cls::__member_ptr_tuple), std::tuple>::value;
	}
struct GetReflectorTypeHelper<Cls> {
	using type = Cls;
};

template<typename Cls>
using ReflectorType = GetReflectorTypeHelper<std::remove_cvref_t<Cls>>::type;

template<typename Cls>
	requires requires {
		GlobalMemberPtrTupleWrapper<Cls>::__member_ptr_tuple;
		is_specialization<decltype(GlobalMemberPtrTupleWrapper<Cls>::__member_ptr_tuple), std::tuple>::value;
	}
struct GetReflectorTypeHelper<Cls> {
	using type = GlobalMemberPtrTupleWrapper<Cls>;
};


template<typename T>
struct Overload;

template<typename Ret, typename ...Args>
struct Overload<Ret(Args...)>
{
	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...)) const
	 	-> decltype(ptr)
	{ return ptr; }

	template<typename Cls>
	constexpr auto operator()(Ret (Cls::*ptr)(Args...) const) const
	 	-> decltype(ptr)
	{ return ptr; }
};

template<typename T>
struct ReflectMethodOverloadImpl;

template<typename Ret, typename ...Args>
struct ReflectMethodOverloadImpl<Ret(Args...)>
{
	template<typename Cls>
	constexpr auto operator()(StringView name, Ret (Cls::*ptr)(Args...)) const
		-> ReflectMemberInfo<decltype(ptr)>
	{ return ReflectMemberInfo{String{name}, ptr}; }

	template<typename Cls>
	constexpr auto operator()(StringView name, Ret (Cls::*ptr)(Args...) const) const
		-> ReflectMemberInfo<decltype(ptr)>
	{ return ReflectMemberInfo{String{name}, ptr}; }
};

template<typename FuncT>
inline constexpr ReflectMethodOverloadImpl<FuncT> ReflectMethodImpl = {};

template<typename Cls, typename FuncT>
struct MemberFunctionPointerHelper;

template<typename Cls, typename Ret, typename ...Args>
struct MemberFunctionPointerHelper<Cls, Ret(Args...)>
{
	using type = Ret (Cls::*)(Args...);
};

NAMESPACE_END(NS_DETAIL)

template<typename FuncT>
inline constexpr NS_DETAIL::Overload<FuncT> Overload = {};

template<typename T>
concept is_reflectable = requires (T t) {
	NS_DETAIL::ReflectorType<T>::__member_ptr_tuple;
	is_specialization<decltype(NS_DETAIL::ReflectorType<T>::__member_ptr_tuple), std::tuple>::value;
};

template<typename Func, typename Cls, typename T>
concept is_for_each_member_invoker = std::is_invocable_v<
	Func,
	Cls*, String, NS_DETAIL::ReflectMemberTypeRef<T>
>;

template<typename Cls, typename FuncT>
using MemberFunctionPointer = NS_DETAIL::MemberFunctionPointerHelper<Cls, FuncT>::type;

NAMESPACE_BEGIN(NS_DETAIL)

template<typename Cls, typename Func, typename ...Tp>
	requires (is_for_each_member_invoker<Func, Cls, Tp> && ...)
void for_each_member_impl_expand(Cls* ptr, Func&& func, NS_DETAIL::ReflectMemberInfo<Tp>& ...pair)
{
	(std::invoke(
		std::forward<Func>(func), ptr, pair.name, pair.unwrap_member_pointer(ptr)),
		...
	);
}

template<typename Cls, typename Func, std::size_t ...Indices>
void for_each_member_impl(Cls* ptr, Func&& func, std::index_sequence<Indices...>)
{
	for_each_member_impl_expand(
		ptr, std::forward<Func>(func),
		std::get<Indices>(ReflectorType<Cls>::__member_ptr_tuple)...
	);
}

template<typename Cls, typename Func, std::size_t ...Indices>
void visit_member_impl(Func&& func, std::index_sequence<Indices...>)
{
	(std::invoke(
		std::forward<Func>(func), std::get<Indices>(ReflectorType<Cls>::__member_ptr_tuple)),
		...
	);
}

NAMESPACE_END(NS_DETAIL)

template<typename Cls, typename Func>
	requires is_reflectable<Cls>
void for_each_member(Cls* ptr, Func&& func)
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::ReflectorType<Cls>::__member_ptr_tuple)>
	>;
	NS_DETAIL::for_each_member_impl(ptr, std::forward<Func>(func), TupleIndex{});
}

template<typename Cls, typename Func>
	requires is_reflectable<Cls>
void visit_member(Cls* ptr, const String& name, Func&& visitor)
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::ReflectorType<Cls>::__member_ptr_tuple)>
	>;
	NS_DETAIL::visit_member_impl<Cls>(
		[&name, visitor, ptr](auto& pair) {
			using MemberType = std::decay_t<decltype(pair.unwrap_member_pointer(ptr))>;
			if constexpr (std::is_invocable_v<Func, Cls*, MemberType&>)
			{
				if (pair.name != name)
					return;
				visitor(ptr, pair.unwrap_member_pointer(ptr));
			}
		},
		TupleIndex{}
	);
}

NAMESPACE_BEGIN(NS_DETAIL)
template<typename Cls, template<class> class Container, std::size_t ...Indices>
constexpr Container<Reflect::String> member_names_impl(std::index_sequence<Indices...>)
{
	constexpr auto extract_name = [](const auto& tuple) {
		return tuple.name;
	};
	return { extract_name(std::get<Indices>(ReflectorType<Cls>::__member_ptr_tuple))... };
}
NAMESPACE_END(NS_DETAIL)

template<typename Cls, template<class> class Container>
constexpr Container<String> member_names()
{
	using TupleIndex = std::make_index_sequence<
		std::tuple_size_v<decltype(NS_DETAIL::ReflectorType<Cls>::__member_ptr_tuple)>
	>;
	return NS_DETAIL::member_names_impl<Cls, Container>(TupleIndex{});
}

template<template<class> class Container, typename Cls>
constexpr Container<String> member_names(Cls* ptr)
{ return member_names<Cls, Container>(); }

template<template<class> class Container, typename Cls>
constexpr Container<String> member_names(Cls& ptr)
{ return member_names<Cls, Container>(); }


NAMESPACE_END(NS_REFLECT)

#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#error "MSVC won't expand macros properly without /Zc:preprocessor flag"
#endif

#define REFLECT_DEFINE_IMPL_0()    \
	template<typename FuncT>       \
	static inline const constexpr NS_REFLECT::NS_DETAIL::ReflectMethodOverloadImpl<FuncT>& \
		REFLECT_METHOD = NS_REFLECT::NS_DETAIL::ReflectMethodImpl<FuncT>;  \
	static inline auto __member_ptr_tuple = std::tuple
#define REFLECT_DEFINE_IMPL_1(cls) \
	using ThisClass = cls; REFLECT_DEFINE_IMPL_0()
#define REFLECT_DEFINE_IMPL_GET(_0, _1, NAME, ...) NAME

// usage: REFLECT_DEFINE() or REFLECT_DEFINE(Class)
#define REFLECT_DEFINE(...) \
	REFLECT_DEFINE_IMPL_GET(_0 __VA_OPT__(,) __VA_ARGS__, REFLECT_DEFINE_IMPL_1, REFLECT_DEFINE_IMPL_0)(__VA_ARGS__)


#define REFLECT_MEMBER_IMPL_2(name, member) NS_REFLECT::NS_DETAIL::ReflectMemberInfo{ name, &ThisClass::member }
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
	template<> struct NS_REFLECT::NS_DETAIL::GlobalMemberPtrTupleWrapper<__VA_ARGS__> \
		: NS_REFLECT::NS_DETAIL::GlobalMemberPtrTupleWrapperBase<__VA_ARGS__>


#endif //! __SIMPLE_REFLECT_HEADER__