#include "console.h"

#include "lua_engine.h"
#include "catacharset.h"
#include "input.h"
#include "string_input_popup.h"

#include <map>

using namespace catalua;

void lua_engine::run_console()
{
    console c( *this );
    c.run();
}

console::console( const lua_engine &e ) : engine( e ), cWin( catacurses::newwin( lines, width, 0,
            0 ) ), iWin( catacurses::newwin( 1, width, lines, 0 ) )
{
    text_stack.push_back( {_( "Welcome to the Lua console! Here you can enter Lua code." ), c_green} );
    text_stack.push_back( {_( "Press [Esc] to close the Lua console." ), c_blue} );
}

console::~console() = default;

std::string console::get_input()
{
    std::map<long, std::function<bool()>> callbacks {
        {
            KEY_ESCAPE, [this]()
            {
                this->quit();
                return false;
            }
        },
        {
            KEY_NPAGE, [this]()
            {
                this->scroll_up();
                return false;
            }
        },
        {
            KEY_PPAGE, [this]()
            {
                this->scroll_down();
                return false;
            }
        } };
    string_input_popup popup;
    popup.window( iWin, 0, 0, width )
    .max_length( width )
    .identifier( "LUA" );
    popup.callbacks = callbacks;
    popup.query();
    return popup.text();
}

void console::draw()
{
    werase( cWin );

    // Some juggling to make sure text is aligned with the bottom of the console.
    int stack_size = text_stack.size() - scroll;
    for( int i = lines; i > lines - stack_size && i >= 0; i-- ) {
        auto line = text_stack[stack_size - 1 - ( lines - i )];
        mvwprintz( cWin, i - 1, 0, line.second, line.first );
    }

    wrefresh( cWin );
}

void console::quit()
{
    done = true;
}

void console::scroll_down()
{
    scroll = std::min( std::max( ( static_cast<int>( text_stack.size() ) ) - lines, 0 ), scroll + 1 );
    draw();
}

void console::scroll_up()
{
    scroll = std::max( 0, scroll - 1 );
    draw();
}

void console::read_stream( std::stringstream &stream, nc_color text_color )
{
    std::string line;
    while( std::getline( stream, line ) ) {
        for( auto str : foldstring( line, width ) ) {
            text_stack.push_back( {str, text_color} );
        }
    }
    stream.str( std::string() ); // empty the buffer
    stream.clear();
}

void console::run()
{
    while( !done ) {
        draw();

        std::string input = get_input();

        engine.call( input );

        read_stream( const_cast<std::stringstream&>( engine.output_stream ), c_white );
        read_stream( const_cast<std::stringstream&>( engine.error_stream ), c_red );
    }
}
