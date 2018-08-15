#pragma once
#ifndef VALUE_OR_REFERENCE_H
#define VALUE_OR_REFERENCE_H

extern "C" {
#include <lua.h>
}

#include "value.h"

// @todo move into namespace catalua and rename to just "value_or_reference"

/**
 * Wrapper class to access objects in Lua that are stored as either a pointer or a value.
 * Functions behave like the functions in a `LuaType`.
 * Note that it does not have a push function because it can not know whether to push a reference
 * or a value (copy). The caller must decide this and must use `LuaValue<T>::push` or `LuaValue<T>::push_ref`.
 */
template<typename T>
class LuaValueOrReference
{
    public:
        static T *get_pointer( lua_State *const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return &LuaValue<T>::get( L, stack_index );
            }
            return LuaPointer<T>::get( L, stack_index );
        }
        static bool has_pointer( lua_State *const L, int const stack_index ) {
            if( lua_isnil( L, stack_index ) ) {
                // If you don't want this, use `has` below, not `has_pointer`
                return true;
            }
            return LuaValue<T>::has( L, stack_index ) || LuaPointer<T>::has( L, stack_index );
        }
        static T &get( lua_State *const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return LuaValue<T>::get( L, stack_index );
            }
            if( T *const ptr = LuaPointer<T>::get( L, stack_index ) ) {
                return *ptr;
            }
            throw std::runtime_error( "wrong value type on stack" );
        }
        static bool has( lua_State *const L, int const stack_index ) {
            return LuaValue<T>::has( L, stack_index ) || LuaPointer<T>::has( L, stack_index );
        }
};

#endif
