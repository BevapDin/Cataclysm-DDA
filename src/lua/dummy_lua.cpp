#include "lua_engine.h"

#include "filesystem.h"
#include "debug.h"

lua_engine::lua_engine() : state( nullptr )
{
}

lua_engine::~lua_engine()
{
}

int lua_engine::call( const std::string &script )
{
    debugmsg( "Tried to call a Lua script \"%s\" in a build without Lua support.", script.substr( 0,
              1000 ) );
}

void lua_engine::callback( const char *const /*name*/ )
{
    // This behaves the same as if no mod has registered callbacks - it does nothing.
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
