#pragma once
#ifndef VEHICLE_PART_REFERENCE_H
#define VEHICLE_PART_REFERENCE_H

#include <string>
#include <list>

class vehicle;
enum vpart_bitflags : int;
class vehicle_stack;
class item;
namespace units
{
template<typename V, typename U>
class quantity;
class volume_in_milliliter_tag;
using volume = quantity<int, volume_in_milliliter_tag>;
} // namespace units

/**
 * This is a wrapper over a vehicle pointer and a part index. The index refers to
 * a part of said vehicle.
 * The reference can be "invalid", which means it does not refer to any vehicle
 * at all. Some (but not all) functions may still be called on invalid references.
 * One can use `operator bool` or @ref is_valid to check for validness.
 * Most functions just forward to the equally named functions in the @ref vehicle
 * class, so see there for documentation.
 */
class vehicle_part_reference
{
    private:
        vehicle *veh_;
        //@todo it should be a size_t
        int index_;

        // Initialize this with a *potentially* invalid part index.
        vehicle_part_reference( const vehicle_part_reference &other, int next_part );

    public:
        /// Creates an invalid part reference.
        vehicle_part_reference();
        /// Creates a valid part reference. @p part must be a valid index!
        vehicle_part_reference( vehicle &veh, int part );

        /// @see is_valid
        explicit operator bool() const {
            return is_valid();
        }
        /// Returns whether this reference is valid, that is whether it refers to
        /// an actual vehicle part.
        bool is_valid() const {
            return veh_ != nullptr;
        }
        /// Returns the vehicle this refers to, if any. Returns `nullptr` on
        /// invalid references.
        vehicle *veh() const {
            return veh_;
        }

        bool operator==( const vehicle_part_reference &rhs ) const {
            return veh_ == rhs.veh_ && ( !veh_ || index_ == rhs.index_ );
        }
        bool operator!=( const vehicle_part_reference &rhs ) const {
            return !operator==( rhs );
        }
        /// Returns an empty string on an invalid reference.
        std::string get_label() const;
        /// May return an invalid reference.
        /// Returns an invalid reference if called on an invalid reference.
        /**@{*/
        vehicle_part_reference part_with_feature( const std::string &feature, bool unbroken = true ) const;
        vehicle_part_reference part_with_feature( vpart_bitflags feature, bool unbroken = true ) const;
        /**@}*/
        /// Returns 0 on an invalid reference.
        units::volume max_volume() const;
        /// Returns 0 on an invalid reference.
        units::volume free_volume() const;
        /// Returns 0 on an invalid reference.
        units::volume stored_volume() const;
        /// Returns `false` on an invalid reference.
        /// Returns `false` on an invalid reference.
        bool is_inside() const;
        /// *Must* be called on an valid reference.
        vehicle_stack get_items() const;
        /// Returns `false` on an invalid reference.
        bool add_item( const item &obj ) const;
        /// Returns 0 on an invalid reference.
        long add_charges( const item &itm ) const;
        /// Returns `false` on an invalid reference, but the overload with
        /// the iterator *must* be called on a valid reference.
        /**@{*/
        bool remove_item( size_t item_index ) const;
        bool remove_item( const item &it ) const;
        std::list<item>::iterator remove_item( std::list<item>::iterator it );
        /**@}*/
};

#endif
