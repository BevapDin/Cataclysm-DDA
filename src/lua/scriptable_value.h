#pragma once
#ifndef SCRIPTABLE_VALUE_H
#define SCRIPTABLE_VALUE_H

#include "script.h"

#include "../optional.h"

class lua_engine;

namespace catalua
{

/**
 * A value (of any type) that can be generated via an embedded Lua script.
 *
 * @tparam value_type The type of the value that this object represents.
 * An instance of that type must be returned by the script.
 * The class also contains an option instance of that value that is
 * returned when no script has been set.
 * @tparam Args Type of arguments that are passed forward to the script.
 */
template<typename value_type, typename ...Args>
class scriptable_value
{
    public:
        using script_type = script<value_type, Args...>;

    private:
        //@todo should be either, but not both, add a variant class and use it
        cata::optional<value_type> value_;
        cata::optional<script_type> script_;

    public:
        scriptable_value() = default;
        explicit scriptable_value( const value_type &v ) : value_( v ) { }
        scriptable_value( const script_type &s ) : script_( s ) { }

        bool has_script() const {
            return static_cast<bool>( script_ );
        }
        const value_type &actual_value() const {
            return *value_;
        }
        void set_value( const value_type &v ) {
            value_.emplace( v );
            script_.reset();
        }

        template<typename ...ActualArgs>
        value_type value( const lua_engine &engine, ActualArgs &&... args ) const {
            if( value_ ) {
                return *value_;
            }
            if( !script_ ) {
                return value_type();
            }
            return ( *script_ )( engine, std::forward<ActualArgs>( args )... );
        }
        template<typename Stream>
        void deserialize( Stream &jsin ) {
            if( jsin.test_object() ) {
                auto obj = jsin.get_object();
                if( obj.has_member( "lua-script" ) ) {
                    script_.emplace( obj.get_string( "lua-script" ) );
                    return;
                }
            }
            value_.emplace();
            jsin.read( *value_ );
            script_.reset();
        }
};

} // namespace catalua

#endif
