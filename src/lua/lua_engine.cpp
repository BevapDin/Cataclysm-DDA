#include "lua_engine.h"

#include "common.h"
#include "game.h"
#include "value.h"
#include "reference.h"
#include "enum.h"
#include "type.h"
#include "filesystem.h"
#include "path_info.h"

#include "rng.h"
#include "line.h"
#include "mtype.h"
#include "map.h"
#include "string_input_popup.h"
#include "monster.h"
#include "iuse.h"
#include "monstergenerator.h"
#include "output.h"
#include "messages.h"
#include "item.h"
#include "item_factory.h"
#include "debug.h"
#include "ui.h"
#include "action.h"

// @todo get rid of those above^^

#include "call.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <stdexcept>

#if LUA_VERSION_NUM < 502
#define LUA_OK 0
#endif

#if LUA_VERSION_NUM < 502
// Compatibility, for before Lua 5.2, which does not have luaL_setfuncs
static void luaL_setfuncs( lua_State *const L, const luaL_Reg arrary[], int const nup )
{
    for( ; arrary->name != nullptr; arrary++ ) {
        lua_pushstring( L, arrary->name );
        // Need to copy the up-values because lua_pushcclosure removes them, they need
        // to be set for each C-function.
        for( int i = 0; i < nup; i++ ) {
            lua_pushvalue( L, -( nup + 1 ) );
        }
        lua_pushcclosure( L, arrary->func, nup );
        lua_settable( L, -( nup + 3 ) );
    }
    // Remove up-values as per definition of luaL_setfuncs in 5.2
    lua_pop( L, nup );
}
#endif

lua_State *get_lua_state( const lua_engine &e );

template<>
int pop_from_stack<int>( const lua_engine &engine, const int index )
{
    return LuaType<int>::get( get_lua_state( engine ), index );
}

// Helper functions for making working with the lua API more straightforward.
// --------------------------------------------------------------------------

void lua_engine::throw_upon_lua_error( const int err, const char *const path ) const
{
    if( err == LUA_OK || err == LUA_ERRRUN ) {
        // No error or error message already shown via traceback function.
        return;
    }
    const char *const error_messsage = lua_tostring( state, -1 /*index @todo */ );
    const char *const error = error_messsage ? error_messsage : "";
    switch( err ) {
        case LUA_ERRSYNTAX:
            throw std::runtime_error( string_format( "Lua returned syntax error for \"%s\": %s", path, error ) );
        case LUA_ERRMEM:
            throw std::runtime_error( string_format( "Lua returned out of memory for \"%s\": %s", path, error ) );
        case LUA_ERRFILE:
            throw std::runtime_error( string_format( "Lua returned file io error for \"%s\": %s", path, error ) );
        default:
            throw std::runtime_error( string_format( "Lua returned unknown error %d for \"%s\": %s", err, path, error ) );
    }
}

class lua_iuse_wrapper : public iuse_actor
{
    private:
        int lua_function;
    public:
        lua_iuse_wrapper( const int f, const std::string &type ) : iuse_actor( type ), lua_function( f ) {}
        ~lua_iuse_wrapper() override = default;
        long use( player &, item &it, bool a, const tripoint &pos ) const override {
            // We'll be using lua_state a lot!
            lua_State *const L = g->lua_engine_ptr->state;

            // If it's a lua function, the arguments have to be wrapped in
            // lua userdata's and passed on the lua stack.
            // We will now call the function f(player, item, active)

            // Push the lua function on top of the stack
            lua_rawgeti( L, LUA_REGISTRYINDEX, lua_function );

            // TODO: also pass the player object, because of NPCs and all
            //       I guess

            const int cnt = catalua::push_onto_stack( *g->lua_engine_ptr, it, a, pos );
            catalua::call_non_void_function( *g->lua_engine_ptr, cnt );

            const int result = pop_from_stack<int>( *g->lua_engine_ptr, -1 );
            return result;
        }
        iuse_actor *clone() const override {
            return new lua_iuse_wrapper( *this );
        }

        static lua_State *state( const lua_engine &e ) {
            return e.state;
        }

        void load( JsonObject & ) override {}
};

// iuse abstraction to make iuse's both in lua and C++ possible
// ------------------------------------------------------------
void Item_factory::register_iuse_lua( const std::string &name, int lua_function )
{
    if( iuse_function_list.count( name ) > 0 ) {
        DebugLog( D_INFO, D_MAIN ) << "lua iuse function " << name << " overrides existing iuse function";
    }
    iuse_function_list[name] = use_function( new lua_iuse_wrapper( lua_function, name ) );
}

