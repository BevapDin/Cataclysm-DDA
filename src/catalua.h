#pragma once
#ifndef CATALUA_H
#define CATALUA_H

#include "int_id.h"
#include "enums.h"

#include <string>
#include <sstream>
#include <list>

class Creature;
class player;
class monster;
enum body_part : int;
struct tripoint;
class item;

class CallbackArgumentContainer
{
    private:
        // counts how many things we have pushed on the stack
        int stack_size;

        // List all the types that can be pushed to Lua
        // @todo automatically generate the list?
        void push_value( const item &v );
        void push_value( double v );
        void push_value( int v );
        void push_value( bool v );
        void push_value( const tripoint &v );
        void push_value( const char *v );
        void push_value( const std::string &v ) {
            push_value( v.c_str() ); // Lua uses 0-terminated strings always and has no binary strings
        }
        void push_value( const Creature &v );
        void push_value( const player &v );
        void push_value( const monster &v );
        void push_value( body_part v );

        void push_all() {
        }
        template<typename Head, typename... Args>
        void push_all( Head &&head, Args &&... args ) {
            // if you get an error that no suitable push_value function could be found,
            // ensure the type is actually exported to Lua and add an function for it
            // (see existing implementations).
            push_value( head );
            stack_size++;
            push_all( std::forward<Args>( args )... );
        }

        void push_callback_function();

    public:
        template<typename... Args>
        CallbackArgumentContainer( const char *name, Args &&... args ) {
            push_callback_function();
            push_value( name );
            stack_size = 1; // the just pushed name of the callback
            push_all( std::forward<Args>( args )... );
        }

        CallbackArgumentContainer( const CallbackArgumentContainer & ) = delete;

        void call();
};
    
class map;
struct mapgendata;
struct oter_t;

using oter_id = int_id<oter_t>;

extern std::stringstream lua_output_stream;
extern std::stringstream lua_error_stream;

/** If this returns 0, no lua function was defined to override behavior.
 *  If this returns 1, lua behavior was called and regular behavior should be omitted.
 */
int lua_monster_move( monster *m );

/**
 * Call the given string as lua code, used for interactive debugging.
 */
int call_lua( std::string tocall );
int lua_mapgen( map *m, const oter_id &terrain_type, const mapgendata &md, int t, float d,
                const std::string &scr );

/**
 * Execute a callback that can be overridden by all mods,
 * storing provided callback arguments in _G.
 */
#ifdef LUA
template<typename... Args>
inline void lua_callback( const char *callback_name, Args &&... args )
{
    CallbackArgumentContainer arguments( callback_name, std::forward<Args>( args )... );
    arguments.call();
}
#else
template<typename... Args>
inline void lua_callback( const char */*callback_name*/, Args &&... /*args*/ ) { }
#endif

/**
 * Load the main file of a lua mod.
 *
 * @param base_path The base path of the mod.
 * @param main_file_name The file name of the lua file, usually "main.lua"
 */
void lua_loadmod( std::string base_path, std::string main_file_name );

#endif
