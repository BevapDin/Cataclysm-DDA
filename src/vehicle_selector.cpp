#include "vehicle_selector.h"

#include "game.h"
#include "map.h"

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible )
{
    for( const auto &e : closest_tripoints_first( radius, pos ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            if( const auto vpart = g->m.veh_part_at( e ) ) {
                data.emplace_back( vpart );
            }
        }
    }
}

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible,
                                    const vehicle &ignore )
{
    for( const auto &e : closest_tripoints_first( radius, pos ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            const auto vpart = g->m.veh_part_at( e );
            if( vpart && vpart.veh() != &ignore ) {
                data.emplace_back( vpart );
            }
        }
    }
}
