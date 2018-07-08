#pragma once
#ifndef REFERENCE_H
#define REFERENCE_H

#include "value.h"
#include "common.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <string>
#include <type_traits>

// @todo move into namespace catalua and rename to just "reference"

/**
 * This is special wrapper (an extension) for references to objects which are not stored in Lua,
 * but are kept in the memory managed by C++. This class only stores and retrieves the pointers,
 * you have to make sure those pointers stay valid.
 *
 * Example (an @ref itype is loaded when a world is loaded and stays valid until the game ends):
 * \code
 * itype *it = type::find_type( "water" );
 * LuaReference<itype>::push( L, it ); // copies the pointer it
 * ... // give control back to Lua, wait for a callback from it,
 * itype &it = LuaValue<itype*>::get( L, 1 ); // get the first argument of the callback
 * assert(it.id == "water");
 * \endcode
 *
 * This class extends LuaValue by some pointer specific behavior:
 * - @ref push is overloaded to accept a reference to T (which will be converted to a pointer
 *   and stored). Additionally, if the pointer passed to @ref push is nullptr, nil will be pushed
 *   (this obviously does not work for references).
 *   \code
 *   Foo *x = ...;
 *   LuaReference<Foo>::push( L, x );
 *   LuaReference<Foo>::push( L, *x ); // both push calls do exactly the same.
 *   \endcode
 *   push is also overloaded to accept const and non-const references / pointers. The templated
 *   third parameter there makes sure that this is only done when T is not const. Otherwise we
 *   would end up with 2 identical push functions, both taking a const references.
 * - @ref get returns a proxy object. It contains the pointer to T. It will automatically convert
 *   to a reference / a pointer to T:
 *   \code
 *   void f_ptr( itype* );
 *   void f_ref( itype& );
 *   auto proxy = LuaReference<itype>::get( L, 1 );
 *   f_ptr( proxy ); // proxy converts to itype*
 *   f_ref( proxy ); // proxy converts to itype&
 *   itype *it = proxy;
 *   \endcode
 *   If you only need a reference (e.g. to call member functions or access members), use
 *   @ref LuaValue<T*>::get instead:
 *   \code
 *   itype &it = LuaValue<itype*>::get( L, 1 );
 *   std::string name = it.nname();
 *   \endcode
 */
template<typename T>
class LuaReference : private LuaValue<T *>
{
    public:
        template<typename U = T>
        static void push( lua_State *const L, T *const value,
                          typename std::enable_if < !std::is_const<U>::value >::value_type * = nullptr ) {
            if( value == nullptr ) {
                lua_pushnil( L );
                return;
            }
            LuaValue<T *>::push( L, value );
        }
        // HACK: because Lua does not known what const is.
        static void push( lua_State *const L, const T *const value ) {
            if( value == nullptr ) {
                lua_pushnil( L );
                return;
            }
            LuaValue<T *>::push( L, const_cast<T *>( value ) );
        }
        template<typename U = T>
        static void push( lua_State *const L, T &value,
                          typename std::enable_if < !std::is_const<U>::value >::value_type * = nullptr ) {
            LuaValue<T *>::push( L, &value );
        }
        // HACK: because Lua does not known what const is.
        static void push( lua_State *const L, const T &value ) {
            LuaValue<T *>::push( L, const_cast<T *>( &value ) );
        }
        static int push_reg( lua_State *const L, T *const value ) {
            push( L, value );
            return luah_store_in_registry( L, -1 );
        }
        static int push_reg( lua_State *const L, T &value ) {
            return LuaValue<T *>::push_reg( L, &value );
        }
        /** A proxy object that allows to convert the reference to a pointer on-demand. The proxy object can
         * be used as argument to functions that expect either a pointer and to functions expecting a
         * reference. */
        struct proxy {
            T *ref;
            operator T *() {
                return ref;
            }
            operator T &() {
                return *ref;
            }
            T *operator &() {
                return ref;
            }
        };
        /** Same as calling @ref get, but returns a @ref proxy containing the reference. */
        static proxy get( lua_State *const L, int const stack_position ) {
            return proxy{ &LuaValue<T *>::get( L, stack_position ) };
        }
        using LuaValue<T *>::has;
};

#endif
