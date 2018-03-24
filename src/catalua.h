#pragma once
#ifndef CATALUA_H
#define CATALUA_H

#include "int_id.h"
#include "enums.h"
#include "item.h"
#include "creature.h"

#include <string>
#include <sstream>
#include <list>

enum CallbackArgumentType : int {
    Integer,
    Number,
    Double = Number,
    Float = Number,
    Boolean,
    String,
    Tripoint,
    Item,
    Reference_Creature,
    Enum_BodyPart,
};

struct CallbackArgument {
    std::string name;
    CallbackArgumentType type;

    int value_integer;
    float value_number;
    bool value_boolean;
    std::string value_string;
    tripoint value_tripoint;
    item value_item;
    Creature* value_creature;
    body_part value_body_part;

    CallbackArgument( const std::string &arg_name, int arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Integer ), value_integer( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, double arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Number ), value_number( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, float arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Number ), value_number( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, bool arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Boolean ), value_boolean( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, const std::string &arg_value ) : name( arg_name ),
        type( CallbackArgumentType::String ), value_string( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, const tripoint &arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Tripoint ), value_tripoint( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, const item &arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Item ), value_item( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, Creature* &arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Reference_Creature ), value_creature( arg_value ) {
    }
    CallbackArgument( const std::string &arg_name, const body_part &arg_value ) : name( arg_name ),
        type( CallbackArgumentType::Enum_BodyPart ), value_body_part( arg_value ) {
    }
#ifdef LUA
    void Save( int top );
#endif //LUA
};

typedef std::list<CallbackArgument> CallbackArgumentContainer;

class map;
class monster;
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
void lua_callback( const char *callback_name, const CallbackArgumentContainer &callback_args );

/**
 * Execute a callback that can be overridden by all mods.
 */
void lua_callback( const char *callback_name );

/**
 * Load the main file of a lua mod.
 *
 * @param base_path The base path of the mod.
 * @param main_file_name The file name of the lua file, usually "main.lua"
 */
void lua_loadmod( std::string base_path, std::string main_file_name );

#endif
