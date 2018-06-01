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
#include "omdata.h"
#include "overmap.h"

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

#if LUA_VERSION_NUM < 502
#define LUA_OK 0
#endif

using item_stack_iterator = std::list<item>::iterator;
using volume = units::volume;
using mass = units::mass;
using npc_template_id = string_id<npc_template>;
using overmap_direction = om_direction::type;

lua_State *lua_state = nullptr;

// Keep track of the current mod from which we are executing, so that
// we know where to load files from.
std::string lua_file_path = "";

std::stringstream lua_output_stream;
std::stringstream lua_error_stream;

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

#if LUA_VERSION_NUM < 502
// Compatibility, for before Lua 5.2, which does not have luaL_setfuncs
static void luaL_setfuncs( lua_State * const L, const luaL_Reg arrary[], int const nup )
{
    for( ; arrary->name != nullptr; arrary++ ) {
        lua_pushstring( L, arrary->name );
        // Need to copy the up-values because lua_pushcclosure removes them, they need
        // to be set for each C-function.
        for( int i = 0; i < nup; i++ ) {
            lua_pushvalue( L, -(nup + 1) );
        }
        lua_pushcclosure( L, arrary->func, nup );
        lua_settable( L, -(nup + 3) );
    }
    // Remove up-values as per definition of luaL_setfuncs in 5.2
    lua_pop( L, nup );
}
#endif

void lua_dofile(lua_State *L, const char *path);

// Helper functions for making working with the lua API more straightforward.
// --------------------------------------------------------------------------

#include "lua/common.h"

// Given a Lua return code and a file that it happened in, print a debugmsg with the error and path.
// Returns true if there was an error, false if there was no error at all.
bool lua_report_error( lua_State *L, int err, const char *path, bool simple = false ) {
    if( err == LUA_OK || err == LUA_ERRRUN ) {
        // No error or error message already shown via traceback function.
        return err != LUA_OK;
    }
    const std::string error = lua_tostring_wrapper( L, -1 );
    switch(err) {
        case LUA_ERRSYNTAX:
            if( !simple ) {
                lua_error_stream << "Lua returned syntax error for "  << path  << std::endl;
            }
            lua_error_stream << error;
            break;
        case LUA_ERRMEM:
            lua_error_stream << "Lua is out of memory";
            break;
        case LUA_ERRFILE:
            if( !simple ) {
                lua_error_stream << "Lua returned file io error for " << path << std::endl;
            }
            lua_error_stream << error;
            break;
        default:
            if( !simple ) {
                lua_error_stream << string_format( "Lua returned unknown error %d for ", err) << path << std::endl;
            }
            lua_error_stream << error;
            break;
    }
    return true;
}

#include "lua/value.h"
#include "lua/reference.h"
#include "lua/type.h"
#include "lua/enum.h"

/**
 * Wrapper class to access objects in Lua that are stored as either a pointer or a value.
 * Technically, this class could inherit from both `LuaValue<T>` and `LuaReference<T>`,
 * but that would basically the same code anyway.
 * It behaves like a LuaValue if there is a value on the stack, and like LuaReference is there
 * is a reference on the stack. Functions behave like the functions in a `LuaType`.
 * Note that it does not have a push function because it can not know whether to push a reference
 * or a value (copy). The caller must decide this and must use `LuaValue` or `LuaReference`.
 */
template<typename T>
class LuaValueOrReference {
    public:
        using proxy = typename LuaReference<T>::proxy;
        static proxy get( lua_State* const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return proxy{ &LuaValue<T>::get( L, stack_index ) };
            }
            return LuaReference<T>::get( L, stack_index );
        }
        static void check( lua_State* const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return;
            }
            LuaValue<T*>::check( L, stack_index );
        }
        static bool has( lua_State* const L, int const stack_index ) {
            return LuaValue<T>::has( L, stack_index ) || LuaValue<T*>::has( L, stack_index );
        }
};

