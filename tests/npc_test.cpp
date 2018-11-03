#include "catch/catch.hpp"

#include "common_types.h"
#include "player.h"
#include "npc.h"
#include "npc_class.h"
#include "game.h"
#include "map.h"
#include "text_snippets.h"
#include "field.h"
#include "overmapbuffer.h"
#include "vehicle.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_reference.h"

#include <string>

void on_load_test( npc &who, const time_duration &from, const time_duration &to )
{
    calendar::turn = to_turn<int>( calendar::time_of_cataclysm + from );
    who.on_unload();
    calendar::turn = to_turn<int>( calendar::time_of_cataclysm + to );
    who.on_load();
}

void test_needs( const npc &who, const numeric_interval<int> &hunger,
                 const numeric_interval<int> &thirst,
                 const numeric_interval<int> &fatigue )
{
    CHECK( who.get_hunger() <= hunger.max );
    CHECK( who.get_hunger() >= hunger.min );
    CHECK( who.get_thirst() <= thirst.max );
    CHECK( who.get_thirst() >= thirst.min );
    CHECK( who.get_fatigue() <= fatigue.max );
    CHECK( who.get_fatigue() >= fatigue.min );
}

npc create_model()
{
    npc model_npc;
    model_npc.normalize();
    model_npc.randomize( NC_NONE );
    for( trait_id tr : model_npc.get_mutations() ) {
        model_npc.unset_mutation( tr );
    }
    model_npc.set_hunger( 0 );
    model_npc.set_thirst( 0 );
    model_npc.set_fatigue( 0 );
    model_npc.remove_effect( efftype_id( "sleep" ) );
    // An ugly hack to prevent NPC falling asleep during testing due to massive fatigue
    model_npc.set_mutation( trait_id( "WEB_WEAVER" ) );

    return model_npc;
}

TEST_CASE( "on_load-sane-values", "[.]" )
{
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc test_npc = create_model();
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );
        const int margin = 2;

        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 2 days, gaining hunger/thirst/fatigue" ) {
        npc test_npc = create_model();
        const auto five_min_ticks = 2_days / 5_minutes;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 20;
        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Sleeping for 6 hours, gaining hunger/thirst (not testing fatigue due to lack of effects processing)" ) {
        npc test_npc = create_model();
        test_npc.add_effect( efftype_id( "sleep" ), 6_hours );
        test_npc.set_fatigue( 1000 );
        const auto five_min_ticks = 6_hours / 5_minutes;
        /*
        // Fatigue regeneration starts at 1 per 5min, but linearly increases to 2 per 5min at 2 hours or more
        const int expected_fatigue_change =
            ((1.0f + 2.0f) / 2.0f * 2_hours / 5_minutes ) +
            (2.0f * (6_hours - 2_hours) / 5_minutes);
        */
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 10;
        const numeric_interval<int> hunger( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> fatigue( test_npc.get_fatigue(), 0, 0 );

        test_needs( test_npc, hunger, thirst, fatigue );
    }
}

TEST_CASE( "on_load-similar-to-per-turn", "[.]" )
{
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = create_model();
        npc iterated_npc = create_model();
        const int five_min_ticks = 2;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::time_of_cataclysm + turn,
                                      calendar::time_of_cataclysm + turn + 1_turns );
        }

        const int margin = 2;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> fatigue( iterated_npc.get_fatigue(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 6 hours, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = create_model();
        npc iterated_npc = create_model();
        const auto five_min_ticks = 6_hours / 5_minutes;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::time_of_cataclysm + turn,
                                      calendar::time_of_cataclysm + turn + 1_turns );
        }

        const int margin = 10;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> fatigue( iterated_npc.get_fatigue(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, fatigue );
    }
}

TEST_CASE( "snippet-tag-test" )
{
    // Actually used tags
    static const std::set<std::string> npc_talk_tags = {{
            "<name_b>", "<thirsty>", "<swear!>",
            "<sad>", "<greet>", "<no>",
            "<im_leaving_you>", "<ill_kill_you>", "<ill_die>",
            "<wait>", "<no_faction>", "<name_g>",
            "<keep_up>", "<yawn>", "<very>",
            "<okay>", "<catch_up>", "<really>",
            "<let_me_pass>", "<done_mugging>", "<happy>",
            "<drop_weapon>", "<swear>", "<lets_talk>",
            "<hands_up>", "<move>", "<hungry>",
            "<fuck_you>",
        }
    };

    for( const auto &tag : npc_talk_tags ) {
        const auto ids = SNIPPET.all_ids_from_category( tag );
        std::set<std::string> valid_snippets;
        for( int id : ids ) {
            const auto snip = SNIPPET.get( id );
            valid_snippets.insert( snip );
        }

        // We want to get all the snippets in the category
        std::set<std::string> found_snippets;
        // Brute force random snippets to see if they are all in their category
        for( size_t i = 0; i < ids.size() * 100; i++ ) {
            const auto &roll = SNIPPET.random_from_category( tag );
            CHECK( valid_snippets.count( roll ) > 0 );
            found_snippets.insert( roll );
        }

        CHECK( found_snippets == valid_snippets );
    }

    // Special tags, those should have empty replacements
    CHECK( SNIPPET.all_ids_from_category( "<yrwp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<mywp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<ammo>" ).empty() );
}

/* Test setup. Player should always be at top-left.
 *
 * U is the player, V is vehicle, # is wall, R is rubble & acid with NPC on it,
 * A is acid with NPC on it, W/M is vehicle & acid with (follower/non-follower) NPC on it,
 * B/C is acid with (follower/non-follower) NPC on it.
 */
