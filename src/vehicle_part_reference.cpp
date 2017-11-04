#include "vehicle_part_reference.h"

#include "vehicle.h"
#include "vehicle_stack.h"

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

units::volume vehicle_part_reference::max_volume() const
{
    return is_valid() ? veh_->max_volume( index_ ) : 0;
}

units::volume vehicle_part_reference::free_volume() const
{
    return is_valid() ? veh_->free_volume( index_ ) : 0;
}

units::volume vehicle_part_reference::stored_volume() const
{
    return is_valid() ? veh_->stored_volume( index_ ) : 0;
}

bool vehicle_part_reference::part_flag( const std::string &flag ) const
{
    return is_valid() && veh_->part_flag( index_, flag );
}

bool vehicle_part_reference::part_flag( const vpart_bitflags flag ) const
{
    return is_valid() && veh_->part_flag( index_, flag );
}

bool vehicle_part_reference::is_inside() const
{
    return is_valid() && veh_->is_inside( index_ );
}

vehicle_stack vehicle_part_reference::get_items() const
{
    assert( is_valid() );
    return veh_->get_items( index_ );
}

bool vehicle_part_reference::add_item( const item &obj ) const
{
    return is_valid() && veh_->add_item( index_, obj );
}

long vehicle_part_reference::add_charges( const item &itm ) const
{
    return is_valid() ? veh_->add_charges( index_, itm ) : 0l;
}

bool vehicle_part_reference::remove_item( const size_t item_index ) const
{
    return is_valid() && veh_->remove_item( index_, item_index );
}

bool vehicle_part_reference::remove_item( const item &it ) const
{
    return is_valid() && veh_->remove_item( index_, &it );
}

std::list<item>::iterator vehicle_part_reference::remove_item( const std::list<item>::iterator it )
{
    assert( is_valid() );
    return veh_->remove_item( index_, it );
}

std::string vehicle_part_reference::name() const
{
    return is_valid() ? veh_->parts[index_].name() : std::string();
}

vehicle_part_reference vehicle_part_reference::next_part_to_close( const bool outside ) const
{
    return vehicle_part_reference( *this, is_valid() ? veh_->next_part_to_close( index_,
                                   outside ) : -1 );
}

vehicle_part_reference vehicle_part_reference::next_part_to_open( const bool outside ) const
{
    return vehicle_part_reference( *this, is_valid() ? veh_->next_part_to_open( index_,
                                   outside ) : -1 );
}

void vehicle_part_reference::close() const
{
    if( !is_valid() ) {
        return;
    }
    veh_->close( index_ );
}

void vehicle_part_reference::open() const
{
    if( !is_valid() ) {
        return;
    }
    veh_->open( index_ );
}

void vehicle_part_reference::open_all_at() const
{
    if( !is_valid() ) {
        return;
    }
    veh_->open_all_at( index_ );
}

tripoint vehicle_part_reference::global_part_pos3() const
{
    assert( is_valid() );
    return veh_->global_part_pos3( index_ );
}

vehicle_part &vehicle_part_reference::part() const
{
    assert( is_valid() );
    return veh_->parts[index_];
}

const vpart_info &vehicle_part_reference::part_info() const
{
    assert( is_valid() );
    return veh_->part_info( index_ );
}

int vehicle_part_reference::damage( const int dmg, const damage_type type, const bool aimed ) const
{
    return is_valid() ? veh_->damage( index_, dmg, type, aimed ) : dmg;
}

vehicle_part_reference vehicle_part_reference::obstacle_at_part() const
{
    return vehicle_part_reference( *this, is_valid() ? veh_->obstacle_at_part( index_ ) : -1 );
}

nc_color vehicle_part_reference::part_color( const bool exact ) const
{
    return is_valid() ? veh_->part_color( index_ ) : c_white;
}

player *vehicle_part_reference::get_passenger() const
{
    return is_valid() ? veh_->get_passenger( index_ ) : nullptr;
}
