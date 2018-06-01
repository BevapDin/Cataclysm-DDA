#pragma once
#ifndef TYPE_H
#define TYPE_H

extern "C" {
#include <lua.h>
}

#include "value.h"
#include "reference.h"

// @todo move into namespace catalua and rename to just "value"

/**
 * This is the basic type-checking interface for the Lua bindings generator.
 * Instead of "if type is string, call lua_isstring, if it's int, call lua_isnumber, ...", the
 * generator can just call "LuaType<"..type..">::has".
 * The C++ classes do the actual separation based on the type through the template parameter.
 *
 * Each implementation contains function like the LuaValue has:
 * - @ref has checks whether the object at given stack index is of the requested type.
 * - @ref check calls @ref has and issues a Lua error if the types is not as requested.
 * - @ref get returns the value at given stack_index. This is like @ref LuaValue::get.
 *   If you need to store the value, use \code auto && val = LuaType<X>::get(...); \endcode
 * - @ref push puts the value on the stack, like @ref LuaValue::push
 */
template<typename T>
struct LuaType;

template<>
struct LuaType<int> {
    static bool has( lua_State *const L, int const stack_index ) {
        return lua_isnumber( L, stack_index );
    }
    static void check( lua_State *const L, int const stack_index ) {
        luaL_checktype( L, stack_index, LUA_TNUMBER );
    }
    static int get( lua_State *const L, int const stack_index ) {
        return lua_tonumber( L, stack_index );
    }
    static void push( lua_State *const L, int const value ) {
        lua_pushnumber( L, value );
    }
};

template<>
struct LuaType<bool> {
    static bool has( lua_State *const L, int const stack_index ) {
        return lua_isboolean( L, stack_index );
    }
    static void check( lua_State *const L, int const stack_index ) {
        luaL_checktype( L, stack_index, LUA_TBOOLEAN );
    }
    static bool get( lua_State *const L, int const stack_index ) {
        return lua_toboolean( L, stack_index );
    }
    static void push( lua_State *const L, bool const value ) {
        lua_pushboolean( L, value );
    }
};

template<>
struct LuaType<std::string> {
    static bool has( lua_State *const L, int const stack_index ) {
        return lua_isstring( L, stack_index );
    }
    static void check( lua_State *const L, int const stack_index ) {
        luaL_checktype( L, stack_index, LUA_TSTRING );
    }
    static std::string get( lua_State *const L, int const stack_index ) {
        return lua_tostring_wrapper( L, stack_index );
    }
    static void push( lua_State *const L, const std::string &value ) {
        lua_pushlstring( L, value.c_str(), value.length() );
    }
    // For better performance: if the input is a c-string, forward it as such without wrapping
    // it into a std::string first.
    static void push( lua_State *const L, const char *value ) {
        lua_pushstring( L, value );
    }
};

template<>
struct LuaType<float> : public LuaType<int> { // inherit checking because it's all the same to Lua
    static float get( lua_State *const L, int const stack_index ) {
        return lua_tonumber( L, stack_index );
    }
    static void push( lua_State *const L, float const value ) {
        lua_pushnumber( L, value );
    }
};

template<typename T>
struct LuaType<LuaValue<T>> : public LuaValue<T> {
};

template<typename T>
struct LuaType<LuaReference<T>> : public LuaReference<T> {
};

#endif