constexpr int height = 5, width = 17;
constexpr char setup[height][width + 1] = {
    "U ###############",
    "V #R#AAA#W# # #C#",
    "  #A#A#A# #M#B# #",
    "  ###AAA#########",
    "    #####        ",
};

static void check_npc_movement( const tripoint &origin )
{
    const efftype_id effect_bouldering( "bouldering" );

    INFO( "Should not crash from infinite recursion" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'A':
                case 'R':
                case 'W':
                case 'M':
                case 'B':
                case 'C':
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    REQUIRE( guy != nullptr );
                    guy->move();
                    break;
            }
        }
    }

    INFO( "NPC on acid should not acquire unstable footing status" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'A' ) {
                tripoint p = origin + point( x, y );
                npc *guy = g->critter_at<npc>( p );
                REQUIRE( guy != nullptr );
                CHECK( !guy->has_effect( effect_bouldering ) );
            }
        }
    }

    INFO( "NPC on rubbles should not lose unstable footing status" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'R' ) {
                tripoint p = origin + point( x, y );
                npc *guy = g->critter_at<npc>( p );
                REQUIRE( guy != nullptr );
                CHECK( guy->has_effect( effect_bouldering ) );
            }
        }
    }

    INFO( "NPC in vehicle should not escape from dangerous terrain" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'W':
                case 'M':
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    CHECK( guy != nullptr );
                    break;
            }
        }
    }

    INFO( "NPC not in vehicle should escape from dangerous terrain" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'B':
                case 'C':
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    CHECK( guy == nullptr );
                    break;
            }
        }
    }
}

TEST_CASE( "npc-movement" )
{
    const ter_id t_reinforced_glass( "t_reinforced_glass" );
    const ter_id t_floor( "t_floor" );
    const furn_id f_rubble( "f_rubble" );
    const furn_id f_null( "f_null" );
    const vpart_id vpart_frame_vertical( "frame_vertical" );
    const vpart_id vpart_seat( "seat" );

    g->place_player( tripoint( 60, 60, 0 ) );

    // kill npcs before removing vehicles so they are correctly unboarded
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = g->u.pos() + point( x, y );
            Creature *cre = g->critter_at( p );
            if( cre != nullptr && cre != &g->u ) {
                npc *guy = dynamic_cast<npc *>( cre );
                cre->die( nullptr );
                if( guy ) {
                    overmap_buffer.remove_npc( guy->getID() );
                }
            }
        }
    }
    g->unload_npcs();
    // remove existing vehicles
    VehicleList vehs = g->m.get_vehicles( g->u.pos(), g->u.pos() + point( width - 1, height - 1 ) );
    for( auto &veh : vehs ) {
        g->m.detach_vehicle( veh.v );
    }
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const char type = setup[y][x];
            const tripoint p = g->u.pos() + point( x, y );
            // create walls
            if( type == '#' ) {
                g->m.ter_set( p, t_reinforced_glass );
            } else {
                g->m.ter_set( p, t_floor );
            }
            // spawn acid
            // a copy is needed because we will remove elements from it
            const field fs = g->m.field_at( p );
            for( const auto &f : fs ) {
                g->m.remove_field( p, f.first );
            }
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                g->m.add_field( p, fd_acid, MAX_FIELD_DENSITY );
            }
            // spawn rubbles
            if( type == 'R' ) {
                g->m.furn_set( p, f_rubble );
            } else {
                g->m.furn_set( p, f_null );
            }
            // create vehicles
            if( type == 'V' || type == 'W' || type == 'M' ) {
                vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), p, 270, 0, 0 );
                REQUIRE( veh != nullptr );
                veh->install_part( point( 0, 0 ), vpart_frame_vertical );
                veh->install_part( point( 0, 0 ), vpart_seat );
                g->m.add_vehicle_to_cache( veh );
            }
            // spawn npcs
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                std::shared_ptr<npc> guy = std::make_shared<npc>();
                guy->normalize();
                guy->randomize();
                guy->spawn_at_precise( {g->get_levx(), g->get_levy()}, p );
                overmap_buffer.insert_npc( guy );
                g->load_npcs();
                guy->set_attitude( ( type == 'M' || type == 'C' ) ? NPCATT_NULL : NPCATT_FOLLOW );
            }
        }
    }

    // check preconditions
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const char type = setup[y][x];
            const tripoint p = g->u.pos() + point( x, y );
            if( type == '#' ) {
                REQUIRE( !g->m.passable( p ) );
            } else {
                REQUIRE( g->m.passable( p ) );
            }
            if( type == 'R' ) {
                REQUIRE( g->m.has_flag( "UNSTABLE", p ) );
            } else {
                REQUIRE( !g->m.has_flag( "UNSTABLE", p ) );
            }
            if( type == 'V' || type == 'W' || type == 'M' ) {
                REQUIRE( g->m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            } else {
                REQUIRE( !g->m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            }
            npc *guy = g->critter_at<npc>( p );
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                REQUIRE( guy != nullptr );
                REQUIRE( guy->is_dangerous_fields( g->m.field_at( p ) ) );
            } else {
                REQUIRE( guy == nullptr );
            }
        }
    }

    SECTION( "NPCs escape dangerous terrain by pushing other NPCs" ) {
        check_npc_movement( g->u.pos() );
    }

    SECTION( "Player in vehicle & NPCs escaping dangerous terrain" ) {
        tripoint origin = g->u.pos();

        for( int y = 0; y < height; ++y ) {
            for( int x = 0; x < width; ++x ) {
                if( setup[y][x] == 'V' ) {
                    g->place_player( g->u.pos() + point( x, y ) );
                    break;
                }
            }
        }

        check_npc_movement( origin );
    }
}
