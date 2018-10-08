#pragma once
#ifndef VPART_REFERENCE_H
#define VPART_REFERENCE_H

#include "vpart_position.h"

#include <string>
#include <list>

class item;
class vehicle;
class vehicle_part;
enum vpart_bitflags : int;
class vpart_info;
class vehicle_stack;
namespace units
{
template<typename V, typename U>
class quantity;
class volume_in_milliliter_tag;
using volume = quantity<int, volume_in_milliliter_tag>;
} // namespace units

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
    private:

    public:
        vpart_reference( ::vehicle &v, const size_t part ) : vpart_position( v, part ) { }
        vpart_reference( const vpart_reference & ) = default;

        using vpart_position::vehicle;

        vehicle_part &part() const;
        const vpart_info &info() const;

        bool has_feature( const std::string &feature ) const;
        bool has_feature( vpart_bitflags feature ) const;

        bool operator==( const vpart_reference &rhs ) const {
            return &vehicle() == &rhs.vehicle() && part_index() == rhs.part_index();
        }
        bool operator!=( const vpart_reference &rhs ) const {
            return !operator==( rhs );
        }

        units::volume max_volume() const;
        units::volume free_volume() const;

        bool add_item( const item &obj ) const;
        bool remove_item( const item *it );
        vehicle_stack get_items() const;

        bool is_open() const;
};

#endif
