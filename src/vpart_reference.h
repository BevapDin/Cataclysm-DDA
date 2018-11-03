#pragma once
#ifndef VPART_REFERENCE_H
#define VPART_REFERENCE_H

#include "vpart_position.h"

#include <string>

class player;
class vehicle;
struct vehicle_part;
class vpart_info;
enum vpart_bitflags : int;

/**
 * This is a wrapper over a vehicle pointer and a reference to a part of it.
 *
 * The class does not support an "invalid" state, it is created from a
 * valid reference and the user must ensure it's still valid when used.
 * Most functions just forward to the equally named functions in the @ref vehicle
 * class, so see there for documentation.
 */
class vpart_reference : public vpart_position
{
    public:
        vpart_reference( ::vehicle &v, const size_t part ) : vpart_position( v, part ) { }
        vpart_reference( const vpart_reference & ) = default;

        using vpart_position::vehicle;

        /// Yields the \ref vehicle_part object referenced by this. @see vehicle::parts
        vehicle_part &part() const;
        /// See @ref vehicle_part::info
        const vpart_info &info() const;
        /**
         * Returns whether the part *type* has the given feature.
         * Note that this is different from part flags (which apply to part
         * instances).
         * For example a feature is "CARGO" (the part can store items).
         */
        /**@{*/
        bool has_feature( const std::string &f ) const;
        bool has_feature( vpart_bitflags f ) const;
        /**@}*/
        /**
         * Yields the currently boarded creature.
         * Returns `nullptr` if there is no boarded creature.
         * Returns `nullptr` if the part can not be boarded at all.
         */
        // @todo change to return a Creature
        player *get_passenger() const;
        /**
         * Set the currently boarded creature (see @ref get_passenger).
         * Also sets some properties in the @p passenger object.
         * Precondition:
         * `get_passenger() == nullptr`
         * Postcondition:
         * `get_passenger() == &passenger`
         * @note Does *not* set the position of the creature, that must
         * be done manually (if needed).
         * See @ref map::board_vehicle.
         */
        void set_passenger( player &passenger ) const;
        /**
         * Remove the currently boarded creature (if any).
         */
        void unset_passenger() const;
        /**
         * Shortcut for `get_passenger() == &p`.
         */
        bool is_passenger( const player &p ) const;
};

#endif
