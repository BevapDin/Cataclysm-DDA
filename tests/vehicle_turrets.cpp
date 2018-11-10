#include "catch/catch.hpp"

#include "ammo.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "player.h"
#include "veh_type.h"
#include "vehicle.h"

static std::vector<const vpart_info *> turret_types()
{
    std::vector<const vpart_info *> res;

    for( const auto &e : vpart_info::all() ) {
        if( e.second.has_flag( "TURRET" ) ) {
            res.push_back( &e.second );
        }
    }

    return res;
}

const vpart_info *biggest_tank( const ammotype ammo )
{
    std::vector<const vpart_info *> res;

    for( const auto &e : vpart_info::all() ) {
        const auto &vp = e.second;
        if( !item( vp.item ).is_watertight_container() ) {
            continue;
        }

        const itype *fuel = item::find_type( vp.fuel_type );
        if( fuel->ammo && fuel->ammo->type.count( ammo ) ) {
            res.push_back( &vp );
        }
    }

    if( res.empty() ) {
        return nullptr;
    }

    return * std::max_element( res.begin(), res.end(),
    []( const vpart_info * lhs, const vpart_info * rhs ) {
        return lhs->size < rhs->size;
    } );
}

TEST_CASE( "vehicle_turret", "[vehicle] [gun] [magazine] [.]" )
{
    for( auto e : turret_types() ) {
        SECTION( e->name() ) {
            vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), 65, 65, 270, 0, 0 );
            REQUIRE( veh );
            REQUIRE( veh->parts.size() == 0 );

            REQUIRE( veh->can_mount( 0, 0, e->get_id() ).success() );
            const int idx = veh->index_of_part( veh->install_part( 0, 0, e->get_id() ) );

            REQUIRE( veh->can_mount( 0, 0, vpart_id( "storage_battery" ) ).success() );
            veh->install_part( 0,  0, vpart_id( "storage_battery" ) );
            REQUIRE( veh->parts.size() == 2 );
            veh->charge_battery( 10000 );

            auto ammo = veh->turret_query( veh->parts[idx] ).base()->ammo_type();

            if( veh->part_flag( idx, "USE_TANKS" ) ) {
                auto *tank = biggest_tank( ammo );
                REQUIRE( tank );
                INFO( tank->get_id().str() );

                REQUIRE( veh->can_mount( 0, 0, tank->get_id() ).success() );
                vehicle_part &tank_part = veh->install_part( 0, 0, tank->get_id() );
                REQUIRE( tank_part.ammo_set( ammo->default_ammotype() ) );

            } else if( ammo ) {
                veh->parts[ idx].ammo_set( ammo->default_ammotype() );
            }

            auto qry = veh->turret_query( veh->parts[ idx ] );
            REQUIRE( qry );

            REQUIRE( qry.query() == turret_data::status::ready );
            REQUIRE( qry.range() > 0 );

            g->u.setpos( veh->global_part_pos3( idx ) );
            REQUIRE( qry.fire( g->u, g->u.pos() + point( qry.range(), 0 ) ) > 0 );

            g->m.destroy_vehicle( veh );
        }
    }
}
