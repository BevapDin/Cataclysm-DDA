#include "creature_tracker.h"
#include "monster.h"
#include "mongroup.h"
#include "string_formatter.h"
#include "debug.h"
#include "mtype.h"
#include "npc.h"
#include "player.h"
#include "overmapbuffer.h"

#include <algorithm>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

Creature_tracker::Creature_tracker()
: player_character( new player() )
{
}

Creature_tracker::~Creature_tracker() = default;

std::shared_ptr<monster> Creature_tracker::find( const tripoint &pos ) const
{
    const auto iter = monsters_by_location.find( pos );
    if( iter != monsters_by_location.end() ) {
        const std::shared_ptr<monster> &mon_ptr = iter->second;
        if( !mon_ptr->is_dead() ) {
            return mon_ptr;
        }
    }
    return nullptr;
}

int Creature_tracker::temporary_id( const monster &critter ) const
{
    const auto iter = std::find_if( monsters_list.begin(), monsters_list.end(),
    [&]( const std::shared_ptr<monster> &ptr ) {
        return ptr.get() == &critter;
    } );
    if( iter == monsters_list.end() ) {
        return -1;
    }
    return iter - monsters_list.begin();
}

std::shared_ptr<monster> Creature_tracker::from_temporary_id( const int id )
{
    if( static_cast<size_t>( id ) < monsters_list.size() ) {
        return monsters_list[id];
    } else {
        return nullptr;
    }
}

bool Creature_tracker::add( monster &critter )
{
    if( critter.type->id.is_null() ) { // Don't wanna spawn null monsters o.O
        return false;
    }

    if( critter.type->has_flag( MF_VERMIN ) ) {
        // Don't spawn vermin, they aren't implemented yet
        return false;
    }

    if( const std::shared_ptr<monster> existing_mon_ptr = find( critter.pos() ) ) {
        // We can spawn stuff on hallucinations, but we need to kill them first
        if( existing_mon_ptr->is_hallucination() ) {
            existing_mon_ptr->die( nullptr );
            // But don't remove - that would change the monster order and could segfault
        } else if( critter.is_hallucination() ) {
            return false;
        } else {
            debugmsg( "add_zombie: there's already a monster at %d,%d,%d",
                      critter.posx(), critter.posy(), critter.posz() );
            return false;
        }
    }

    if( MonsterGroupManager::monster_is_blacklisted( critter.type->id ) ) {
        return false;
    }

    monsters_list.emplace_back( std::make_shared<monster>( critter ) );
    monsters_by_location[critter.pos()] = monsters_list.back();
    return true;
}

size_t Creature_tracker::num_creatures() const
{
    return monsters_list.size() + active_npc.size() + 1; // 1 == player_character
}

bool Creature_tracker::update_pos( const monster &critter, const tripoint &new_pos )
{
    if( critter.is_dead() ) {
        // find ignores dead critters anyway, changing their position in the
        // monsters_by_location map is useless.
        remove_from_location_map( critter );
        return true;
    }

    if( const std::shared_ptr<monster> new_critter_ptr = find( new_pos ) ) {
        auto &othermon = *new_critter_ptr;
        if( othermon.is_hallucination() ) {
            othermon.die( nullptr );
        } else {
            debugmsg( "update_zombie_pos: wanted to move %s to %d,%d,%d, but new location already has %s",
                      critter.disp_name().c_str(),
                      new_pos.x, new_pos.y, new_pos.z, othermon.disp_name().c_str() );
            return false;
        }
    }

    const auto iter = std::find_if( monsters_list.begin(), monsters_list.end(),
    [&]( const std::shared_ptr<monster> &ptr ) {
        return ptr.get() == &critter;
    } );
    if( iter != monsters_list.end() ) {
        monsters_by_location.erase( critter.pos() );
        monsters_by_location[new_pos] = *iter;
        return true;
    } else {
        const tripoint &old_pos = critter.pos();
        // We're changing the x/y/z coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update monsters_by_location for us.
        debugmsg( "update_zombie_pos: no %s at %d,%d,%d (moving to %d,%d,%d)",
                  critter.disp_name().c_str(),
                  old_pos.x, old_pos.y, old_pos.z, new_pos.x, new_pos.y, new_pos.z );
        // Rebuild cache in case the monster actually IS in the game, just bugged
        rebuild_cache();
        return false;
    }

    return false;
}

