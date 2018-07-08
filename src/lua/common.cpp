#include "common.h"

#include "debug.h"

extern "C" {
#include <lauxlib.h>
}

#include <stdexcept>

int luah_store_in_registry( lua_State *L, int stackpos )
{
    lua_pushvalue( L, stackpos );
    return luaL_ref( L, LUA_REGISTRYINDEX );
}

std::string lua_tostring_wrapper( lua_State *const L, int const index )
{
    size_t length = 0;
    const char *const result = lua_tolstring( L, index, &length );
    if( result == nullptr || length == 0 ) {
        return std::string{};
    }
    return std::string( result, length );
}

void luah_remove_from_registry( lua_State *const L, const int index )
{
    lua_rawgeti( L, LUA_REGISTRYINDEX, index );
    luaL_unref( L, LUA_REGISTRYINDEX, index );
}

void luah_setmetatable( lua_State *const L, const char *const metatable_name )
{
    // Push the metatable on top of the stack.
    lua_getglobal( L, metatable_name );

    // The element we want to set the metatable for is now below the top.
    lua_setmetatable( L, -2 );
}

void luah_setglobal( lua_State *const L, const char *const name, const int index )
{
    lua_pushvalue( L, index );
    lua_setglobal( L, name );
}

int catch_exception_for_lua( int( *func )( lua_State * ), lua_State *const L )
{
    static std::string extramsg;
    int narg;
    int type = 0;
    try {
        return func( L );
    } catch( const catalua::argerror &err ) {
        extramsg = err.extramsg;
        narg = err.narg;
        type = 1;
    } catch( const std::exception &err ) {
        lua_pushstring( L, err.what() );
        // Don't call lua_error here, as it does a longjmp and skips the
        // destruction of the exception.
    }
    if( type == 1 ) {
        return luaL_argerror( L, narg, extramsg.c_str() );
    }
    return lua_error( L );
}
