#pragma once
#ifndef VEHICLE_PART_REFERENCE_H
#define VEHICLE_PART_REFERENCE_H

#include <string>

class vehicle;
enum vpart_bitflags : int;

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

        /// May return an invalid reference.
        /// Returns an invalid reference if called on an invalid reference.
        /**@{*/
        vehicle_part_reference part_with_feature( const std::string &feature, bool unbroken = true ) const;
        vehicle_part_reference part_with_feature( vpart_bitflags feature, bool unbroken = true ) const;
        /**@}*/
        /// Returns `false` on an invalid reference.
        bool is_inside() const;
};

#endif
