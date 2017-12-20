#include "terrain_window.h"

#include "player.h"
#include "game.h"

#include <algorithm>

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

void terrain_window::insert( std::unique_ptr<drawer> drawer_ptr )
{
    assert( drawer_ptr );
    const auto iter = std::lower_bound( drawers.begin(), drawers.end(), [&drawer_ptr]( const std::unique_ptr<drawer> &lhs ) {
        return lhs->priority() < drawer_ptr->priority();
    } );
    drawers.insert( iter, std::move( drawer_ptr ) );
}

void terrain_window::clear()
{
    drawers.clear();
}

void terrain_window::draw()
{
    for( const std::unique_ptr<drawer> &ptr : drawers ) {
        ptr->draw( *this );
    }
    //@todo move into a drawer class
    // Place the cursor over the player as is expected by screen readers.
    //@todo remove the dependency on class game
    const point mp = to_screen_coord( g->u.pos() );
    wmove( w, mp.y, mp.x );

    wrefresh( w );
}
