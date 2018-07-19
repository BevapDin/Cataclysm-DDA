#include "script_reference.h"

#include "call.h"
#include "lua_engine.h"
#include "game.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

lua_State *get_lua_state( const lua_engine &e );

catalua::script_reference::script_reference( const std::string &script )
{
    const lua_engine &engine = *g->lua_engine_ptr ;
    lua_State *const L = get_lua_state( engine );
    const int err = luaL_loadstring( L, script.c_str() );
    engine.throw_upon_lua_error( err, script.c_str() );
    id_ = luaL_ref( L, LUA_REGISTRYINDEX );
}

void catalua::stack::push_script( const lua_engine &engine, const script_reference &script )
{
    lua_State *const L = get_lua_state( engine );
    lua_rawgeti( L, LUA_REGISTRYINDEX, script.id() );
}
