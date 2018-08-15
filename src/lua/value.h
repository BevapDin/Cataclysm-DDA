#pragma once
#ifndef VALUE_H
#define VALUE_H

#include "common.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <string>
#include <stdexcept>

template<typename T>
class LuaPointer;

// @todo move into namespace catalua and rename to just "value"

/**
 * Base interface for values, that are copied into Luas own memory (and thereby managed by Lua).
 * The class creates a metatable for the wrapped objects, this is all set up via
 * generate_bindings.lua.
 * Usage: there are two main functions you might need: @ref push and @ref get.
 * - @ref push copies the object into Luas memory and pushes a reference to it on the stack.
 *   It is like @ref lua_pushnumber, only it pushes a whole object.
 * - @ref get reads a value from the stack and returns a reference to it (the memory of the object
 *   is managed by Lua and is kept until the garbage collector frees it).
 *
 * You can expect the following behavior:
 * \code
 * const Foo &myfoo = get_my_foo( ... );
 * LuaValue<Foo>::push( L, myfoo ); // copies myfoo
 * ... // give control back to Lua, wait for a callback from it,
 * Foo &thefoo = LuaValue<Foo>::get( L, 1 ); // get the first argument of the callback
 * thefoo.something(); // do something with it, not that myfoo and thefoo are different objects
 * \endcode
 *
 * @param T is the type of object that should be managed. It must be copy-constructible.
 */
template<typename T>
class LuaValue
{
    static_assert( !std::is_pointer<T>::value, "LuaValue is only for class types, not for pointers, use LuaPointer instead" );
    private:
        friend class LuaPointer<T>;
        /** Defined by generate_bindings.lua in catabindings.cpp */
        static const char *const METATABLE_NAME;
        /** Defined by generate_bindings.lua in catabindings.cpp */
        static const luaL_Reg FUNCTIONS[];
        static T *get_subclass( lua_State *S, int stack_index );

        // Calls `object->T::~T();` but putting this in a separate compilation unit
        // means using this class LuaValue does not require T to be a complete type.
        static void call_destructor( T &object );
        static int gc( lua_State *const L ) {
            T *object = static_cast<T *>( lua_touserdata( L, 1 ) );
            call_destructor( *object );
            lua_pop( L, 1 );
            return 0;
        }
        static int get_member( lua_State *L, const T &instance, const char *name );
        static int set_member( lua_State *L, T &instance, const char *name );
        /**
         * Wrapper for the Lua __index entry in the metatable of the userdata.
         * It queries the actual metatable in case the call goes to a function (and does not request
         * and actual class member) and returns that function (if found).
         * If there is no function of the requested name, it looks up the name in @ref READ_MEMBERS,
         * if it's there, it calls the function that the entry refers to (which acts as a getter).
         * Finally it returns nil, which is what Lua would have used anyway.
         */
        static int index( lua_State *const L ) {
            // -2 is the userdata, -1 is the key (function to call)
            const char *const key = lua_tostring( L, -1 );
            if( key == nullptr ) {
                throw std::runtime_error( "Invalid input to __index: key is not a string." );
            }
            if( luaL_getmetafield( L, -2, key ) != 0 ) {
                // There is an entry of that name, return it.
                lua_remove( L, -3 ); // remove userdata
                lua_remove( L, -2 ); // remove key
                // -1 is now the things we have gotten from luaL_getmetafield, return it.
                return 1;
            }
            const T &instance = get( L, 1 );
            return get_member( L, instance, key );
        }
        /**
         * Wrapper for the Lua __newindex entry in the metatable of the userdata.
         * It looks up the name of the requested member in @ref WRITE_MEMBERS and (if found),
         * calls the function that the entry refers to (which acts as a setter).
         */
        static int newindex( lua_State *const L ) {
            // -3 is the userdata, -2 is the key (name of the member), -1 is the value
            const char *const key = lua_tostring( L, -2 );
            if( key == nullptr ) {
                throw std::runtime_error( "Invalid input to __newindex: key is not a string." );
            }
            T &instance = const_cast<T&>( get( L, 1 ) );
            return set_member( L, instance, key );
        }
        /**
         * This loads the metatable (and adds the available functions) and pushes it on the stack.
         */
        static void get_metatable( lua_State *const L ) {
            // Create table (if it does not already exist), pushes it on the stack.
            // If the table already exists, we have already filled it, so we can return
            // without doing it again.
            if( luaL_newmetatable( L, METATABLE_NAME ) == 0 ) {
                return;
            }
            // Push the metatable itself, the stack now contains two pointers to the same metatable
            lua_pushvalue( L, -1 );
            // Set the metatable of the new metatable (-2 on the stack) to be itself. Pretty meta, hu?
            // It also pops one value from the stack.
            lua_setmetatable( L, -2 );
            // Now set the actual functions of the metatable.
            luaL_setfuncs( L, &FUNCTIONS[0], 0 );

            // Push function pointer
            lua_pushcfunction( L, &catch_exception_for_lua_wrapper<&LuaValue<T>::gc> );
            // -1 would be the function pointer, -2 is the metatable, the function pointer is popped
            lua_setfield( L, -2, "__gc" );
            lua_pushcfunction( L, &catch_exception_for_lua_wrapper<&LuaValue<T>::index> );
            lua_setfield( L, -2, "__index" );
            lua_pushcfunction( L, &catch_exception_for_lua_wrapper<&LuaValue<T>::newindex> );
            lua_setfield( L, -2, "__newindex" );
        }
        /**
         * Checks the metatable that of value at stack_index against the metatable of this
         * object (matching type T). Returns the stack in the same state as it was when called.
         */
        static bool has_matching_metatable( lua_State *const L, int const stack_index ) {
            if( lua_getmetatable( L, stack_index ) == 0 ) {
                // value does not have a metatable, can not be valid at all.
                return false;
            }
            get_metatable( L );
            const bool is_correct_metatable = lua_rawequal( L, -1, -2 );
            lua_remove( L, -1 );
            lua_remove( L, -1 );
            return is_correct_metatable;
        }

