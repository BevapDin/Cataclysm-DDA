#pragma once
#ifndef VALUE_OR_REFERENCE_H
#define VALUE_OR_REFERENCE_H

extern "C" {
#include <lua.h>
}

#include "value.h"
#include "reference.h"

// @todo move into namespace catalua and rename to just "value_or_reference"

/**
 * Wrapper class to access objects in Lua that are stored as either a pointer or a value.
 * Technically, this class could inherit from both `LuaValue<T>` and `LuaReference<T>`,
 * but that would basically be the same code anyway.
 * It behaves like a LuaValue if there is a value on the stack, and like LuaReference is there
 * is a reference on the stack. Functions behave like the functions in a `LuaType`.
 * Note that it does not have a push function because it can not know whether to push a reference
 * or a value (copy). The caller must decide this and must use `LuaValue` or `LuaReference`.
 */
template<typename T>
class LuaValueOrReference
{
    public:
        using proxy = typename LuaReference<T>::proxy;
        static proxy get( lua_State *const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return proxy{ &LuaValue<T>::get( L, stack_index ) };
            }
            return LuaReference<T>::get( L, stack_index );
        }
        static void check( lua_State *const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return;
            }
            LuaValue<T *>::check( L, stack_index );
        }
        static bool has( lua_State *const L, int const stack_index ) {
            return LuaValue<T>::has( L, stack_index ) || LuaValue<T *>::has( L, stack_index );
        }
};

#endif
