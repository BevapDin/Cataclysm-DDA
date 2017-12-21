#include "terrain_window.h"

#include "player.h"
#include "game.h"

#include <algorithm>
#include <cassert>

bool terrain_window::contains( const map_coord &pos ) const
{
    return contains( to_screen_coord( pos ) );
}

bool terrain_window::contains( const screen_coord &pos ) const
{
    return pos.x >= 0 && pos.x < getmaxy( w ) && pos.y >= 0 && pos.y < getmaxx( w );
}

terrain_window::map_coord terrain_window::to_map_coord( const screen_coord &pos ) const
{
    return tripoint( screen_coord.x - POSX + center_.x, screen_coord.y - POSY + center_.y, center_.z );
}

terrain_window::screen_coord terrain_window::to_screen_coord( const map_coord &pos ) const
{
    return screen_coord( POSX + pos.x - center_.x, POSY + pos.y - center_.y );
}

static bool operator<( const std::unique_ptr<terrain_window_drawer> &lhs, const std::unique_ptr<terrain_window_drawer> &rhs )
{
    assert( lhs );
    assert( rhs );
    return lhs->priority() < rhs->priority();
}

void terrain_window_drawers::insert( std::unique_ptr<terrain_window_drawer> drawer_ptr )
{
    assert( drawer_ptr );
    const auto iter = std::lower_bound( drawers.begin(), drawers.end(), drawer_ptr );
    drawers.insert( iter, std::move( drawer_ptr ) );
}

void terrain_window_drawers::clear()
{
    drawers.clear();
}

void terrain_window_drawers::draw( terrain_window &win )
{
    for( const std::unique_ptr<terrain_window_drawer> &ptr : drawers ) {
        ptr->draw( win );
    }
    //@todo move into a drawer class
    // Place the cursor over the player as is expected by screen readers.
    //@todo remove the dependency on class game
    const point mp = win.to_screen_coord( g->u.pos() );
    wmove( win, mp.y, mp.x );

    wrefresh( win );
}
