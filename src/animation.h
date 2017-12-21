#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include "color.h"
#include "terrain_window.h"
#include "enums.h"

#include <memory>

class vehicle;
class player;
class map;
struct rl_vec2d;
class scent_map;
struct weather_printable;

enum explosion_neighbors {
    N_NO_NEIGHBORS = 0,
    N_NORTH = 1,

    N_SOUTH = 2,
    N_NS = 3,

    N_WEST = 4,
    N_NW = 5,
    N_SW = 6,
    N_NSW = 7,

    N_EAST = 8,
    N_NE = 9,
    N_SE = 10,
    N_NSE = 11,
    N_WE = 12,
    N_NWE = 13,
    N_SWE = 14,
    N_NSWE = 15
};

struct explosion_tile {
    explosion_neighbors neighborhood;
    nc_color color;
};

class line_drawer : public terrain_window_drawer
{
    private:
        tripoint pos;
        std::vector<tripoint> points;
        bool trail;

    public:
        line_drawer( const tripoint &p, std::vector<tripoint> r, const bool t ) : terrain_window_drawer( 700 ), pos( p ), points( std::move( r ) ), trail( t ) { }
        ~line_drawer() override = default;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};

class weather_drawer : public terrain_window_drawer
{
    private:
        // private implementation to avoid including the header
        std::unique_ptr<weather_printable> wprint_ptr;

    public:
        // 110 is shortly after basic map drawing and before anything after it.
        weather_drawer( weather_printable w );
        ~weather_drawer() override;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};

class scent_vision_drawer : public terrain_window_drawer
{
    private:
        const player &u;
        const scent_map &scent;

    public:
        scent_vision_drawer( const player &u, const scent_map &s ) : terrain_window_drawer( 300 ), u( u ),
            scent( s ) { }
        ~scent_vision_drawer() override = default;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};

class footsteps_drawer : public terrain_window_drawer
{
    public:
        footsteps_drawer() : terrain_window_drawer( 100 ) { }
        ~footsteps_drawer() override = default;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};
/**
 * Draws the map to the window, as much of it as possible. See map::draw.
 */
class basic_map_drawer : public terrain_window_drawer
{
    private:
        map &m;

    public:
        basic_map_drawer( map &m ) : terrain_window_drawer( 0 ), m( m ) { }
        ~basic_map_drawer() override = default;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};
/**
 * Draws an indicator where the vehicle is currently moving and facing to.
 */
class vehicle_direction_drawer : public terrain_window_drawer
{
    private:
        player &u;
        map &m;

        void draw_indicator( terrain_window &w, const nc_color &col, const rl_vec2d &dir ) const;

    public:
        vehicle_direction_drawer( player &u, map &m ) : terrain_window_drawer( 500 ), u( u ), m( m ) {
        }
        ~vehicle_direction_drawer() override = default;

        void draw( terrain_window &w ) override;
#ifdef TILES
        void draw( cata_tiles &tilecontext ) override;
#endif
};

#endif