void catalua::push_mod_callback_call( const lua_engine &engine  )
{
    lua_getglobal( get_lua_state( engine ), "mod_callback" );
}

void push_value_onto_stack( const lua_engine &engine, const char *const value )
{
    lua_pushstring( get_lua_state( engine ), value );
}

void push_value_onto_stack( const lua_engine &engine, const std::string &value )
{
    lua_pushstring( get_lua_state( engine ), value.c_str() ); // @ŧodo handle embeded nul-characters
}

void push_value_onto_stack( const lua_engine &engine, const bool value )
{
    lua_pushboolean( get_lua_state( engine ), value );
}

void push_integer_onto_stack( const lua_engine &engine, long long int value )
{
    lua_pushinteger( get_lua_state( engine ), value );
}

void push_float_onto_stack( const lua_engine &engine, long double value )
{
    lua_pushnumber( get_lua_state( engine ), value );
}

// Custom functions that are to be wrapped from lua.
// -------------------------------------------------
static int global_add_item_to_group( lua_State *L )
{
    LuaType<std::string>::check( L, 1 );
    auto &&parameter1 = LuaType<std::string>::get( L, 1 );
    LuaType<std::string>::check( L, 2 );
    auto &&parameter2 = LuaType<std::string>::get( L, 2 );
    LuaType<int>::check( L, 3 );
    auto &&parameter3 = LuaType<int>::get( L, 3 );
    LuaType<bool>::push( L, item_controller->add_item_to_group( parameter1, parameter2, parameter3 ) );
    return 1; // 1 return values
}
static int global_string_input_popup( lua_State *L )
{
    LuaType<std::string>::check( L, 1 );
    auto &&parameter1 = LuaType<std::string>::get( L, 1 );
    LuaType<int>::check( L, 2 );
    auto &&parameter2 = LuaType<int>::get( L, 2 );
    LuaType<std::string>::check( L, 3 );
    auto &&parameter3 = LuaType<std::string>::get( L, 3 );
    const std::string result = string_input_popup().title( parameter1 ).width( parameter2 ).description(
                                   parameter3 ).query_string();
    LuaType<std::string>::push( L, result );
    return 1; // 1 return values
}
static int global_rng( lua_State *L )
{
    LuaType<int>::check( L, 1 );
    auto &&parameter1 = LuaType<int>::get( L, 1 );
    LuaType<int>::check( L, 2 );
    auto &&parameter2 = LuaType<int>::get( L, 2 );
    LuaType<int>::push( L, rng( parameter1, parameter2 ) );
    return 1; // 1 return values
}
static int global_trig_dist( lua_State *L )
{
    LuaType<int>::check( L, 1 );
    auto &&parameter1 = LuaType<int>::get( L, 1 );
    LuaType<int>::check( L, 2 );
    auto &&parameter2 = LuaType<int>::get( L, 2 );
    LuaType<int>::check( L, 3 );
    auto &&parameter3 = LuaType<int>::get( L, 3 );
    LuaType<int>::check( L, 4 );
    auto &&parameter4 = LuaType<int>::get( L, 4 );
    LuaType<int>::push( L, trig_dist( parameter1, parameter2, parameter3, parameter4 ) );
    return 1; // 1 return values
}
static int global_get_calendar_turn( lua_State *L )
{
    LuaReference<calendar>::push( L, calendar::turn );
    return 1; // 1 return values
}
static int global_popup( lua_State *L )
{
    LuaType<std::string>::check( L, 1 );
    auto &&parameter1 = LuaType<std::string>::get( L, 1 );
    popup( "%s", parameter1.c_str() );
    return 0; // 0 return values
}
static int global_create_monster( lua_State *L )
{
    LuaValue<mtype_id>::check( L, 1 );
    auto &&parameter1 = LuaValue<mtype_id>::get( L, 1 );
    LuaValue<tripoint>::check( L, 2 );
    auto &&parameter2 = LuaValue<tripoint>::get( L, 2 );

    monster new_monster( parameter1, parameter2 );
    if( !g->add_zombie( new_monster ) ) {
        lua_pushnil( L );
    } else {
        LuaReference<monster>::push( L, g->critter_at<monster>( parameter2 ) );
    }
    return 1; // 1 return values
}
static int global_distance( lua_State *L )
{
    LuaType<int>::check( L, 1 );
    auto &&parameter1 = LuaType<int>::get( L, 1 );
    LuaType<int>::check( L, 2 );
    auto &&parameter2 = LuaType<int>::get( L, 2 );
    LuaType<int>::check( L, 3 );
    auto &&parameter3 = LuaType<int>::get( L, 3 );
    LuaType<int>::check( L, 4 );
    auto &&parameter4 = LuaType<int>::get( L, 4 );
    LuaType<int>::push( L, rl_dist( parameter1, parameter2, parameter3, parameter4 ) );
    return 1; // 1 return values
}
static int global_one_in( lua_State *L )
{
    LuaType<int>::check( L, 1 );
    auto &&parameter1 = LuaType<int>::get( L, 1 );
    LuaType<bool>::push( L, one_in( parameter1 ) );
    return 1; // 1 return values
}
static int global_add_msg( lua_State *L )
{
    LuaType<std::string>::check( L, 1 );
    auto &&parameter1 = LuaType<std::string>::get( L, 1 );
    add_msg( "%s", parameter1 );
    return 0; // 0 return values
}
// items = game.items_at(x, y)
static int game_items_at( lua_State *L )
{
    int x = lua_tointeger( L, 1 );
    int y = lua_tointeger( L, 2 );

    auto items = g->m.i_at( x, y );
    lua_createtable( L, items.size(), 0 ); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    int i = 0;
    for( auto &an_item : items ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber( L, i++ + 1 );
        LuaReference<item>::push( L, an_item );
        lua_rawset( L, -3 );
    }

    return 1; // 1 return values
}

