#pragma once
#ifndef COMMON_H
#define COMMON_H

extern "C" {
#include <lua.h>
}

#include <string>

/// Stores item at the given stack position into the registry.
int luah_store_in_registry( lua_State *L, int stackpos );

/** Safe wrapper to get a Lua string as std::string. Handles nullptr and binary data. */
std::string lua_tostring_wrapper( lua_State *L, int index );

/// Removes item from registry and pushes on the top of stack.
void luah_remove_from_registry( lua_State *L, int index );

/// Sets the metatable for the element on top of the stack.
void luah_setmetatable( lua_State *L, const char *metatable_name );

void luah_setglobal( lua_State *L, const char *name, int index );

/**
 * Invokes the given function with the given Lua state and returns its result.
 * If the function throws, the exception is caught and converted into a Lua error.
 * No exception leaves this function, so its suitable as callback for Luas API.
 */
int catch_exception_for_lua( int( *func )( lua_State * ), lua_State *const L );
/**
 * Same as @ref catch_exception_for_lua, but takes the function as template
 * argument, so it can be used directly in a call to the C API.
 */
template<int ( *func )( lua_State * )>
inline int catch_exception_for_lua_wrapper( lua_State *const L ) {
    return catch_exception_for_lua( func, L );
}

namespace catalua
{
/// Used to invoke @ref luaL_argerror
class argerror : public std::exception
{
    public:
        int narg;
        std::string extramsg;
};
} // namespace catalua

#endif
