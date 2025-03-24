# Simple Reflect

A small, header-only static reflection library for C++20, features reflection for class members and enums.

## Usage

See examples for basic usage.

### Class Reflection

**IMPORTANT: For MSVC users, you need to add `/Zc:preprocessor` flag in order to expand macros properly.**

All utilities are defined under `Reflect` namespace.

To declare reflection for a class, use `REFLECT_DEFINE` macro.

```cpp
class MyClass
{
    int a;
    double b;
public:
    REFLECT_DEFINE(MyClass) {
        REFLECT_MEMBER(a),
        REFLECT_MEMBER(b)
    };
};
```

**NOTE:** you need to declare `REFLECT_DEFINE` public.

You can use `REFLECT_MEMBER(var)` or `REFLECT_MEMBER(name, var)` to declare class members that need to be reflected.

> The following examples assume a function similar to `std::print` in C++23 called `print` is defined.

Member functions are also supported:

```cpp
struct MyClass
{
    int a;
    double b;

    void func()
    { ::print("this = {}\n", (void*)this); }

    REFLECT_DEFINE(MyClass) {
        REFLECT_MEMBER(a),
        REFLECT_MEMBER(b),
        REFLECT_MEMBER(func)
    };
};
```

In this case, `func` is not overloaded, so a simple `REFLECT_MEMBER` will do the job. To reflect overloaded member function, use `REFLECT_METHOD`:

```cpp
struct MyClass
{
    int a;
    double b;

    void func()
    { ::print("this = {}\n", (void*)this); }

    void func(int a)
    { ::print("this = {} with param {}\n", (void*)this, a); }

    REFLECT_DEFINE(MyClass) {
        REFLECT_MEMBER(a),
        REFLECT_MEMBER(b),
        REFLECT_METHOD<void(void), "func">(&MyClass::func),
        REFLECT_METHOD<void(int),  "func_int">(&MyClass::func)
    };
};
```

Note that with `REFLECT_METHOD`, you have to manually specify name and member function pointer. If you have a very long class name, you can use `ThisClass` as alias.

Reflection can also be declared outside of a class:

```cpp
REFLECT_DEFINE_GLOBAL(MyClass) {
    REFLECT_DEFINE() {
        REFLECT_MEMBER(a),
        REFLECT_MEMBER(b),
        REFLECT_METHOD<void(void), "func">(&MyClass::func),
        REFLECT_METHOD<void(int),  "func_int">(&MyClass::func)
    };
}
```

Wrap around `REFLECT_DEFINE` with `REFLECT_DEFINE_GLOBAL`, and this time you don't have to specify class name in `REFLECT_DEFINE`'s parameter.

To iterate through a reflected class, call `Reflect::for_each_member`.

```cpp
MyClass x{
    .a = 42,
    .b = 3.14
};
Reflect::for_each_member(&x, [](MyClass* ptr, const std::string& name, auto& mbr) {
    // do something...
});
```

The second parameter of `Reflect::for_each_member` is a callable, that will be called for each reflected member. It takes three parameters. The first is the pointer to the instance, the second is the name of the member. If the member is a function, the third parameter will be a class member function pointer, otherwise it will be a reference to the member.

Note this template function will be instantiate for all member(type)s, make sure your function can work on every member.



### Enum Reflection

All utilities are defined under `Reflect::Enums` namespace.

To convert an enum value to string, use `Reflect::Enums::to_string`:

```cpp
enum class Colors
{
	Red,
	Green,
	Blue
};

Reflect::Enums::to_string(Colors::Red); // output: Colors::Red
// or
Colors color = (Colors)1;
Reflect::Enums::to_string(color); // output: Colors::Green
```

If an enum class is defined inside a class or namespace, the output will have the class name or the namespace, e.g. `MyClass::Colors::Red`.

To iterate through an enum, use `Reflect::Enums::entries`:

```cpp
for (const auto& [ name, value ] : Reflect::Enums::entries<Colors>())
{
    std::cout << name << ' ' << (int)value << '\n';
}
```

By default, reflection only supports enum values that are smaller than `64`, larger values will be ignored. To change this behavior, make a specialization for `struct Reflect::Enums::ReflectConfig`:

```cpp
enum class HttpStatus
{
	OK = 200,
	Accept = 202,
	NotFound = 404
};

template<>
struct Reflect::Enums::ReflectConfig<>
{
	constexpr static const std::size_t max = (std::size_t)HttpStatus::NotFound;
	constexpr static const std::size_t min = (std::size_t)HttpStatus::OK;
};
// or inherit Reflect::Enums::ConfigBase
template<>
struct Reflect::Enums::ReflectConfig<HttpStatus>
	: Reflect::Enums::ConfigBase<(std::size_t)HttpStatus::NotFound> {}
```

Be aware that the name of enum values are `std::string_view`, which is **NOT** a null-terminated string (C style string).