// item_groups = game.get_item_groups()
static int game_get_item_groups( lua_State *L )
{
    std::vector<std::string> items = item_controller->get_all_group_names();

    lua_createtable( L, items.size(), 0 ); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < items.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber( L, i + 1 );
        lua_pushstring( L, items[i].c_str() );
        lua_rawset( L, -3 );
    }

    return 1; // 1 return values
}

// monster_types = game.get_monster_types()
static int game_get_monster_types( lua_State *L )
{
    const auto mtypes = MonsterGenerator::generator().get_all_mtypes();

    lua_createtable( L, mtypes.size(), 0 ); // Preallocate enough space for all our monster types.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < mtypes.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing id
        // 2 - k, index at which the next id will be inserted
        // 3 - v, next id to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber( L, i + 1 );
        LuaValue<mtype_id>::push( L, mtypes[i].id );
        lua_rawset( L, -3 );
    }

    return 1; // 1 return values
}

// x, y = choose_adjacent(query_string, x, y)
static int game_choose_adjacent( lua_State *L )
{
    const std::string parameter1 = lua_tostring_wrapper( L, 1 );
    int parameter2 = ( int ) lua_tonumber( L, 2 );
    int parameter3 = ( int ) lua_tonumber( L, 3 );
    bool success = ( bool ) choose_adjacent( parameter1, parameter2, parameter3 );
    if( success ) {
        // parameter2 and parameter3 were updated by the call
        lua_pushnumber( L, parameter2 );
        lua_pushnumber( L, parameter3 );
        return 2; // 2 return values
    } else {
        return 0; // 0 return values
    }
}

// game.register_iuse(string, function_object)
static int game_register_iuse( lua_State *L )
{
    // Make sure the first argument is a string.
    const char *name = luaL_checkstring( L, 1 );
    if( !name ) {
        return luaL_error( L, "First argument to game.register_iuse is not a string." );
    }

    // Make sure the second argument is a function
    luaL_checktype( L, 2, LUA_TFUNCTION );

    // function_object is at the top of the stack, so we can just pop
    // it with luaL_ref
    int function_index = luaL_ref( L, LUA_REGISTRYINDEX );

    // Now register function_object with our iuse's
    item_controller->register_iuse_lua( name, function_index );

    return 0; // 0 return values
}

// Load the main file of a mod
void lua_engine::loadmod( const std::string &base_path, const std::string &main_file_name )
{
    std::string full_path = base_path + "/" + main_file_name;
    if( file_exist( full_path ) ) {
        lua_file_path = base_path;
        run_file( full_path );
        lua_file_path.clear();
    }
    // debugmsg("Loading from %s", full_path.c_str());
}

// Custom error handler
static int traceback( lua_State *L )
{
    // Get the error message
    const std::string error = lua_tostring_wrapper( L, -1 );

    // Get the lua stack trace
#if LUA_VERSION_NUM < 502
    lua_getfield( L, LUA_GLOBALSINDEX, "debug" );
    lua_getfield( L, -1, "traceback" );
#else
    lua_getglobal( L, "debug" );
    lua_getfield( L, -1, "traceback" );
    lua_remove( L, -2 );
#endif
    lua_pushvalue( L, 1 );
    lua_pushinteger( L, 2 );
    lua_call( L, 2, 1 );

    const std::string stacktrace = lua_tostring_wrapper( L, -1 );

    // Print a debug message.
    debugmsg( "Error in lua module: %s", error.c_str() );

    // Print the stack trace to our debug log.
    DebugLog( D_ERROR, DC_ALL ) << stacktrace;
    return 1;
}

