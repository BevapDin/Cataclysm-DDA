#include "script.h"

#include "lua_engine.h"

#include "player.h"
#include "item.h"

#include <fstream>
#include <stdexcept>

void test_lua_scripting( const lua_engine &e )
{
    std::ofstream log( "lua.log", std::ios::app );
    try {
        catalua::script<item, player &> scr( "local pl = ... ; pl:weapon.charges = 100; return pl:weapon" );

        player pl;
        pl.weapon = item( "water" );
        const item w = scr( e, pl );
        log << "weapon: " << w.display_name() << "\n";
    } catch( const std::exception &err ) {
        log << "Error: " << err.what() << "\n";
    }
    log << "output_stream: " << e.output_stream.str() << "\n";
    log << "error_stream: " << e.error_stream.str() << "\n";
}
