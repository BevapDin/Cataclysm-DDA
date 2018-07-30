#include "lua_engine.h"

#include "filesystem.h"
#include "debug.h"

lua_engine::lua_engine() : state( nullptr )
{
}

lua_engine::~lua_engine()
{
}

void CallbackArgument::Save()
{
}

// Note: lua_engine::mapgen is implemented in mapgen.cpp

int lua_engine::call( const std::string &script )
{
    debugmsg( "Tried to call a Lua script \"%s\" in a build without Lua support.", script.substr( 0,
              1000 ) );
    return 0; // @todo
}

void lua_engine::callback( const char *const /*name*/ )
{
    // This behaves the same as if no mod has registered callbacks - it does nothing.
}

void lua_engine::callback( const char *, const CallbackArgumentContainer & )
{
    // This behaves the same as if no mod has registered callbacks - it does nothing.
}

std::string lua_engine::callback_getstring( const char *, const CallbackArgumentContainer & )
{
    return {}; // @todo
}

void lua_engine::loadmod( const std::string &base_path, const std::string &main_file_name )
{
    const std::string full_path = base_path + "/" + main_file_name;
    if( file_exist( full_path ) ) {
        debugmsg( "Tried to load Lua script \"%s\" in a build without Lua support.", full_path );
    }
}

void lua_engine::init()
{
}

int lua_engine::monster_move( monster * )
{
    return 0;
}

#endif
