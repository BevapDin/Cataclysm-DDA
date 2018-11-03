#pragma once
#ifndef VPART_REFERENCE_H
#define VPART_REFERENCE_H

#include "vpart_position.h"

#include <string>

class vehicle;
struct vehicle_part;
class vpart_info;
enum vpart_bitflags : int;

enum class OpenOrClosed {
    Open,
    Closed,
};

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
         * Opens/closes this part. If it's a multipart, opens/closes all
         * attached parts as well.
         * `this` must refer to a suitable part, see
         * @ref vpart_position::next_part_to_close /
         * @ref vpart_position::next_part_to_open.
         */
        /**@{*/
        void open() const;
        void close() const;
        void open_or_close( OpenOrClosed what ) const;
        /**@}*/
        /**
         * Returns whether this part is considered open.
         * Returns `false` for parts that can not be opended at all.
         */
        bool is_open() const;
};

#endif
