#pragma once

#include <string>
#include <type_traits>

class lua_engine;

// This is implemented for various types by the wrapper generator.
template<typename T>
T get_wrapped_from_stack( const lua_engine &, int );
// This is implemented for various types by the wrapper generator.
template<typename T>
void push_wrapped_onto_stack( const lua_engine &engine, const T &value );

namespace catalua
{

/**
 * Don't call anything in this namespace directly, except:
 * @ref push_value and @ref get_value.
 *
 * Those are the generic functions that can be called with all supported types.
 * There are several implementations of those functions, they simply defer to
 * the actual worker functions. Note that Lua only has *one* generic floating
 * point and *one* generic integer type, as well as string and boolean build in.
 * So all calls with a string (C-String, std::string) or arithmetic type will
 * call the wrapper for those build in Lua types.
 * The remaining calls will either fail (with no suitable function to be called,
 * happens for unions (which are not supported by Lua / the wrapper at all)),
 * or they calls the wrapper functions implemented by the wrapper generator.
 */
namespace stack
{

void push_value( const lua_engine &engine, const char * const value );
void push_value( const lua_engine &engine, const std::string &value );
void push_value( const lua_engine &engine, bool value );

bool get_bool( const lua_engine &engine, int index );
template<typename T>
inline typename std::enable_if<std::is_same<T, bool>::value, T>::type get_value( const lua_engine &engine, const int index ) {
    return get_bool( engine, index );
}

std::string get_string( const lua_engine &engine, int index );
template<typename T>
inline typename std::enable_if<std::is_same<T, std::string>::value, T>::type get_value( const lua_engine &engine, const int index ) {
    return get_string( engine, index );
}

// This excludes types that are definitively *not* wrapped (like primitives).
template<typename T>
struct is_maybey_wrapped : public std::integral_constant < bool, ( ( std::is_class<T>::value ||
        std::is_enum<T>::value ) ||
( std::is_reference<T>::value &&std::is_class<typename std::decay<T>::type>::value ) ) && !std::is_same<typename std::decay<T>::type, std::string>::value > { };

template<typename T>
inline typename std::enable_if<is_maybey_wrapped<T>::value, void>::type push_value( const lua_engine &engine, const T &value ) {
    return push_wrapped_onto_stack( engine, value );
}
template<typename T>
inline typename std::enable_if<is_maybey_wrapped<T>::value, T>::type get_value( const lua_engine &engine, const int index ) {
    return get_wrapped_from_stack<T>( engine, index );
}

template<typename T>
struct is_integer : public std::integral_constant < bool,
    !std::is_same<T, bool>::value &&std::is_arithmetic<T>::value &&!std::is_floating_point<T >::value> { };

template<typename T>
struct is_float : public std::integral_constant < bool,
    !std::is_same<T, bool>::value &&std::is_arithmetic<T>::value &&std::is_floating_point<T >::value> { };

// Lua only has one integer type, push an instance of it.
void push_integer( const lua_engine &engine, long long int value );
template<typename T>
inline typename std::enable_if<is_integer<T>::value, void>::type push_value( const lua_engine &engine, const T &value ) {
    return push_integer( engine, value );
}
long long int get_integer( const lua_engine &engine, int index );
template<typename T>
inline typename std::enable_if<is_integer<T>::value, T>::type get_value( const lua_engine &engine, const int index ) {
    return get_integer( engine, index );
}
// Lua only has one floating point type, push an instance of it.
void push_float( const lua_engine &engine, long double value );
template<typename T>
inline typename std::enable_if<is_float<T>::value, void>::type push_value( const lua_engine &engine, const T &value ) {
    // @todo handle overfloating values
    return static_cast<T>( push_float( engine, value ) );
}
long double get_float( const lua_engine &engine, int index );
template<typename T>
inline typename std::enable_if<is_float<T>::value, T>::type get_value( const lua_engine &engine, const int index ) {
    // @todo handle overfloating values
    return static_cast<T>( get_float( engine, index ) );
}

inline int push_all( const lua_engine &/*engine*/ )
{
    return 0;
}
template<typename Head, typename ... Args>
inline int push_all( const lua_engine &engine, Head &&head, Args &&... args )
{
    push_value( engine, std::forward<Head>( head ) );
    return 1 + push_all( engine, std::forward<Args>( args )... );
}

void push_script( const lua_engine &, const std::string & );

void call_void_function( const lua_engine &, int );
void call_non_void_function( const lua_engine &, int );

// Specialized for void and non-void return types, because that
// can't be done via specialization functions.
template<typename T>
class call_wrapper;

template<>
class call_wrapper<void>
{
    public:
        static void call( const lua_engine &engine, const int cnt ) {
            call_void_function( engine, cnt );
        }
};

template<typename T>
class call_wrapper
{
    public:
        static T call( const lua_engine &engine, const int cnt ) {
            call_non_void_function( engine, cnt );
            return get_value<T>( engine, -1 );
        }
};

} // namespace stack

/**
 * @brief Generic global function to run any arbitrary Lua code.
 *
 * Usage:
 *
<code>
item result = catalua::call<item>( *g->lua_engine_ptr, "some lua script that returns an item value", some, more, parameters );
</code>
 *
 * @tparam value_type The type that is returned by the script. Can be simply `void`.
 * This must be specified explicitly as the compiler can not detect it. Note that only
 * few types are actually supported here. It can be a reference (but not a `std::reference_wrapper`).
 * @throws @ref std::exception if the Lua code does not work out )-;
 */
template<typename value_type, typename ... Args>
inline value_type call( const lua_engine &engine, const std::string &script, Args &&... args )
{
    stack::push_script( engine, script );
    const int cnt = stack::push_all( engine, std::forward<Args>( args )... );
    return stack::call_wrapper<value_type>::call( engine, cnt );
}

} // namespace catalua
