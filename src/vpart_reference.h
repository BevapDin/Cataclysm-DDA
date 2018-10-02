#pragma once
#ifndef VPART_REFERENCE_H
#define VPART_REFERENCE_H

#include "vpart_position.h"

#include <string>

class vehicle;
class vehicle_part;
enum vpart_bitflags : int;
class vpart_info;

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
};

#endif
