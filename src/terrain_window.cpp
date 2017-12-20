#include "terrain_window.h"

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