void Creature_tracker::remove_from_location_map( const monster &critter )
{
    const tripoint &loc = critter.pos();
    const auto pos_iter = monsters_by_location.find( loc );
    if( pos_iter != monsters_by_location.end() ) {
        if( pos_iter->second.get() == &critter ) {
            monsters_by_location.erase( pos_iter );
        }
    }
}

void Creature_tracker::remove( const monster &critter )
{
    const auto iter = std::find_if( monsters_list.begin(), monsters_list.end(),
    [&]( const std::shared_ptr<monster> &ptr ) {
        return ptr.get() == &critter;
    } );
    if( iter == monsters_list.end() ) {
        debugmsg( "Tried to remove invalid monster %s", critter.name().c_str() );
        return;
    }

    remove_from_location_map( critter );
    monsters_list.erase( iter );
}

void Creature_tracker::clear_creatures()
{
    monsters_list.clear();
    monsters_by_location.clear();
    active_npc.clear();
    assert( num_creatures() == 1 ); // the player character
}

void Creature_tracker::rebuild_cache()
{
    monsters_by_location.clear();
    for( const std::shared_ptr<monster> &mon_ptr : monsters_list ) {
        monsters_by_location[mon_ptr->pos()] = mon_ptr;
    }
}

void Creature_tracker::swap_positions( monster &first, monster &second )
{
    if( first.pos() == second.pos() ) {
        return;
    }

    // Either of them may be invalid!
    const auto first_iter = monsters_by_location.find( first.pos() );
    const auto second_iter = monsters_by_location.find( second.pos() );
    // implied: first_iter != second_iter

    std::shared_ptr<monster> first_ptr;
    if( first_iter != monsters_by_location.end() ) {
        first_ptr = first_iter->second;
        monsters_by_location.erase( first_iter );
    }

    std::shared_ptr<monster> second_ptr;
    if( second_iter != monsters_by_location.end() ) {
        second_ptr = second_iter->second;
        monsters_by_location.erase( second_iter );
    }
    // implied: (first_ptr != second_ptr) or (first_ptr == nullptr && second_ptr == nullptr)

    tripoint temp = second.pos();
    second.spawn( first.pos() );
    first.spawn( temp );

    // If the pointers have been taken out of the list, put them back in.
    if( first_ptr ) {
        monsters_by_location[first.pos()] = first_ptr;
    }
    if( second_ptr ) {
        monsters_by_location[second.pos()] = second_ptr;
    }
}

bool Creature_tracker::kill_marked_for_death()
{
    // Important: `Creature::die` must not be called after creature objects (NPCs, monsters) have
    // been removed, the dying creature could still have a pointer (the killer) to another creature.
    bool creature_is_dead = false;
    for( const auto &mon_ptr : monsters_list ) {
        monster &critter = *mon_ptr;
        if( critter.is_dead() ) {
            dbg( D_INFO ) << string_format( "cleanup_dead: critter %d,%d,%d hp:%d %s",
                                            critter.posx(), critter.posy(), critter.posz(),
                                            critter.get_hp(), critter.name().c_str() );
            critter.die( nullptr );
            creature_is_dead = true;
        }
    }

    for( const std::shared_ptr<npc> &n : active_npc ) {
        npc &guy = *n;
        if( guy.is_dead() ) {
            guy.die( nullptr ); // make sure this has been called to create corpses etc.
            creature_is_dead = true;
        }
    }

    return creature_is_dead;
}

void Creature_tracker::remove_dead()
{
    // Can't use game::all_monsters() as it would not contain *dead* monsters.
    for( auto iter = monsters_list.begin(); iter != monsters_list.end(); ) {
        const monster &critter = **iter;
        if( critter.is_dead() ) {
            remove_from_location_map( critter );
            iter = monsters_list.erase( iter );
        } else {
            ++iter;
        }
    }

    for( auto it = active_npc.begin(); it != active_npc.end(); ) {
        const npc &guy = **it;
        if( guy.is_dead() ) {
            overmap_buffer.remove_npc( guy.getID() );
            it = active_npc.erase( it );
        } else {
            it++;
        }
    }
}

