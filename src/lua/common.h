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
std::string lua_tostring_wrapper( lua_State* const L, int const stack_position );

#endif
