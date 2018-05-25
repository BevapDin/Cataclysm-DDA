#include "reload_option.h"

#include "item.h"
#include "itype.h"
#include "player.h"

#include <algorithm>
#include <climits>

reload_option::reload_option( const reload_option &rhs ) : who( rhs.who ), target( rhs.target ),
    ammo( rhs.ammo.clone() ), qty_( rhs.qty_ ), max_qty( rhs.max_qty ), parent( rhs.parent ) { }

reload_option &reload_option::operator=( const reload_option &rhs )
{
    who = rhs.who;
    target = rhs.target;
    ammo = rhs.ammo.clone();
    qty_ = rhs.qty_;
    max_qty = rhs.max_qty;
    parent = rhs.parent;

    return *this;
}

reload_option::reload_option( const player *who, const item *target, const item *parent,
                              item_location &&ammo ) : who( who ), target( target ), ammo( std::move( ammo ) ), parent( parent )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage != "NULL" ) {
        max_qty = this->who->charges_of( this->target->type->magazine->linkage );
    } else {
        max_qty = LONG_MAX;
    }
    qty( max_qty );
}

int reload_option::moves() const
{
    int mv = ammo.obtain_cost( *who, qty() ) + who->item_reload_cost( *target, *ammo, qty() );
    if( parent != target ) {
        if( parent->is_gun() ) {
            mv += parent->type->gun->reload_time;
        } else if( parent->is_tool() ) {
            mv += 100;
        }
    }
    return mv;
}

void reload_option::qty( long val )
{
    bool ammo_in_container = ammo->is_ammo_container();
    bool ammo_in_liquid_container = ammo->is_watertight_container();
    item &ammo_obj = ( ammo_in_container || ammo_in_liquid_container ) ?
                     ammo->contents.front() : *ammo;

    if( ( ammo_in_container && !ammo_obj.is_ammo() ) ||
        ( ammo_in_liquid_container && !ammo_obj.made_of( LIQUID ) ) ) {
        debugmsg( "Invalid reload option: %s", ammo_obj.tname().c_str() );
        return;
    }

    // Checking ammo capacity implicitly limits guns with removable magazines to capacity 0.
    // This gets rounded up to 1 later.
    long remaining_capacity = target->is_watertight_container() ?
                              target->get_remaining_capacity_for_liquid( ammo_obj, true ) :
                              target->ammo_capacity() - target->ammo_remaining();
    if( target->has_flag( "RELOAD_ONE" ) && !ammo->has_flag( "SPEEDLOADER" ) ) {
        remaining_capacity = 1;
    }
    if( target->ammo_type() == ammotype( "plutonium" ) ) {
        remaining_capacity = remaining_capacity / PLUTONIUM_CHARGES +
                             ( remaining_capacity % PLUTONIUM_CHARGES != 0 );
    }

    bool ammo_by_charges = ammo_obj.is_ammo() || ammo_in_liquid_container;
    long available_ammo = ammo_by_charges ? ammo_obj.charges : ammo_obj.ammo_remaining();
    // constrain by available ammo, target capacity and other external factors (max_qty)
    // @ref max_qty is currently set when reloading ammo belts and limits to available linkages
    qty_ = std::min( { val, available_ammo, remaining_capacity, max_qty } );

    // always expect to reload at least one charge
    qty_ = std::max( qty_, 1L );

}