void lua_engine::run_file( const std::string &path )
{
    lua_pushcfunction( state, &traceback );
    const int err = luaL_loadfile( state, path.c_str() );
    throw_upon_lua_error( err, path.c_str() );
    const int err2 = lua_pcall( state, 0, LUA_MULTRET, -2 );
    throw_upon_lua_error( err2, path.c_str() );
}

// game.dofile(file)
//
// Method to load files from lua, later should be made "safe" by
// ensuring it's being loaded from a valid path etc.
static int game_dofile( lua_State *L )
{
    const char *path = luaL_checkstring( L, 1 );
    std::string full_path = g->lua_engine_ptr->lua_file_path + "/" + path;
    g->lua_engine_ptr->run_file( full_path );
    return 0;
}

static int game_myPrint( lua_State *L )
{
    int argc = lua_gettop( L );
    for( int i = argc; i > 0; i-- ) {
        g->lua_engine_ptr->output_stream << lua_tostring_wrapper( L, -i );
    }
    g->lua_engine_ptr->output_stream << std::endl;
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
    {nullptr, nullptr}
};
static const struct luaL_Reg gamelib [] = {
    {"add_msg", global_add_msg},
    {"popup", global_popup},
    {"distance", global_distance},
    {"string_input_popup", global_string_input_popup},
    {"one_in", global_one_in},
    {"get_calendar_turn", global_get_calendar_turn},
    {"create_monster", global_create_monster},
    {"add_item_to_group", global_add_item_to_group},
    {"trig_dist", global_trig_dist},
    {"rng", global_rng},
    {nullptr, nullptr}
};

// Created by the bindings generator
void load_metatables( lua_State * );

void lua_engine::init()
{
    // This is called on each new-game, the old state (if any) is closed to dispose any data
    // introduced by mods of the previously loaded world.
    if( state ) {
        lua_close( state );
        state = nullptr;
    }
    state = luaL_newstate();
    if( !state ) {
        throw std::runtime_error( "Failed to start Lua" );
    }

    luaL_openlibs( state ); // Load standard lua libs

    // Load our custom "game" module
#if LUA_VERSION_NUM < 502
    luaL_register( state, "game", gamelib );
    luaL_register( state, "game", global_funcs );
#else
    std::vector<luaL_Reg> lib_funcs;
    for( auto x = gamelib; x->name != nullptr; ++x ) {
        lib_funcs.push_back( *x );
    }
    for( auto x = global_funcs; x->name != nullptr; ++x ) {
        lib_funcs.push_back( *x );
    }
    lib_funcs.push_back( luaL_Reg { nullptr, nullptr } );
    luaL_newmetatable( state, "game" );
    lua_pushvalue( state, -1 );
    luaL_setfuncs( state, &lib_funcs.front(), 0 );
    lua_setglobal( state, "game" );
#endif

    load_metatables( state );
    LuaEnum<body_part>::export_global( state, "body_part" );

    // override default print to our version
    lua_register( state, "print", game_myPrint );

    LuaReference<player>::push( state, g->u );
    lua_setglobal( state, "player" );

    LuaReference<map>::push( state, g->m );
    lua_setglobal( state, "map" );

    LuaReference<game>::push( state, g );
    lua_setglobal( state, "g" );

    // Load lua-side metatables etc.
    run_file( FILENAMES["class_defslua"] );
    run_file( FILENAMES["autoexeclua"] );

    void test_lua_scripting( const lua_engine & );
    test_lua_scripting( *this );
}

lua_engine::lua_engine() : state( nullptr )
{
}

lua_engine::~lua_engine()
{
    if( state ) {
        lua_close( state );
    }
}

//@todo move all into the namespace
lua_State *get_lua_state( const lua_engine &e )
{
    return lua_iuse_wrapper::state( e );
}

void catalua::push_script( const lua_engine &engine, const std::string &script )
{
    lua_State *const L = get_lua_state( engine );
    const int err = luaL_loadstring( L, script.c_str() );
    engine.throw_upon_lua_error( err, script.c_str() );
}

void catalua::call_void_function( const lua_engine &engine, const int args )
{
    lua_State *const L = get_lua_state( engine );
    const int err = lua_pcall( L, args, 0, 0 );
    engine.throw_upon_lua_error( err, "<inline script>" );
}

void catalua::call_non_void_function( const lua_engine &engine, const int args )
{
    lua_State *const L = get_lua_state( engine );
    const int err = lua_pcall( L, args, 1, 0 );
    engine.throw_upon_lua_error( err, "<inline script>" );
}
