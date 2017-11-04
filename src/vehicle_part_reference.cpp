#include "vehicle_part_reference.h"

#include "vehicle.h"

#include <cassert>

vehicle_part_reference::vehicle_part_reference( const vehicle_part_reference &other,
        const int next_part ) : veh_( next_part >= 0 ? other.veh_ : nullptr ), index_( next_part )
{
    assert( other.is_valid() );
    assert( !is_valid() || static_cast<size_t>( index_ ) < veh_->parts.size() );
}

vehicle_part_reference::vehicle_part_reference() : veh_( nullptr ), index_( -1 )
{
    assert( !is_valid() );
}

vehicle_part_reference::vehicle_part_reference( vehicle &veh, const int part ) : veh_( &veh ),
    index_( part )
{
    assert( static_cast<size_t>( index_ ) < veh_->parts.size() );
}

vehicle_part_reference vehicle_part_reference::part_with_feature( const std::string &feature,
        const bool unbroken ) const
{
    return vehicle_part_reference( *this, is_valid() ? veh_->part_with_feature( index_, feature,
                                   unbroken ) : -1 );
}

vehicle_part_reference vehicle_part_reference::part_with_feature( const vpart_bitflags feature,
        const bool unbroken ) const
{
    return vehicle_part_reference( *this, is_valid() ? veh_->part_with_feature( index_, feature,
                                   unbroken ) : -1 );
}

std::string vehicle_part_reference::get_label() const
{
    if( !is_valid() ) {
        return std::string();
    }
    const vehicle_part &part = veh_->parts[index_];
    return veh_->get_label( part.mount.x, part.mount.y );
}

bool vehicle_part_reference::is_inside() const
{
    return is_valid() && veh_->is_inside( index_ );
}
