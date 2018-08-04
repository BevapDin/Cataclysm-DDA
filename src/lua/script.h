#pragma once
#ifndef SCRIPT_H
#define SCRIPT_H

#include "call.h"
#include "script_reference.h"

#include "../optional.h"

#include <string>

class lua_engine;

namespace catalua
{

template<typename value_type, typename ...Args>
class script
{
    private:
        cata::optional<script_reference> internal_script_reference_;

    public:
        script() = default;
        script( const std::string &s ) : internal_script_reference_( s ) { }

        value_type operator()( const lua_engine &engine, Args ... args ) const {
            if( internal_script_reference_ ) {
                return call<value_type>( engine, *internal_script_reference_, std::forward<Args>( args )... );
            } else {
                return call<value_type>( engine, std::string(), std::forward<Args>( args )... );
            }
        }
        template<typename ...ActualArgs>
        value_type invoke_with_catch( const lua_engine &engine, ActualArgs &&... args ) const {
            try {
                return operator()( engine, std::forward<ActualArgs>( args )... );
            } catch( ... ) {
                catalua::report_exception( engine );
                return value_type();
            }
        }

        template<typename Stream>
        void deserialize( Stream &s ) {
            internal_script_reference_.emplace( s );
        }
};

} // namespace catalua

#endif
