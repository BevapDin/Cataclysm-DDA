#include "glyph.h"

#include "catacharset.h"
#include "json.h"
#include "assign.h"

glyph::glyph() : symbol_( " " ), color_( c_white )
{
}

glyph::glyph( std::string s, nc_color c ) : symbol_( std::move( s ) ), color_( std::move( c ) )
{
}

// Returns whether the value was changed at all.
template<typename T>
static bool load_internal( T &value, JsonObject &jo, const std::string &name, const glyph::required_or_optional roo )
{
    if( !jo.has_member( name ) ) {
        if( roo == glyph::optional ) {
            return false;
        } else {
            jo.throw_error( "mandatory member \"" + name + "\" is missing" );
        }
    }
    return assign( jo, name, value );
}

void glyph::load_symbol( JsonObject &jo, const std::string &name, const required_or_optional roo )
{
    if( load_internal( symbol_, jo, name, roo ) ) {
        jo.throw_error( "symbol must be exactly one console cell width", name );
    }
}

void glyph::load_color( JsonObject &jo, const std::string &name, const required_or_optional roo )
{
    load_internal( color_, jo, name, roo );
}