    public:
        static void load_metatable( lua_State *const L, const char *const global_name ) {
            // Create the metatable for the first time (or just retrieve it)
            get_metatable( L );
            if( global_name == nullptr ) {
                // remove the table from stack, setglobal does this in the other branch,
                // make it here manually to leave the stack in the same state.
                lua_remove( L, -1 );
            } else {
                lua_setglobal( L, global_name );
            }
        }
        template<typename ...Args>
        static void push( lua_State *const L, Args &&... args ) {
            // Push user data,
            T *value_in_lua = static_cast<T *>( lua_newuserdata( L, sizeof( T ) ) );
            // Push metatable,
            get_metatable( L );
            // -1 is the metatable, -2 is the uservalue, the table is popped
            lua_setmetatable( L, -2 );
            // This is where the copy happens:
            new( value_in_lua ) T( std::forward<Args>( args )... );
        }
        static int push_reg( lua_State *const L, const T &value ) {
            push( L, value );
            return luah_store_in_registry( L, -1 );
        }
        static T &get( lua_State *const L, int const stack_index ) {
            luaL_checktype( L, stack_index, LUA_TUSERDATA );
            T *user_data = static_cast<T *>( lua_touserdata( L, stack_index ) );
            if( user_data == nullptr ) {
                throw std::runtime_error( "value is not a user defined type" );
            }
            if( has_matching_metatable( L, stack_index ) ) {
                return *user_data;
            }
            T *const subobject = get_subclass( L, stack_index );
            if( subobject == nullptr ) {
                throw std::runtime_error( "value is of incompatible type" );
            }
            return *subobject;
        }
        /** Checks whether the value at stack_index is of the type T. If so, @ref get can be used to get it. */
        static bool has( lua_State *const L, int const stack_index ) {
            if( !lua_isuserdata( L, stack_index ) ) {
                return false;
            }
            if( has_matching_metatable( L, stack_index ) ) {
                return true;
            }
            return get_subclass( L, stack_index ) != nullptr;
        }
};

