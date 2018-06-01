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

#endif
