#pragma once
#ifndef VALUE_H
#define VALUE_H

#include "common.h"

#include <lua.h>
#include <lauxlib.h>

#include <map>
#include <string>

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
    private:
        /** Defined by generate_bindings.lua in catabindings.cpp */
        static const char *const METATABLE_NAME;
        /** Defined by generate_bindings.lua in catabindings.cpp */
        static const luaL_Reg FUNCTIONS[];
        /** Defined by generate_bindings.lua in catabindings.cpp */
        using MRMap = std::map<std::string, int( * )( lua_State * )>;
        static const MRMap READ_MEMBERS;
        /** Defined by generate_bindings.lua in catabindings.cpp */
        using MWMap = std::map<std::string, int( * )( lua_State * )>;
        static const MWMap WRITE_MEMBERS;

        /*@{*/
        /**
         * @name Dynamic type conversion for Lua, or so.
         *
         * (Disclaimer: don't feel bad for not understanding this, I don't either.)
         *
         * get_subclass is a generated function, it checks whether the value at stack_index
         * is one of the direct subclasses of T (this check is recursive).
         * If one type matches, it returns a pointer to the object.
         *
         * Normally the function would be defined as `T* get_subclass(...)`, but if T is
         * already a pointer (e.g. Creature*), we end with `Creature** get_subclass`.
         * The problem is that a `monster**` can not be converted to `Creature**` and thereby
         * not be returned via this function (trust me, I tried, one may be able to use a
         * reinterpret_cast, but that is evil).
         * We need a simple pointer (not a pointer-to-pointer).
         *
         * We get the simple pointer by removing the pointer from T via the std thingy, which gives us
         * @ref Type. A reference to that is returned by @ref get.
         *
         * Reading user data from Lua gives a T*, so it must be converted to Type&, which may either
         * be just one dereferencing (if T*==Type*) or two (if T*==Type**).
         * One dereferencing is always done int @ref get, the (conditional) second is done in @ref cast.
         * The two overloads match either a Type* (which dereferences the parameter) or a Type&
         * (which just returns the reference without any changes).
         *
         * Maybe an example will help:
         * For T = monster* the function monster::die(Creature*) needs a Creature as parameter.
         * Try this in Lua: `game.zombie(0):die(player)`
         * Lua will check the parameter to be of type LuaValue<Creature*>, this means checking the
         * metatable (whether it's the very same as the one generated by LuaValue<Creature*>::get_metatable)
         * However, `player` has a different table (from LuaValue<player*>), so Lua would complain
         * about the wrong of the argument.
         * `LuaValue<Creature*>::get_subclass` is hard coded (via the code generator) to
         * check whether the value is actually a `Character*`, or a `monster*` (direct subclasses).
         * If so, a pointer the those objects (converted to `Creature*`) is returned. `cast()` will
         * simply pass that pointer through and the caller can return the `Creature*`.
         *
         * Now assume T = point (and assume tripoint inherit from point, why not?)
         * A function calls for a `point`, the input is a tripoint, so the metatables don't match.
         * `get_subclass` is called and successfully extract a tripoint from the userdata. It
         * returns a pointer to the tripoint (converted to `point*`). The caller needs a point (or
         * a point reference), so the `point*` must be converted back to a reference.
         * This is done by the first `cast()` overload.
         */
        using Type = typename std::remove_pointer<T>::type;
        static Type *get_subclass( lua_State *S, int stack_index );
        template<typename P>
        static Type &cast( P *ptr ) {
            return *ptr;
        }
        template<typename P>
        static Type &cast( P &ptr ) {
            return ptr;
        }
        /*@}*/

        static void call_delete( T* );
        static int gc( lua_State *const L ) {
            T *object = static_cast<T *>( lua_touserdata( L, 1 ) );
            call_delete( object );
            lua_pop( L, 1 );
            return 0;
        }
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
                luaL_error( L, "Invalid input to __index: key is not a string." );
            }
            if( luaL_getmetafield( L, -2, key ) != 0 ) {
                // There is an entry of that name, return it.
                lua_remove( L, -3 ); // remove userdata
                lua_remove( L, -2 ); // remove key
                // -1 is now the things we have gotten from luaL_getmetafield, return it.
                return 1;
            }
            const auto iter = READ_MEMBERS.find( key );
            if( iter == READ_MEMBERS.end() ) {
                // No such member or function
                lua_pushnil( L );
                return 1;
            }
            lua_remove( L, -1 ); // remove key
            // userdata is still there (now on -1, where it is expected by the getter)
            return iter->second( L );
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
                luaL_error( L, "Invalid input to __newindex: key is not a string." );
            }
            const auto iter = WRITE_MEMBERS.find( key );
            if( iter == WRITE_MEMBERS.end() ) {
                luaL_error( L, "Unknown attribute" );
            }
            lua_remove( L, -2 ); // key, userdata is still there, but now on -2, and the value is on -1
            return iter->second( L );
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
            lua_pushcfunction( L, &gc );
            // -1 would be the function pointer, -2 is the metatable, the function pointer is popped
            lua_setfield( L, -2, "__gc" );
            lua_pushcfunction( L, &index );
            lua_setfield( L, -2, "__index" );
            lua_pushcfunction( L, &newindex );
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
        static Type &get( lua_State *const L, int const stack_index ) {
            luaL_checktype( L, stack_index, LUA_TUSERDATA );
            T *user_data = static_cast<T *>( lua_touserdata( L, stack_index ) );
            if( user_data == nullptr ) {
                // luaL_error does not return at all.
                luaL_error( L, "First argument to function is not a class" );
            }
            if( has_matching_metatable( L, stack_index ) ) {
                return cast( *user_data );
            }
            Type *const subobject = get_subclass( L, stack_index );
            if( subobject == nullptr ) {
                // luaL_argerror does not return at all.
                luaL_argerror( L, stack_index, METATABLE_NAME );
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
        /** Raises a Lua error if the type of the value at stack_index is not compatible with T. */
        static void check( lua_State *const L, int const stack_index ) {
            luaL_checktype( L, stack_index, LUA_TUSERDATA );
            if( !has( L, stack_index ) ) {
                // METATABLE_NAME is used here as the name of the type we expect.
                luaL_argerror( L, stack_index, METATABLE_NAME );
            }
        }
};

#endif
