#include "reload_option.h"

#include "item.h"
#include "itype.h"
#include "player.h"

#include <algorithm>

reload_option::reload_option( const reload_option &rhs ) :
    who( rhs.who ), target( rhs.target ), ammo( rhs.ammo.clone() ),
    qty_( rhs.qty_ ), max_qty( rhs.max_qty ), parent( rhs.parent ) {}

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
                              item_location &&ammo ) :
    who( who ), target( target ), ammo( std::move( ammo ) ), parent( parent )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage != "NULL" ) {
        max_qty = this->who->charges_of( this->target->type->magazine->linkage );
    }

    // magazine, ammo or ammo container
    item &tmp = ( this->ammo->is_ammo_container() || this->ammo->is_watertight_container() )
                ? this->ammo->contents.front()
                : *this->ammo;

    if( this->ammo->is_watertight_container() || ( tmp.is_ammo() &&
            !target->has_flag( "RELOAD_ONE" ) ) ) {
        qty( tmp.charges );
    } else {
        qty( 1 );
    }
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
    if( ammo->is_magazine() || target->has_flag( "RELOAD_ONE" ) ) {
        qty_ = 1L;
        return;
    }

    item *obj = nullptr;
    bool is_ammo_container = ammo->is_ammo_container();
    bool is_liquid_container = ammo->is_watertight_container();
    if( is_ammo_container || is_liquid_container ) {
        obj = &( ammo->contents.front() );
    } else {
        obj = &*ammo;
    }

    if( ( is_ammo_container && !obj->is_ammo() ) ||
        ( is_liquid_container && !obj->made_of( LIQUID ) ) ) {
        debugmsg( "Invalid reload option: %s", obj->tname().c_str() );
        return;
    }

    long limit = is_liquid_container
                 ? target->get_remaining_capacity_for_liquid( *obj, true )
                 : target->ammo_capacity() - target->ammo_remaining();

    if( target->ammo_type() == ammotype( "plutonium" ) ) {
        limit = limit / PLUTONIUM_CHARGES + ( limit % PLUTONIUM_CHARGES != 0 );
    }

    // constrain by available ammo, target capacity and other external factors (max_qty)
    // @ref max_qty is currently set when reloading ammo belts and limits to available linkages
    qty_ = std::min( { val, obj->charges, limit, max_qty } );

    // always expect to reload at least one charge
    qty_ = std::max( qty_, 1L );

}
