#include "mission.h"
#include "game.h"
#include "overmapbuffer.h"
#include "creature_tracker.h"
#include "npc.h"

void mission_fail::kill_npc( mission *miss )
{
    if( npc *const elem = g->critter_tracker.critter_by_id<npc>( miss->get_npc_id() ) ) {
        elem->die( nullptr );
        // Actuall removoal of the npc is done in @ref Creature_tracker::remove_dead.
    }
    std::shared_ptr<npc> n = overmap_buffer.find_npc( miss->get_npc_id() );
    if( n != nullptr && !n->is_dead() ) {
        // in case the npc was not inside the reality bubble, mark it as dead anyway.
        n->marked_for_death = true;
    }
}
