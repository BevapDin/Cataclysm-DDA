#include "lua_engine.h"

#include "filesystem.h"
#include "debug.h"
#include "output.h"
#include "call.h"

#include <stdexcept>

lua_engine::lua_engine() : state( nullptr )
{
}

lua_engine::~lua_engine()
{
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

void lua_engine::run_console()
{
    popup( _( "This build does not support Lua." ) );
}

bool lua_engine::enabled() const
{
    return false;
}

void catalua::stack::push_value( const lua_engine &/*engine*/, const char */*value*/ )
{
}

void catalua::stack::push_value( const lua_engine &/*engine*/, const std::string &/*value*/ )
{
}

void catalua::stack::push_value( const lua_engine &/*engine*/, bool /*value*/ )
{
}

void catalua::stack::push_integer( const lua_engine &/*engine*/, long long int /*value*/ )
{
}

void catalua::stack::push_float( const lua_engine &/*engine*/, long double /*value*/ )
{
}

std::string catalua::stack::get_string( const lua_engine &/*engine*/, int /*index*/ )
{
    throw std::runtime_error( "Tried to get a value from Lua, but Lua is not build into this executable" );
}

bool catalua::stack::get_bool( const lua_engine &/*engine*/, int /*index*/ )
{
    throw std::runtime_error( "Tried to get a value from Lua, but Lua is not build into this executable" );
}

long long int catalua::stack::get_integer( const lua_engine &/*engine*/, int /*index*/ )
{
    throw std::runtime_error( "Tried to get a value from Lua, but Lua is not build into this executable" );
}

long double catalua::stack::get_float( const lua_engine &/*engine*/, int /*index*/ )
{
    throw std::runtime_error( "Tried to get a value from Lua, but Lua is not build into this executable" );
}

void catalua::stack::push_script( const lua_engine &/*engine*/, const std::string &/*script*/ )
{
    throw std::runtime_error( "Tried to call a Lua script, but Lua is not build into this executable" );
}

void catalua::stack::call_void_function( const lua_engine &/*engine*/, int /*args*/ )
{
    throw std::runtime_error( "Tried to run a Lua function, but Lua is not build into this executable" );
}

void catalua::stack::call_non_void_function( const lua_engine &/*engine*/, int /*args*/ )
{
    throw std::runtime_error( "Tried to run a Lua function, but Lua is not build into this executable" );
}
