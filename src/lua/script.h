#pragma once
#ifndef SCRIPT_H
#define SCRIPT_H

#include "call.h"

#include <string>

class lua_engine;

namespace catalua
{

template<typename value_type, typename ...Args>
class script
{
    private:
        std::string script_;

    public:
        script() = default;
        script( const std::string &s ) : script_( s ) { }

        value_type operator()( const lua_engine &engine, Args ... args ) const {
            return call<value_type>( engine, script_, std::forward<Args>( args )... );
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
            *this = s.get_string();
        }
};

} // namespace catalua

#endif
