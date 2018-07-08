#pragma once

#include "push_value_onto_stack.gen.h"

#include <string>
#include <type_traits>

class lua_engine;

void push_value_onto_stack( const lua_engine &engine, const char * const value );
void push_value_onto_stack( const lua_engine &engine, const std::string &value );
void push_value_onto_stack( const lua_engine &engine, bool value );
void push_integer_onto_stack( const lua_engine &engine, long long int value );
void push_float_onto_stack( const lua_engine &engine, long double value );

template<typename T>
inline typename std::enable_if<!std::is_same<T, bool>::value && std::is_arithmetic<T>::value && !std::is_floating_point<T>::value, void>::type push_value_onto_stack( const lua_engine &engine, const T &value ) {
    return push_integer_onto_stack( engine, value );
}

template<typename T>
inline typename std::enable_if<!std::is_same<T, bool>::value && std::is_arithmetic<T>::value && std::is_floating_point<T>::value, void>::type push_value_onto_stack( const lua_engine &engine, const T &value ) {
    return push_float_onto_stack( engine, value );
}

// Don't call this directly.
template<typename T>
T pop_from_stack( const lua_engine &, int );

namespace catalua
{

// Don't call this directly.
void push_mod_callback_call( const lua_engine & );
// Don't call this directly.
void push_script( const lua_engine &, const std::string & );
// Don't call this directly.
void call_void_function( const lua_engine &, int );
// Don't call this directly.
void call_non_void_function( const lua_engine &, int );
// Don't call this directly.
inline int push_onto_stack( const lua_engine &/*engine*/ )
{
    return 0;
}
// Don't call this directly.
template<typename Head, typename ... Args>
inline int push_onto_stack( const lua_engine &engine, Head &&head, Args &&... args )
{
    push_value_onto_stack( engine, std::forward<Head>( head ) );
    return 1 + push_onto_stack( engine, std::forward<Args>( args )... );
}

namespace detail {

// Specialized for void and non-void return types, because that
// can't be done via specialization functions.
template<typename T>
class call_wrapper;

template<>
class call_wrapper<void>
{
    public:
        template<typename ... Args>
        static void call( const lua_engine &engine, const std::string &script, Args &&... args ) {
            push_script( engine, script );
            const int cnt = push_onto_stack( engine, std::forward<Args>( args )... );
            call_void_function( engine, cnt );
        }
};

template<typename T>
class call_wrapper
{
    public:
        template<typename ... Args>
        static T call( const lua_engine &engine, const std::string &script, Args &&... args ) {
            push_script( engine, script );
            const int cnt = push_onto_stack( engine, std::forward<Args>( args )... );
            call_non_void_function( engine, cnt );
            return pop_from_stack<T>( engine, -1 );
        }
};
} // namespace detail

/**
 * Generic global function to run any arbitrary Lua code.
 *
 * @tparam value_type The type that is returned by the script. Can be simply `void`.
 * This must be specified explicitly as the compiler can not detect it. Note that only
 * few types are actually supported here. It can be a reference.
 * @throws @ref std::exception if the Lua code does not work out )-; 
 *
 * The interface is designed to *not* require the `lua_engine` to be a complete type,
 * that means one can use the interface while only having a *reference* to a `lua_engine`.
 * 
 * Usage:
 *
<code>
item result = catalua::call<item>( *g->lua_engine_ptr, "some lua script that returns an item value", some, more, parameters );
</code>
 */
template<typename value_type, typename ... Args>
inline value_type call( const lua_engine &engine, const std::string &script, Args &&... args )
{
    return detail::call_wrapper<value_type>::call( engine, script, std::forward<Args>( args )... );
}

/**
 * Invokes a callback that can be registered by each mod. The callback is invoked for each mod.
 * The function does *not* return anything as it may result in several calls (for several mods),
 * or in no call at all (no mod has registered the callback).
 * @param name Name of the callback. Should be a simple constant identifier name. Make sure to
 * document the callbacks.
 * @throws No errors from Lua, those are reported a different way.
 */
//@todo where to document the callbacks?
template<typename ... Args>
inline void mod_callback( const lua_engine &engine, const char *const name, Args &&... args )
{
    try {
        push_mod_callback_call( engine );
        // This pushes a reference to a function in Lua onto the stack, that function
        // dispatches this callback to all registered mods.
        // The name of the callback is pushed together with the arguments below.
        const int cnt = push_onto_stack( engine, name, std::forward<Args>( args )... );
        call_void_function( engine, cnt );
    } catch( const std::exception &err ) {
        // @todo handle this
        (void) err;
    }
}

} // namespace catalua