template<typename T>
class LuaPointer
{
    private:
        static T *get_subclass( lua_State *S, int stack_index );

        static int index( lua_State *const L ) {
            const char *const key = lua_tostring( L, -1 );
            if( key == nullptr ) {
                throw std::runtime_error( "Invalid input to __index: key is not a string." );
            }
            if( luaL_getmetafield( L, -2, key ) != 0 ) {
                lua_remove( L, -3 );
                lua_remove( L, -2 );
                return 1;
            }
            const T *const instance = get( L, 1 );
            if( !instance ) {
                throw std::runtime_error( "nil given to __index" );
            }
            return LuaValue<T>::get_member( L, *instance, key );
        }
        static int newindex( lua_State *const L ) {
            const char *const key = lua_tostring( L, -2 );
            if( key == nullptr ) {
                throw std::runtime_error( "Invalid input to __newindex: key is not a string." );
            }
            T *const instance = get( L, 1 );
            if( !instance ) {
                throw std::runtime_error( "nil given to __newindex" );
            }
            return LuaValue<T>::set_member( L, *instance, key );
        }
        static void get_metatable( lua_State *const L ) {
            static const std::string metatable_name = LuaValue<T>::METATABLE_NAME + std::string( "*" );
            if( luaL_newmetatable( L, metatable_name.c_str() ) == 0 ) {
                return;
            }
            lua_pushvalue( L, -1 );
            lua_setmetatable( L, -2 );
            luaL_setfuncs( L, &LuaValue<T>::FUNCTIONS[0], 0 );
            lua_pushcfunction( L, &catch_exception_for_lua_wrapper<&LuaPointer<T>::index> );
            lua_setfield( L, -2, "__index" );
            lua_pushcfunction( L, &catch_exception_for_lua_wrapper<&LuaPointer<T>::newindex> );
            lua_setfield( L, -2, "__newindex" );
        }
        static bool has_matching_metatable( lua_State *const L, int const stack_index ) {
            if( lua_getmetatable( L, stack_index ) == 0 ) {
                return false;
            }
            get_metatable( L );
            const bool is_correct_metatable = lua_rawequal( L, -1, -2 );
            lua_remove( L, -1 );
            lua_remove( L, -1 );
            return is_correct_metatable;
        }

    public:
        static void push( lua_State *const L, const T *const value ) {
            if( value ) {
                push( L, *value );
            } else {
                lua_pushnil( L );
            }
        }
        static void push( lua_State *const L, const T &value ) {
            T **value_in_lua = static_cast<T **>( lua_newuserdata( L, sizeof( T* ) ) );
            get_metatable( L );
            lua_setmetatable( L, -2 );
            *value_in_lua = const_cast<T*>( &value );
        }
        static int push_reg( lua_State *const L, T *const value ) {
            push( L, value );
            return luah_store_in_registry( L, -1 );
        }
        static T *get( lua_State *const L, int const stack_index ) {
            if( lua_isnil( L, stack_index ) ) {
                return nullptr;
            }
            luaL_checktype( L, stack_index, LUA_TUSERDATA );
            T **user_data = static_cast<T **>( lua_touserdata( L, stack_index ) );
            if( user_data == nullptr ) {
                throw std::runtime_error( "value is not a user defined type" );
            }
            if( has_matching_metatable( L, stack_index ) ) {
                return *user_data;
            }
            T *const subobject = get_subclass( L, stack_index );
            if( subobject == nullptr ) {
                throw std::runtime_error( "value is of incompatible type" );
            }
            return subobject;
        }
        /** Checks whether the value at stack_index is of the type T. If so, @ref get can be used to get it. */
        static bool has( lua_State *const L, int const stack_index ) {
            if( !lua_isuserdata( L, stack_index ) ) {
                return false;
            }
            if( has_matching_metatable( L, stack_index ) ) {
                return true;
            }
            return get_subclass( L, stack_index ) != nullptr;
        }
};

#endif