void update_globals(lua_State *L)
{
    LuaReference<player>::push( L, g->u );
    luah_setglobal( L, "player", -1 );

    LuaReference<map>::push( L, g->m );
    luah_setglobal( L, "map", -1 );

    LuaReference<game>::push( L, g );
    luah_setglobal( L, "g", -1 );
}

class lua_iuse_wrapper : public iuse_actor {
private:
    int lua_function;
public:
    lua_iuse_wrapper( const int f, const std::string &type ) : iuse_actor( type ), lua_function( f ) {}
    ~lua_iuse_wrapper() override = default;
    long use( player &, item &it, bool a, const tripoint &pos ) const override {
        // We'll be using lua_state a lot!
        lua_State * const L = lua_state;

        // If it's a lua function, the arguments have to be wrapped in
        // lua userdata's and passed on the lua stack.
        // We will now call the function f(player, item, active)

        update_globals( L );

        // Push the lua function on top of the stack
        lua_rawgeti( L, LUA_REGISTRYINDEX, lua_function );

        // TODO: also pass the player object, because of NPCs and all
        //       I guess

        // Push the item on top of the stack.
        const int item_in_registry = LuaReference<item>::push_reg( L, it );
        // Push the "active" parameter on top of the stack.
        lua_pushboolean( L, a );
        // Push the location of the item.
        const int tripoint_in_registry = LuaValue<tripoint>::push_reg( L, pos );

        // Call the iuse function
        int err = lua_pcall( L, 3, 1, 0 );
        lua_report_error( L, err, "iuse function" );

        // Make sure the now outdated parameters we passed to lua aren't
        // being used anymore by setting a metatable that will error on
        // access.
        luah_remove_from_registry( L, item_in_registry );
        luah_setmetatable( L, "outdated_metatable");
        luah_remove_from_registry( L, tripoint_in_registry );
        luah_setmetatable( L, "outdated_metatable" );

        return lua_tointeger( L, -1 );
    }
    iuse_actor *clone() const override {
        return new lua_iuse_wrapper( *this );
    }

    void load( JsonObject & ) override {}
};

// iuse abstraction to make iuse's both in lua and C++ possible
// ------------------------------------------------------------
void Item_factory::register_iuse_lua(const std::string &name, int lua_function)
{
    if( iuse_function_list.count( name ) > 0 ) {
        DebugLog(D_INFO, D_MAIN) << "lua iuse function " << name << " overrides existing iuse function";
    }
    iuse_function_list[name] = use_function( new lua_iuse_wrapper( lua_function, name ) );
}

// Call the given string directly, used in the lua debug command.
int call_lua( const std::string &tocall )
{
    lua_State *L = lua_state;

    update_globals(L);
    int err = luaL_dostring(L, tocall.c_str());
    lua_report_error(L, err, tocall.c_str(), true);
    return err;
}

void lua_callback(const char *callback_name)
{
    call_lua(std::string("mod_callback(\"") + std::string(callback_name) + "\")");
}

//
int lua_mapgen(map *m, const oter_id &terrain_type, const mapgendata &, const time_point &t, float, const std::string &scr)
{
    if( lua_state == nullptr ) {
        return 0;
    }
    lua_State *L = lua_state;
    LuaReference<map>::push( L, m );
    luah_setglobal(L, "map", -1);

    int err = luaL_loadstring(L, scr.c_str() );
    if( lua_report_error( L, err, scr.c_str() ) ) {
        return err;
    }
    //    int function_index = luaL_ref(L, LUA_REGISTRYINDEX); // @todo; make use of this
    //    lua_rawgeti(L, LUA_REGISTRYINDEX, function_index);

    lua_pushstring(L, terrain_type.id().c_str());
    lua_setglobal(L, "tertype");
    lua_pushinteger( L, to_turn<int>( t ) );
    lua_setglobal(L, "turn");

    err = lua_pcall(L, 0 , LUA_MULTRET, 0);
    lua_report_error( L, err, scr.c_str() );

    //    luah_remove_from_registry(L, function_index); // @todo: make use of this

    return err;
}

