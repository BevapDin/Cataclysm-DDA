#pragma once
#ifndef VEHICLE_PART_REFERENCE_H
#define VEHICLE_PART_REFERENCE_H

#include <string>
#include <list>

class player;
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
struct tripoint;
struct point;
struct vehicle_part;
class vpart_info;
enum damage_type : int;
using nc_color = int;

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
        /// Returns the part index this refers to. The result is unspecified
        /// if this reference is invalid.
        int index() const {
            return index_;
        }
        bool operator==( const vehicle_part_reference &rhs ) const {
            return veh_ == rhs.veh_ && ( !veh_ || index_ == rhs.index_ );
        }
        bool operator!=( const vehicle_part_reference &rhs ) const {
            return !operator==( rhs );
        }
        /// Returns the referred vehicle part from the parts vector @ref vehicle::parts.
        /// Must be called on a valid reference.
        vehicle_part &part() const;
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
        /**@{*/
        bool part_flag( const std::string &flag ) const;
        bool part_flag( vpart_bitflags flag ) const;
        /**@}*/
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
        /// @return vehicle_part::mount. Must not be called on an invalid reference.
        point mount_point() const;
        /// Returns an empty string on an invalid reference.
        std::string name() const;
        /// Returns an invalid reference when called on an invalid reference.
        vehicle_part_reference next_part_to_close( bool outside = false ) const;
        /// Returns an invalid reference when called on an invalid reference.
        vehicle_part_reference next_part_to_open( bool outside = false ) const;
        void open() const;
        void close() const;
        void open_all_at() const;
        /// Returns \p dmg on an invalid reference.
        int damage( int dmg, damage_type type, bool aimed = true ) const;
        /// Returns an invalid reference when called on an invalid reference.
        vehicle_part_reference obstacle_at_part() const;
        /// Must be called on a valid reference.
        tripoint global_part_pos3() const;
        /// Must be called on a valid reference.
        const vpart_info &part_info() const;
        /// Returns an unspecific, but valid color when called on an invalid reference.
        nc_color part_color( bool exact = false ) const;
        /// Returns `nullptr` on an invalid reference.
        player *get_passenger() const;
};

/// Compares @ref vehicle_part_reference::veh() to given vehicle pointer.
/**@{*/
inline bool is_same_vehicle( const vehicle_part_reference &vpart, const vehicle *const veh )
{
    return vpart.veh() == veh;
}

inline bool is_same_vehicle( const vehicle *const veh, const vehicle_part_reference &vpart )
{
    return vpart.veh() == veh;
}

inline bool is_same_vehicle( const vehicle_part_reference &vpart1,
                             const vehicle_part_reference &vpart2 )
{
    return vpart1.veh() == vpart2.veh();
}
/**@}*/

#endif
