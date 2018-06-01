#include "catalua.h"
#include "mapgen.h"
#include "output.h"
#include "translations.h"
#include "game.h"

int call_lua( const std::string & )
{
    popup( _( "This binary was not compiled with Lua support." ) );
    return 0;
}

// Implemented in mapgen.cpp:
// int lua_mapgen( map *, std::string, mapgendata, int, float, const std::string & )

void lua_callback( const char * )
{
}

void lua_callback( const char *, const CallbackArgumentContainer & )
{
}

void lua_loadmod( const std::string &, const std::string & )
{
}

void game::init_lua()
{
}

int lua_monster_move( monster * )
{
    return 0;
}