// Custom functions that are to be wrapped from lua.
// -------------------------------------------------
static calendar &get_calendar_turn_wrapper() {
    return calendar::turn;
}

static time_duration get_time_duration_wrapper( const int t )
{
    return time_duration::from_turns( t );
}

static std::string get_omt_id( const overmap &om, const tripoint &p )
{
    return om.get_ter( p ).id().str();
}

static overmap_direction get_omt_dir( const overmap &om, const tripoint &p )
{
   return om.get_ter( p ).obj().get_dir();
}

static std::string string_input_popup_wrapper( const std::string &title, int width, const std::string &desc ) {
    return string_input_popup().title(title).width(width).description(desc).query_string();
}

/** Get reference to monster at given tripoint. */
monster *get_monster_at( const tripoint &p )
{
    return g->critter_at<monster>( p );
}

/** Get reference to Creature at given tripoint. */
Creature *get_critter_at( const tripoint &p )
{
    return g->critter_at( p );
}

/** Create a new monster of the given type. */
monster *create_monster( const mtype_id &mon_type, const tripoint &p )
{
    monster new_monster( mon_type, p );
    if(!g->add_zombie(new_monster)) {
        return NULL;
    } else {
        return g->critter_at<monster>( p );
    }
}

// Manually implemented lua functions
//
// Most lua functions are generated by src/lua/generate_bindings.lua,
// these generated functions can be found in src/lua/catabindings.cpp

static void popup_wrapper(const std::string &text) {
    popup( "%s", text.c_str() );
}

static void add_msg_wrapper(const std::string &text) {
    add_msg( text );
}

static bool query_yn_wrapper(const std::string &text) {
    return query_yn( text );
}

