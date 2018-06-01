#include "catalua.h"

#include <memory>

#include "game.h"
#include "player.h"
#include "action.h"
#include "item_factory.h"
#include "item.h"
#include "pldata.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "map.h"
#include "output.h"
#include "string_formatter.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "messages.h"
#include "debug.h"
#include "translations.h"
#include "line.h"
#include "requirements.h"
#include "weather_gen.h"

#ifdef LUA
#include "ui.h"
#include "mongroup.h"
#include "itype.h"
#include "morale_types.h"
#include "trap.h"
#include "overmap.h"
#include "gun_mode.h"
#include "mapdata.h"
#include "mtype.h"
#include "field.h"
#include "filesystem.h"
#include "string_input_popup.h"
#include "mutation.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <type_traits>

using item_stack_iterator = std::list<item>::iterator;
using volume = units::volume;
using mass = units::mass;
using npc_template_id = string_id<npc_template>;


// Not used in the C++ code, but implicitly required by the Lua bindings.
// Gun modes need to be created via an actual item.
template<>
const gun_mode &string_id<gun_mode>::obj() const
{
    static const gun_mode dummy{};
    return dummy;
}
template<>
bool string_id<gun_mode>::is_valid() const
{
    return false;
}

/* Empty functions for builds without Lua: */
int lua_monster_move( monster * )
{
    return 0;
}
int call_lua( std::string ) {
    popup( _( "This binary was not compiled with Lua support." ) );
    return 0;
}
// Implemented in mapgen.cpp:
// int lua_mapgen( map *, std::string, mapgendata, int, float, const std::string & )
void lua_callback( const char * )
{
}
void lua_loadmod( std::string, std::string )
{
}
void game::init_lua()
{
}
#endif
