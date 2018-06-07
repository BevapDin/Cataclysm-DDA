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
        catalua::script<item, std::reference_wrapper<player>> scr( "local pl = ... ; pl.weapon.charges = 100; return pl.weapon" );

        // Note: this won't compile because std::ofstream is not exported to Lua:
        //catalua::call<void>( e, "", log );
        // Sadly, this will compile, but not link, because the pop-from-stack function is a template
        //catalua::call<std::ofstream&>( e, "" );

        player pl;
        pl.weapon = item( "water" );
        const item w = scr( e, pl );
        log << "weapon: " << w.display_name() << "\n";

        // Won't work because it will attempt to push the player by-value, which is not enabled.
        //catalua::call<void>( e, "", pl );
    } catch( const std::exception &err ) {
        log << "Error: " << err.what() << "\n";
    }
    log << "output_stream: " << e.output_stream.str() << "\n";
    log << "error_stream: " << e.error_stream.str() << "\n";
}