// items = game.items_at(x, y)
static int game_items_at(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    auto items = g->m.i_at(x, y);
    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    int i = 0;
    for( auto &an_item : items ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i++ + 1);
        item **item_userdata = (item **) lua_newuserdata(L, sizeof(item *));
        *item_userdata = &an_item;
        // TODO: update using LuaReference<item>
        luah_setmetatable(L, "item_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// item_groups = game.get_item_groups()
static int game_get_item_groups(lua_State *L)
{
    std::vector<std::string> items = item_controller->get_all_group_names();

    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < items.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        lua_pushstring(L, items[i].c_str());
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// monster_types = game.get_monster_types()
static int game_get_monster_types(lua_State *L)
{
    const auto mtypes = MonsterGenerator::generator().get_all_mtypes();

    lua_createtable(L, mtypes.size(), 0); // Preallocate enough space for all our monster types.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < mtypes.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing id
        // 2 - k, index at which the next id will be inserted
        // 3 - v, next id to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        LuaValue<mtype_id>::push( L, mtypes[i].id );
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// x, y = choose_adjacent(query_string, x, y)
static int game_choose_adjacent(lua_State *L)
{
    const std::string parameter1 = lua_tostring_wrapper( L, 1 );
    int parameter2 = (int) lua_tonumber(L, 2);
    int parameter3 = (int) lua_tonumber(L, 3);
    bool success = (bool) choose_adjacent(parameter1, parameter2, parameter3);
    if(success) {
        // parameter2 and parameter3 were updated by the call
        lua_pushnumber(L, parameter2);
        lua_pushnumber(L, parameter3);
        return 2; // 2 return values
    } else {
        return 0; // 0 return values
    }
}

// game.register_iuse(string, function_object)
static int game_register_iuse(lua_State *L)
{
    // Make sure the first argument is a string.
    const char *name = luaL_checkstring(L, 1);
    if(!name) {
        return luaL_error(L, "First argument to game.register_iuse is not a string.");
    }

    // Make sure the second argument is a function
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // function_object is at the top of the stack, so we can just pop
    // it with luaL_ref
    int function_index = luaL_ref(L, LUA_REGISTRYINDEX);

    // Now register function_object with our iuse's
    item_controller->register_iuse_lua(name, function_index);

    return 0; // 0 return values
}

#include "lua/catabindings.cpp"

// Load the main file of a mod
void lua_loadmod(const std::string &base_path, const std::string &main_file_name)
{
    std::string full_path = base_path + "/" + main_file_name;
    if( file_exist( full_path ) ) {
        lua_file_path = base_path;
        lua_dofile( lua_state, full_path.c_str() );
        lua_file_path.clear();
    }
    // debugmsg("Loading from %s", full_path.c_str());
}

// Custom error handler
static int traceback(lua_State *L)
{
    // Get the error message
    const std::string error = lua_tostring_wrapper( L, -1 );

    // Get the lua stack trace
#if LUA_VERSION_NUM < 502
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    lua_getfield(L, -1, "traceback");
#else
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
#endif
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    const std::string stacktrace = lua_tostring_wrapper( L, -1 );

    // Print a debug message.
    debugmsg("Error in lua module: %s", error.c_str());

    // Print the stack trace to our debug log.
    DebugLog( D_ERROR, DC_ALL ) << stacktrace;
    return 1;
}

// Load an arbitrary lua file
void lua_dofile(lua_State *L, const char *path)
{
    lua_pushcfunction(L, &traceback);
    int err = luaL_loadfile(L, path);
    if( lua_report_error( L, err, path ) ) {
        return;
    }
    err = lua_pcall(L, 0, LUA_MULTRET, -2);
    lua_report_error( L, err, path );
}

// game.dofile(file)
//
// Method to load files from lua, later should be made "safe" by
// ensuring it's being loaded from a valid path etc.
static int game_dofile(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    std::string full_path = lua_file_path + "/" + path;
    lua_dofile(L, full_path.c_str());
    return 0;
}

static int game_myPrint( lua_State *L )
{
    int argc = lua_gettop( L );
    for( int i = argc; i > 0; i-- ) {
        lua_output_stream << lua_tostring_wrapper( L, -i );
    }
    lua_output_stream << std::endl;
    return 0;
}

// Registry containing all the game functions exported to lua.
// -----------------------------------------------------------
static const struct luaL_Reg global_funcs [] = {
    {"register_iuse", game_register_iuse},
    //{"get_monsters", game_get_monsters},
    {"items_at", game_items_at},
    {"choose_adjacent", game_choose_adjacent},
    {"dofile", game_dofile},
    {"get_monster_types", game_get_monster_types},
    {"get_item_groups", game_get_item_groups},
    {NULL, NULL}
};

// Lua initialization.
void game::init_lua()
{
    // This is called on each new-game, the old state (if any) is closed to dispose any data
    // introduced by mods of the previously loaded world.
    if( lua_state != nullptr ) {
        lua_close( lua_state );
    }
    lua_state = luaL_newstate();
    if( lua_state == nullptr ) {
        debugmsg( "Failed to start Lua. Lua scripting won't be available." );
        return;
    }

    luaL_openlibs(lua_state); // Load standard lua libs

    // Load our custom "game" module
#if LUA_VERSION_NUM < 502
    luaL_register(lua_state, "game", gamelib);
    luaL_register(lua_state, "game", global_funcs);
#else
    std::vector<luaL_Reg> lib_funcs;
    for( auto x = gamelib; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    for( auto x = global_funcs; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    lib_funcs.push_back( luaL_Reg { NULL, NULL } );
    luaL_newmetatable(lua_state, "game");
    lua_pushvalue(lua_state, -1);
    luaL_setfuncs(lua_state, &lib_funcs.front(), 0);
    lua_setglobal(lua_state, "game");
#endif

    load_metatables( lua_state );
    LuaEnum<body_part>::export_global( lua_state, "body_part" );

    // override default print to our version
    lua_register( lua_state, "print", game_myPrint );

    // Load lua-side metatables etc.
    lua_dofile(lua_state, FILENAMES["class_defslua"].c_str());
    lua_dofile(lua_state, FILENAMES["autoexeclua"].c_str());
}

#endif // #ifdef LUA

#ifndef LUA
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
void lua_loadmod( const std::string &, const std::string & )
{
}
void game::init_lua()
{
}
#endif
