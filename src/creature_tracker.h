#pragma once
#ifndef CREATURE_TRACKER_H
#define CREATURE_TRACKER_H

#include "enums.h"

#include <memory>
#include <vector>
#include <unordered_map>

class player;
class Creature;
class monster;
class JsonIn;
class JsonOut;
class npc;

class Creature_tracker
{
    public:
        Creature_tracker();
        ~Creature_tracker();
        /**
         * Returns the monster at the given location.
         * If there is no monster, it returns a `nullptr`.
         * Dead monsters are ignored and not returned.
         */
        std::shared_ptr<monster> find( const tripoint &pos ) const;
        /**
         * Returns a temporary id of the given monster (which must exist in the tracker).
         * The id is valid until monsters are added or removed from the tracker.
         * The id remains valid through serializing and deserializing.
         * Use @ref from_temporary_id to get the monster pointer back. (The later may
         * return a nullptr if the given id is not valid.)
         */
        int temporary_id( const monster &critter ) const;
        std::shared_ptr<monster> from_temporary_id( int id );
        /** Adds the given monster to the creature_tracker. Returns whether the operation was successful. */
        bool add( monster &critter );
        /**
         * Returns the approximate number of creatures in the tracker.
         * Because of performance restrictions it may return a slightly incorrect
         * values as it includes dead, but not yet cleaned up creatures.
         * The count includes any type of creature (npcs, player character, monsters).
         */
        size_t num_creatures() const;
        /** Updates the position of the given monster to the given point. Returns whether the operation
         *  was successful. */
        bool update_pos( const monster &critter, const tripoint &new_pos );
        /** Removes the given monster from the Creature tracker, adjusting other entries as needed. */
        void remove( const monster &critter );
        void rebuild_cache();
        /** Swaps the positions of two monsters */
        void swap_positions( monster &first, monster &second );
        /** Kills 0 hp monsters. Returns if it killed any. */
        bool kill_marked_for_death();
        /** Removes dead monsters from. Their pointers are invalidated. */
        void remove_dead();
        /**
         * Remove a specific creature and store it on the @ref overmap. Any pointer
         * to it will become invalid.
         * Note that the creature may get respawned in the next turn when it's still
         * inside the reality bubble.
         * Note: has no effect when being called with @ref player_character.
         */
        void despawn( Creature &critter );
        /// Calls @ref despawn for all (except @ref player_character) contained creatures.
        void despawn_all();

        const std::vector<std::shared_ptr<monster>> &get_monsters_list() const {
            return monsters_list;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        std::vector<std::shared_ptr<npc>> active_npc;

        player &get_player_character() const {
            return *player_character;
        }
        /**
         * Returns the Creature at the given location @p p.
         * Optionally casted to the given type of creature: @ref npc, @ref player, @ref monster.
         * If there is a creature, but it's not of the requested type, returns `nullptr`.
         * @param allow_hallucination Whether to return monsters that are actually
         * hallucinations (@ref Creature::is_hallucination).
         */
        template<typename T = Creature>
        T *critter_at( const tripoint &p, bool allow_hallucination ) const;
        /**
         * Returns a shared pointer to the given creature (which can be of any
         * of the subclasses of @ref Creature).
         * The function may return an empty pointer if the given critter
         * is not stored anywhere (e.g. it was allocated on the stack).
         * Precondition: if the returned pointer is not `nullptr`, it points to @p critter.
         */
        template<typename T = Creature>
        std::shared_ptr<T> shared_from( const T &critter ) const;
        /**
         * Returns the living creature with the given @p id and the requested type.
         * Returns `nullptr` if no living creature with such an id exists, of it has
         * an incompatible type.
         * Never returns a dead creature.
         * Currently only the player character and npcs have ids.
         */
        template<typename T = Creature>
        T *critter_by_id( int id ) const;
        /**
         * Removes all monsters (as if by calling @ref remove) and npcs
         * from the tracker. Note that the npcs remain stored in an overmap, but monsters
         * are effectively lost.
         * Postcondition: `num_creatures() == 1` (@ref player_character).
         */
        void clear_creatures();

    private:
        std::vector<std::shared_ptr<monster>> monsters_list;
        std::unordered_map<tripoint, std::shared_ptr<monster>> monsters_by_location;
        /** Remove the monsters entry in @ref monsters_by_location */
        void remove_from_location_map( const monster &critter );

        std::shared_ptr<player> player_character;
};

#endif
