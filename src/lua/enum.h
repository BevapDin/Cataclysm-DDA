#pragma once
#ifndef ENUM_H
#define ENUM_H

#include "type.h"

extern "C" {
#include <lua.h>
}

#include <string>
#include <map>

// @todo move into namespace catalua and rename to just "enum"

/** This basically transforms a string (therefore inheriting from LuaType<string>) into a C++
 * enumeration value. It simply contains a table of string-to-enum-values. */
template<typename E>
class LuaEnum : private LuaType<std::string>
{
    private:
        using Parent = LuaType<std::string>;
        /** Defined by generate_bindings.lua in catabindings.cpp */
        using EMap = std::map<std::string, E>;
        static const EMap BINDINGS;
        static E from_string( const std::string &value ) {
            const auto iter = BINDINGS.find( value );
            if( iter == BINDINGS.end() ) {
                // This point shall not be reached. Always call this with valid input.
                return BINDINGS.begin()->second;
            }
            return iter->second;
        }
        static const std::string &to_string( E const value ) {
            for( auto &e : BINDINGS ) {
                if( e.second == value ) {
                    return e.first;
                }
            }
            // This point shall not be reached. Always call this with valid input.
            return BINDINGS.begin()->first;
        }
        static bool has( const std::string &value ) {
            return BINDINGS.count( value ) > 0;
        }
        static int index( lua_State *const L ) {
            // -1 is the key (function to call)
            const char *const key = lua_tostring( L, -1 );
            if( key == nullptr ) {
                luaL_error( L, "Invalid input to __index: key is not a string." );
            }
            const auto iter = BINDINGS.find( key );
            if( iter == BINDINGS.end() ) {
                return luaL_error( L, "Invalid enum value." );
            }
            lua_remove( L, -1 ); // remove key
            // Push the enum as string, it will be converted back to the enum later. This way, it can
            // be specified both ways in Lua code: either as string or via an entry here.
            lua_pushlstring( L, iter->first.c_str(), iter->first.length() );
            return 1;
        }
    public:
        static bool has( lua_State *const L, int const stack_index ) {
            return Parent::has( L, stack_index ) && has( Parent::get( L, stack_index ) );
        }
        static void check( lua_State *const L, int const stack_index ) {
            Parent::check( L, stack_index );
            if( !has( Parent::get( L, stack_index ) ) ) {
                luaL_argerror( L, stack_index, "invalid value for enum" );
            }
        }
        static E get( lua_State *const L, int const stack_index ) {
            return from_string( Parent::get( L, stack_index ) );
        }
        static void push( lua_State *const L, E const value ) {
            Parent::push( L, to_string( value ) );
        }
        /** Export the enum values as entries of a global metatable */
        static void export_global( lua_State *const L, const char *global_name ) {
            lua_createtable( L, 0, 1 ); // +1
            lua_pushvalue( L, -1 ); // + 1
            // Set the new table to have itself as metatable
            lua_setmetatable( L, -2 ); // -1
            // Setup the __index entry, which will translate the entry to a enum value
            lua_pushcfunction( L, &index ); // +1
            lua_setfield( L, -2, "__index" ); // -1
            // And register as a global value
            lua_setglobal( L, global_name ); // -1
        }
};

template<typename E>
struct LuaType<LuaEnum<E>> : public LuaEnum<E> {
};

#endif
