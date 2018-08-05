#include "script_reference.h"

#include "call.h"
#include "lua_engine.h"
#include "game.h"
#include "json.h"

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

catalua::script_reference::script_reference( JsonIn &jsin )
{
    const lua_engine &engine = *g->lua_engine_ptr;
    lua_State *const L = get_lua_state( engine );
    if( jsin.test_string() ) {
        const std::string script = jsin.get_string();
        const int err = luaL_loadstring( L, script.c_str() );
        engine.throw_upon_lua_error( err, script.c_str() );
    } else if( jsin.test_array() ) {
        std::string script;
        JsonArray jarr = jsin.get_array();
        while( jarr.has_more() ) {
            script.append( jarr.next_string() );
            script.append( "\n" );
        }
        const int err = luaL_loadstring( L, script.c_str() );
        engine.throw_upon_lua_error( err, script.c_str() );
    } else {
        JsonObject jobj = jsin.get_object();
        const std::string path = jobj.get_string( "file" );
        const int err = luaL_loadfile( L, path.c_str() );
        engine.throw_upon_lua_error( err, path.c_str() );
    }
    id_ = luaL_ref( L, LUA_REGISTRYINDEX );
}

void catalua::stack::push_script( const lua_engine &engine, const script_reference &script )
{
    lua_State *const L = get_lua_state( engine );
    lua_rawgeti( L, LUA_REGISTRYINDEX, script.id() );
}
