#include "terrain_window.h"

#include "player.h"
#include "game.h"
#include "map_iterator.h"

#include <algorithm>
#include <cassert>

#ifdef TILES
extern std::unique_ptr<cata_tiles> tilecontext;
#endif

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
    return tripoint( screen_coord.x - getmaxx( w ) / 2 + center_.x, screen_coord.y - getmaxy( w ) / 2 + center_.y, center_.z );
}

terrain_window::screen_coord terrain_window::to_screen_coord( const map_coord &pos ) const
{
    return screen_coord( getmaxx( w ) / 2 + pos.x - center_.x, getmaxy( w ) / 2 + pos.y - center_.y );
}

tripoint_range terrain_window::map_range() const
{
    return tripoint_range( to_map_coord( screen_coord( 0, 0 ) ),
                           to_map_coord( screen_coord( getmaxy( w ), getmaxx( w ) ) ) );
}

void terrain_window::center_on( const map_coord &pos )
{
    // v the component in screen coordinates
    // l the maximal value of the component in screen coordinates
    static const auto centered = []( const int v, const int l ) {
        if( v < 0 ) {
            return v;
        } else {
            return v - l + 1;
        }
    };

    const point sp = to_screen_coord( pos );
    center_.x = centered( sp.x, getmaxy( w ) );
    center_.y = centered( sp.y, getmaxx( w ) );
    center_.z = pos.z;
}

void terrain_window::scroll_into_view( const map_coord &pos )
{
    // v the component in screen coordinates
    // l the maximal value of the component in screen coordinates
    static const auto clamp = []( const int v, const int l ) {
        if( v < 0 ) {
            return v;
        } else if( v > l ) {
            return v - l + 1;
        } else {
            return 0;
        }
    };

    const point sp = to_screen_coord( pos );
    center_.x = clamp( sp.x, getmaxy( w ) );
    center_.y = clamp( sp.y, getmaxx( w ) );
    center_.z = pos.z;
}

static bool operator<( const std::unique_ptr<terrain_window_drawer> &lhs, const std::unique_ptr<terrain_window_drawer> &rhs )
{
    assert( lhs );
    assert( rhs );
    return lhs->priority() < rhs->priority();
}

terrain_window_drawer &terrain_window_drawers::insert( std::unique_ptr<terrain_window_drawer> drawer_ptr )
{
    assert( drawer_ptr );
    const auto iter = std::lower_bound( drawers.begin(), drawers.end(), drawer_ptr );
    return **drawers.insert( iter, std::move( drawer_ptr ) );
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

void terrain_window_drawers::draw( cata_tiles &tilecontext )
{
    for( const std::unique_ptr<terrain_window_drawer> &ptr : drawers ) {
        ptr->draw( tilecontext );
    }
}

void terrain_window_drawers::draw()
{
#ifdef TILES
    if( use_tiles ) {
        draw( *tilecontext );
    } else {
        draw( g->w_terrain );
    }
#else
    draw( g->w_terrain );
#endif
}
