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
#include "string_input_popup.h"
#include "monster.h"
#include "iuse.h"
#include "monstergenerator.h"
#include "output.h"
#include "messages.h"
#include "map.h"
#include "item.h"
#include "item_factory.h"
#include "mapgen.h"
#include "debug.h"
#include "ui.h"
#include "action.h"

// @todo get rid of those above^^

#include "call.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdexcept>

#if LUA_VERSION_NUM < 502
#define LUA_OK 0
#endif

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


// Removes item from registry and pushes on the top of stack.
void luah_remove_from_registry(lua_State *L, int item_index)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, item_index);
    luaL_unref(L, LUA_REGISTRYINDEX, item_index);
}

// Sets the metatable for the element on top of the stack.
void luah_setmetatable(lua_State *L, const char *metatable_name)
{
    // Push the metatable on top of the stack.
    lua_getglobal(L, metatable_name);

    // The element we want to set the metatable for is now below the top.
    lua_setmetatable(L, -2);
}

void luah_setglobal(lua_State *L, const char *name, int index)
{
    lua_pushvalue(L, index);
    lua_setglobal(L, name);
}

// Given a Lua return code and a file that it happened in, print a debugmsg with the error and path.
// Returns true if there was an error, false if there was no error at all.
bool lua_report_error(lua_State *L, int err, const char *path, bool simple = false ) {
    std::ostream &error_stream = g->lua_engine_ptr->error_stream;
    if( err == LUA_OK || err == LUA_ERRRUN ) {
        // No error or error message already shown via traceback function.
        return err != LUA_OK;
    }
    const std::string error = lua_tostring_wrapper( L, -1 );
    switch(err) {
        case LUA_ERRSYNTAX:
            if( !simple ) {
                error_stream << "Lua returned syntax error for "  << path  << std::endl;
            }
            error_stream << error;
            break;
        case LUA_ERRMEM:
            error_stream << "Lua is out of memory";
            break;
        case LUA_ERRFILE:
            if( !simple ) {
                error_stream << "Lua returned file io error for " << path << std::endl;
            }
            error_stream << error;
            break;
        default:
            if( !simple ) {
                error_stream << string_format( "Lua returned unknown error %d for ", err) << path << std::endl;
            }
            error_stream << error;
            break;
    }
    return true;
}

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
        lua_State * const L = g->lua_engine_ptr->state;

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

    static lua_State *state( const lua_engine &e ) {
        return e.state;
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
int lua_engine::call( const std::string &tocall )
{
    try {
        catalua::call<void>( *this, tocall );
        return 0;
    } catch( ... ) {
        //@todo handle this
        return -1;
    }
}

void lua_engine::callback( const char *const callback_name )
{
    call(std::string("mod_callback(\"") + std::string(callback_name) + "\")");
}

int lua_engine::mapgen(map *m, const oter_id &terrain_type, const mapgendata &, const time_point &t, float, const std::string &scr)
{
    if( !state ) {
        return 0;
    }
    lua_State *L = state;
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

static int global_create_uimenu( lua_State *L )
{
    static std::unique_ptr<uimenu> uimenu_instance;
    uimenu_instance.reset( new uimenu() );
    LuaReference<uimenu>::push( L, uimenu_instance.get() );
    return 1; // 1 return values
}
static int global_add_item_to_group(lua_State *L) {
    LuaType<std::string>::check(L, 1);
    auto && parameter1 = LuaType<std::string>::get(L, 1);
    LuaType<std::string>::check(L, 2);
    auto && parameter2 = LuaType<std::string>::get(L, 2);
    LuaType<int>::check(L, 3);
    auto && parameter3 = LuaType<int>::get(L, 3);
    LuaType<bool>::push(L, item_controller->add_item_to_group(parameter1, parameter2, parameter3));
    return 1; // 1 return values
}
static int global_string_input_popup(lua_State *L) {
    LuaType<std::string>::check(L, 1);
    auto && parameter1 = LuaType<std::string>::get(L, 1);
    LuaType<int>::check(L, 2);
    auto && parameter2 = LuaType<int>::get(L, 2);
    LuaType<std::string>::check(L, 3);
    auto && parameter3 = LuaType<std::string>::get(L, 3);
    const std::string result = string_input_popup().title(parameter1).width(parameter2).description(parameter3).query_string();
    LuaType<std::string>::push(L, result);
    return 1; // 1 return values
}
static int global_rng(lua_State *L) {
    LuaType<int>::check(L, 1);
    auto && parameter1 = LuaType<int>::get(L, 1);
    LuaType<int>::check(L, 2);
    auto && parameter2 = LuaType<int>::get(L, 2);
    LuaType<int>::push(L, rng(parameter1, parameter2));
    return 1; // 1 return values
}
static int global_trig_dist(lua_State *L) {
    LuaType<int>::check(L, 1);
    auto && parameter1 = LuaType<int>::get(L, 1);
    LuaType<int>::check(L, 2);
    auto && parameter2 = LuaType<int>::get(L, 2);
    LuaType<int>::check(L, 3);
    auto && parameter3 = LuaType<int>::get(L, 3);
    LuaType<int>::check(L, 4);
    auto && parameter4 = LuaType<int>::get(L, 4);
    LuaType<int>::push(L, trig_dist(parameter1, parameter2, parameter3, parameter4));
    return 1; // 1 return values
}
static int global_get_calendar_turn(lua_State *L) {
    LuaReference<calendar>::push(L, calendar::turn);
    return 1; // 1 return values
}
static int global_popup(lua_State *L) {
    LuaType<std::string>::check(L, 1);
    auto && parameter1 = LuaType<std::string>::get(L, 1);
    popup( "%s", parameter1.c_str() );
    return 0; // 0 return values
}
static int global_get_terrain_type(lua_State *L) {
    LuaType<int>::check(L, 1);
    auto && parameter1 = LuaType<int>::get(L, 1);
    LuaReference<ter_t>::push(L, ter_id( parameter1 ).obj());
    return 1; // 1 return values
}
static int global_create_monster(lua_State *L) {
    LuaValue<mtype_id>::check(L, 1);
    auto && parameter1 = LuaValue<mtype_id>::get(L, 1);
    LuaValue<tripoint>::check(L, 2);
    auto && parameter2 = LuaValue<tripoint>::get(L, 2);

    monster new_monster( parameter1, parameter2 );
    if(!g->add_zombie(new_monster)) {
        lua_pushnil( L );
    } else {
        LuaReference<monster>::push(L, g->critter_at<monster>( parameter2 ) );
    }
    return 1; // 1 return values
}
static int global_distance(lua_State *L) {
    LuaType<int>::check(L, 1);
    auto && parameter1 = LuaType<int>::get(L, 1);
    LuaType<int>::check(L, 2);
    auto && parameter2 = LuaType<int>::get(L, 2);
    LuaType<int>::check(L, 3);
    auto && parameter3 = LuaType<int>::get(L, 3);
    LuaType<int>::check(L, 4);
    auto && parameter4 = LuaType<int>::get(L, 4);
    LuaType<int>::push(L, rl_dist(parameter1, parameter2, parameter3, parameter4));
    return 1; // 1 return values
}
static int global_one_in(lua_State *L) {
    LuaType<int>::check(L, 1);
    auto && parameter1 = LuaType<int>::get(L, 1);
    LuaType<bool>::push(L, one_in(parameter1));
    return 1; // 1 return values
}
static int global_add_msg(lua_State *L) {
    LuaType<std::string>::check(L, 1);
    auto && parameter1 = LuaType<std::string>::get(L, 1);
    add_msg( "%s", parameter1 );
    return 0; // 0 return values
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

// Load the main file of a mod
void lua_engine::loadmod( const std::string &base_path, const std::string &main_file_name )
{
    std::string full_path = base_path + "/" + main_file_name;
    if( file_exist( full_path ) ) {
        lua_file_path = base_path;
        lua_dofile( state, full_path.c_str() );
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

    std::string full_path = g->lua_engine_ptr->lua_file_path + "/" + path;
    lua_dofile(L, full_path.c_str());
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
    {NULL, NULL}
};
static const struct luaL_Reg gamelib [] = {
    {"add_msg", global_add_msg},
    {"popup", global_popup},
    {"distance", global_distance},
    {"create_uimenu", global_create_uimenu},
    {"string_input_popup", global_string_input_popup},
    {"one_in", global_one_in},
    {"get_calendar_turn", global_get_calendar_turn},
    {"create_monster", global_create_monster},
    {"add_item_to_group", global_add_item_to_group},
    {"trig_dist", global_trig_dist},
    {"get_terrain_type", global_get_terrain_type},
    {"rng", global_rng},
    {NULL, NULL}
};

lua_engine::lua_engine() : state( nullptr ) {
}

void load_metatables( lua_State * );

void lua_engine::init() {
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

    luaL_openlibs(state); // Load standard lua libs

    // Load our custom "game" module
#if LUA_VERSION_NUM < 502
    luaL_register(state, "game", gamelib);
    luaL_register(state, "game", global_funcs);
#else
    std::vector<luaL_Reg> lib_funcs;
    for( auto x = gamelib; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    for( auto x = global_funcs; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    lib_funcs.push_back( luaL_Reg { NULL, NULL } );
    luaL_newmetatable(state, "game");
    lua_pushvalue(state, -1);
    luaL_setfuncs(state, &lib_funcs.front(), 0);
    lua_setglobal(state, "game");
#endif

    load_metatables( state );
    LuaEnum<body_part>::export_global( state, "body_part" );

    // override default print to our version
    lua_register( state, "print", game_myPrint );

    // Load lua-side metatables etc.
    lua_dofile(state, FILENAMES["class_defslua"].c_str());
    lua_dofile(state, FILENAMES["autoexeclua"].c_str());

    void test_lua_scripting( const lua_engine & );
    test_lua_scripting( *this );
}

lua_engine::~lua_engine() {
    if( state ) {
        lua_close( state );
    }
}

//@todo move all into the namespace
lua_State *get_lua_state( const lua_engine &e ) {
    return lua_iuse_wrapper::state( e );
}

void catalua::push_script( const lua_engine &engine, const std::string &script )
{
    lua_State *const L = get_lua_state( engine );
    const int err = luaL_loadstring( L, script.c_str() );
    if( lua_report_error( L, err, script.c_str() ) ) {
        throw std::runtime_error( "failed to load script" );
    }
}

void catalua::call_void_function( const lua_engine &engine, const int args )
{
    lua_State *const L = get_lua_state( engine );
    update_globals( L );
    const int err = lua_pcall( L, args, 0, 0 );
    if( lua_report_error( L, err, "inline script" ) ) {
        throw std::runtime_error( "failed to call function" );
    }
}

void catalua::call_non_void_function( const lua_engine &engine, const int args )
{
    lua_State *const L = get_lua_state( engine );
    update_globals( L );
    const int err = lua_pcall( L, args, 1, 0 );
    if( lua_report_error( L, err, "inline script" ) ) {
        throw std::runtime_error( "failed to call function" );
    }
}
