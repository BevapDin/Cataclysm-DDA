#pragma once
#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

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
    CallbackArgumentType type;

    int value_integer;
    float value_number;
    bool value_boolean;
    std::string value_string;
    tripoint value_tripoint;
    item value_item;
    Creature *value_creature;
    body_part value_body_part;

    CallbackArgument( int arg_value ) :
        type( CallbackArgumentType::Integer ), value_integer( arg_value ) {
    }
    CallbackArgument( double arg_value ) :
        type( CallbackArgumentType::Number ), value_number( arg_value ) {
    }
    CallbackArgument( float arg_value ) :
        type( CallbackArgumentType::Number ), value_number( arg_value ) {
    }
    CallbackArgument( bool arg_value ) :
        type( CallbackArgumentType::Boolean ), value_boolean( arg_value ) {
    }
    CallbackArgument( const std::string &arg_value ) :
        type( CallbackArgumentType::String ), value_string( arg_value ) {
    }
    CallbackArgument( const tripoint &arg_value ) :
        type( CallbackArgumentType::Tripoint ), value_tripoint( arg_value ) {
    }
    CallbackArgument( const item &arg_value ) :
        type( CallbackArgumentType::Item ), value_item( arg_value ) {
    }
    CallbackArgument( Creature *&arg_value ) :
        type( CallbackArgumentType::Reference_Creature ), value_creature( arg_value ) {
    }
    CallbackArgument( const body_part &arg_value ) :
        type( CallbackArgumentType::Enum_BodyPart ), value_body_part( arg_value ) {
    }
    void Save();
};

typedef std::list<CallbackArgument> CallbackArgumentContainer;

class map;
class monster;
class time_point;
struct mapgendata;
struct oter_t;
using oter_id = int_id<oter_t>;

//@todo hide this
class lua_iuse_wrapper;

template<typename T>
class int_id;
class map;
class time_point;
struct mapgendata;
struct oter_t;
using oter_id = int_id<oter_t>;

class lua_engine
{
    private:
        friend class lua_iuse_wrapper;

        // Wrapper for anonymous pointer so we can store
        // lua_state without having a definition of it in this header.
        class pointer_wrapper
        {
            private:
                void *ptr;
            public:
                pointer_wrapper( void *const p ): ptr( p ) { }
                template<typename T>
                operator T *() const {
                    return static_cast<T *>( ptr );
                }
                explicit operator bool() const {
                    return ptr;
                }
        } state;

    public://@todo make private
        std::string lua_file_path;
        std::stringstream output_stream;
        std::stringstream error_stream;

    public:
        lua_engine();
        lua_engine( const lua_engine & ) = delete;

        ~lua_engine();

        /// @throws when initialization fails
        void init();
        void loadmod( const std::string &base_path, const std::string &main_file_name );

        int monster_move( monster *m );

        void run_console();

        int call( const std::string &script );
        void callback( const char *name );
        void callback( const char *name, const CallbackArgumentContainer &callback_args );
        std::string callback_getstring( const char *callback_name, const CallbackArgumentContainer &callback_args );

        // This is a legacy function, ideally code would use `catalua::call` instead,
        // but for mapgen we do a bit more than just call the script.
        int mapgen( map *m, const oter_id &terrain_type, const mapgendata &, const time_point &t, float, const std::string &scr );
};

#endif
