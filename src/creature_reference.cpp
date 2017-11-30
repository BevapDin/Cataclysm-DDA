#include "creature_reference.h"

#include "monster.h"
#include "npc.h"
#include "creature.h"
#include "game.h"

creature_reference::creature_reference( const player &critter )
    : creature_id( critter.getID() )
{
}

template<typename T>
T *creature_reference::get() const
{
    T *const critter = g->critter_by_id<T>( creature_id );
    if( !critter ) {
        return nullptr;
    }
    if( const auto guy = dynamic_cast<npc *>( critter ) ) {
        if( guy->is_dead() || !guy->is_active() ) {
            return nullptr;
        }
    } else if( const auto mon = dynamic_cast<monster *>( critter ) ) {
        // monsters are always active, stored in creature_tracker
        if( mon->is_dead() ) {
            return nullptr;
        }
    }
    // player character is always active and always alive
    return critter;
}
