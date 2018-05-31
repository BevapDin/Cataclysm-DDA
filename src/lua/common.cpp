#include "common.h"

#include <lauxlib.h>

int luah_store_in_registry( lua_State *L, int stackpos )
{
    lua_pushvalue( L, stackpos );
    return luaL_ref( L, LUA_REGISTRYINDEX );
}

std::string lua_tostring_wrapper( lua_State *const L, int const stack_position )
{
    size_t length = 0;
    const char *const result = lua_tolstring( L, stack_position, &length );
    if( result == nullptr || length == 0 ) {
        return std::string{};
    }
    return std::string( result, length );
}
