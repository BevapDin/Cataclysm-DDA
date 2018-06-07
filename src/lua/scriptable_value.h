#pragma once
#ifndef SCRIPTABLE_VALUE_H
#define SCRIPTABLE_VALUE_H

#include "script.h"

#include "optional.h"

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
        using script_type = script<value_type, Args>;

    private:
        //@todo should be either, but not both, add a variant class and use it
        cata::optional<value_type> value_;
        cata::optional<script_type> script_;

    public:
        scriptable_value() = default;
        scriptable_value( const value_type &v ) : value_( v ) { }
        scriptable_value( const script_type &s ) : script_( s ) { }

        value_type value( Args... args ) const {
            if( value_ ) {
                return *value_;
            }
            if( !script_ ) {
                return value_type();
            }
            const script_type &script = *script_;
            return script( std::forward<Args>( args )... );
        }
};

} // namespace catalua

#endif