void Creature_tracker::despawn( Creature &critter )
{
    if( monster *const mon = dynamic_cast<monster*>( &critter ) ) {
        // hallucinations aren't stored, they come and go as they like,
        if( !mon->is_hallucination() ) {
            overmap_buffer.despawn_monster( *mon );
        }
        mon->on_unload();
        remove( *mon );
    } else if( npc *const guy = dynamic_cast<npc*>( &critter ) ) {
        guy->on_unload();
        //@todo move into remove()
        const auto iter = std::find_if( active_npc.begin(), active_npc.end(), [&guy]( const std::shared_ptr<npc> &ptr ) {
            return ptr.get() == guy;
        } );
        if( iter != active_npc.end() ) {
            active_npc.erase( iter );
        }
    }
}

void Creature_tracker::despawn_all()
{
    const auto monsters_copy = monsters_list;
    for( const auto &monster_ptr : monsters_copy ) {
        despawn( *monster_ptr );
    }
    const auto npcs_copy = active_npc;
    for( const auto &npc_ptr : npcs_copy ) {
        despawn( *npc_ptr );
    }
}

template<typename T>
T *Creature_tracker::critter_at( const tripoint &p, const bool allow_hallucination ) const
{
    if( const std::shared_ptr<monster> mon_ptr = find( p ) ) {
        if( !allow_hallucination && mon_ptr->is_hallucination() ) {
            return nullptr;
        }
        return dynamic_cast<T *>( mon_ptr.get() );
    }
    if( p == player_character->pos() ) {
        return dynamic_cast<T *>( player_character.get() );
    }
    for( auto &cur_npc : active_npc ) {
        if( cur_npc->pos() == p && !cur_npc->is_dead() ) {
            return dynamic_cast<T *>( cur_npc.get() );
        }
    }
    return nullptr;
}

template monster *Creature_tracker::critter_at<monster>( const tripoint &, bool ) const;
template npc *Creature_tracker::critter_at<npc>( const tripoint &, bool ) const;
template player *Creature_tracker::critter_at<player>( const tripoint &, bool ) const;
template Character *Creature_tracker::critter_at<Character>( const tripoint &, bool ) const;
template Creature *Creature_tracker::critter_at<Creature>( const tripoint &, bool ) const;

template<typename T>
std::shared_ptr<T> Creature_tracker::shared_from( const T &critter ) const
{
    if( const std::shared_ptr<monster> mon_ptr = find( critter.pos() ) ) {
        return std::dynamic_pointer_cast<T>( mon_ptr );
    }
    if( static_cast<const Creature*>( &critter ) == static_cast<const Creature*>( player_character.get() ) ) {
        return std::dynamic_pointer_cast<T>( player_character );
    }
    for( const auto &cur_npc : active_npc ) {
        if( static_cast<const Creature*>( cur_npc.get() ) == static_cast<const Creature*>( &critter ) ) {
            return std::dynamic_pointer_cast<T>( cur_npc );
        }
    }
    return nullptr;
}

template std::shared_ptr<Creature> Creature_tracker::shared_from<Creature>( const Creature & ) const;
template std::shared_ptr<Character> Creature_tracker::shared_from<Character>( const Character & ) const;
template std::shared_ptr<player> Creature_tracker::shared_from<player>( const player & ) const;
template std::shared_ptr<monster> Creature_tracker::shared_from<monster>( const monster & ) const;
template std::shared_ptr<npc> Creature_tracker::shared_from<npc>( const npc & ) const;

template<typename T>
T *Creature_tracker::critter_by_id( const int id ) const
{
    if( id == player_character->getID() ) {
        // player is always alive, therefor no is-dead check
        return dynamic_cast<T*>( player_character.get() );
    }
    for( const std::shared_ptr<npc> &cur_npc : active_npc ) {
        if( cur_npc->getID() == id && !cur_npc->is_dead() ) {
            return dynamic_cast<T*>( cur_npc.get() );
        }
    }
    return nullptr;
}

// monsters don't have ids
template player *Creature_tracker::critter_by_id<player>( int ) const;
template npc *Creature_tracker::critter_by_id<npc>( int ) const;
template Creature *Creature_tracker::critter_by_id<Creature>( int ) const;
