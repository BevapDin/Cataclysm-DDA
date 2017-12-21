#include "animation.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "monster.h"
#include "terrain_window.h"
#include "mtype.h"
#include "weather.h"
#include "sounds.h"
#include "scent_map.h"
#include "player.h"
#include "vehicle.h"
#include "map_iterator.h"
#ifdef TILES
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

#include <memory>

extern std::unique_ptr<cata_tiles> tilecontext; // obtained from sdltiles.cpp
#endif

#include <algorithm>
#include <array>

namespace {

class basic_animation
{
public:
    basic_animation( long const scale ) :
        delay{ 0, get_option<int>( "ANIMATION_DELAY" ) * scale * 1000000l }
    {
    }

    void draw() const {
        auto window = create_wait_popup_window( _( "Hang on a bit..." ) );

        wrefresh( g->w_terrain );
        wrefresh( window.get() );

        refresh_display();
    }

    void progress() const {
        draw();

        if( delay.tv_nsec > 0 ) {
            nanosleep( &delay, nullptr );
        }
    }

private:
    timespec delay;
};

class explosion_animation : public basic_animation
{
public:
    explosion_animation() :
        basic_animation( EXPLOSION_MULTIPLIER )
    {
    }
};

class bullet_animation : public basic_animation
{
public:
    bullet_animation() : basic_animation( 1 )
    {
    }
};


bool is_point_visible( const tripoint &p, int margin = 0 )
{
    return g->is_in_viewport( p, margin ) && g->u.sees( p );
}

bool is_radius_visible( const tripoint &center, int radius )
{
    return is_point_visible( center, -radius );
}

bool is_layer_visible( const std::map<tripoint, explosion_tile> &layer )
{
    return std::any_of( layer.begin(), layer.end(),
    [](const std::pair<tripoint, explosion_tile>& element) {
        return is_point_visible( element.first );
    } );
}


//! Get (x, y) relative to u's current position and view
tripoint relative_view_pos( player const &u, int const x, int const y, int const z ) noexcept
{
    return tripoint { getmaxx( g->w_terrain ) / 2 + x - u.posx() - u.view_offset.x,
            getmaxy( g->w_terrain ) / 2 + y - u.posy() - u.view_offset.y,
            z - u.posz() - u.view_offset.z };
}

tripoint relative_view_pos( player const &u, tripoint const &p ) noexcept
{
    return relative_view_pos( u, p.x, p.y, p.z );
}

void draw_explosion_curses(game &g, const tripoint &center, int const r, nc_color const col)
{
    if( !is_radius_visible( center, r ) ) {
        return;
    }
    // TODO: Make it look different from above/below
    tripoint const p = relative_view_pos( g.u, center );

    if (r == 0) { // TODO why not always print '*'?
        mvwputch(g.w_terrain, p.x, p.y, col, '*');
    }

    explosion_animation anim;

    for (int i = 1; i <= r; ++i) {
        mvwputch(g.w_terrain, p.y - i, p.x - i, col, '/');  // corner: top left
        mvwputch(g.w_terrain, p.y - i, p.x + i, col, '\\'); // corner: top right
        mvwputch(g.w_terrain, p.y + i, p.x - i, col, '\\'); // corner: bottom left
        mvwputch(g.w_terrain, p.y + i, p.x + i, col, '/');  // corner: bottom right
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(g.w_terrain, p.y - i, p.x + j, col, '-'); // edge: top
            mvwputch(g.w_terrain, p.y + i, p.x + j, col, '-'); // edge: bottom
            mvwputch(g.w_terrain, p.y + j, p.x - i, col, '|'); // edge: left
            mvwputch(g.w_terrain, p.y + j, p.x + i, col, '|'); // edge: right
        }

        anim.progress();
    }
}

constexpr explosion_neighbors operator | ( explosion_neighbors lhs, explosion_neighbors rhs )
{
    return static_cast<explosion_neighbors>( static_cast< int >( lhs ) | static_cast< int >( rhs ) );
}

constexpr explosion_neighbors operator ^ ( explosion_neighbors lhs, explosion_neighbors rhs )
{
    return static_cast<explosion_neighbors>( static_cast< int >( lhs ) ^ static_cast< int >( rhs ) );
}

void draw_custom_explosion_curses( game &g,
                                   const std::list< std::map<tripoint, explosion_tile> > &layers )
{
    // calculate screen offset relative to player + view offset position
    const tripoint center = g.u.pos() + g.u.view_offset;
    const tripoint topleft( center.x - getmaxx( g.w_terrain ) / 2, center.y - getmaxy( g.w_terrain ) / 2, 0 );

    explosion_animation anim;

    for( const auto &layer : layers ) {
        for( const auto &pr : layer ) {
            // update tripoint in relation to top left corner of curses window
            // mvwputch already filters out of bounds coordinates
            const tripoint p = pr.first - topleft;
            const explosion_neighbors ngh = pr.second.neighborhood;
            const nc_color col = pr.second.color;

            switch( ngh ) {
                // '^', 'v', '<', '>'
                case N_NORTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, '^' );
                    break;
                case N_SOUTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, 'v' );
                    break;
                case N_WEST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '<' );
                    break;
                case N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '>' );
                    break;
                // '|' and '-'
                case N_NORTH | N_SOUTH:
                case N_NORTH | N_SOUTH | N_WEST:
                case N_NORTH | N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '|' );
                    break;
                case N_WEST | N_EAST:
                case N_WEST | N_EAST | N_NORTH:
                case N_WEST | N_EAST | N_SOUTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, '-' );
                    break;
                // '/' and '\'
                case N_NORTH | N_WEST:
                case N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '/' );
                    break;
                case N_SOUTH | N_WEST:
                case N_NORTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '\\' );
                    break;
                case N_NO_NEIGHBORS:
                    mvwputch( g.w_terrain, p.y, p.x, col, '*' );
                    break;
                case N_WEST | N_EAST | N_NORTH | N_SOUTH:
                    break;
            }
        }

        if( is_layer_visible( layer ) ) {
            anim.progress();
        }
    }
}
} // namespace

#if defined(TILES)
void game::draw_explosion( const tripoint &p, int const r, nc_color const col )
{
    if( test_mode ) {
        return; // avoid segfault
    }

    if (!use_tiles) {
        draw_explosion_curses(*this, p, r, col);
        return;
    }

    if( !is_radius_visible( p, r ) ) {
        return;
    }

    explosion_animation anim;

    bool visible = is_radius_visible( p, r );
    for (int i = 1; i <= r; i++) {
        tilecontext->init_explosion( p, i ); // TODO not xpos ypos?
        if (visible) {
            anim.progress();
        }
    }

    if (r > 0) {
        tilecontext->void_explosion();
    }
}
#else
void game::draw_explosion( const tripoint &p, int const r, nc_color const col )
{
    draw_explosion_curses(*this, p, r, col);
}
#endif

void game::draw_custom_explosion( const tripoint &, const std::map<tripoint, nc_color> &all_area )
{
    if( test_mode ) {
        return; // avoid segfault
    }

    constexpr explosion_neighbors all_neighbors = N_NORTH | N_SOUTH | N_WEST | N_EAST;
    // We will "shell" the explosion area
    // Each phase will strip a single layer of points
    // A layer contains all points that have less than 4 neighbors in cardinal directions
    // Layers will first be generated, then drawn in inverse order

    // Start by getting rid of everything except current z-level
    std::map<tripoint, explosion_tile> neighbors;
#if defined(TILES)
    if( !use_tiles ) {
        for( const auto &pr : all_area ) {
            const tripoint relative_point = relative_view_pos( u, pr.first );
            if( relative_point.z == 0 ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    } else {
        // In tiles mode, the coordinates have to be absolute
        const tripoint view_center = relative_view_pos( u, u.pos() );
        for( const auto &pr : all_area ) {
            // Relative point is only used for z level check
            const tripoint relative_point = relative_view_pos( u, pr.first );
            if( relative_point.z == view_center.z ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    }
#else
    for( const auto &pr : all_area ) {
        const tripoint relative_point = relative_view_pos( u, pr.first );
        if( relative_point.z == 0 ) {
            neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
        }
    }
#endif

    // Searches for a neighbor, sets the neighborhood flag on current point and on the neighbor
    const auto set_neighbors = [&]( const tripoint &pos,
                                    explosion_neighbors &ngh,
                                    explosion_neighbors here,
                                    explosion_neighbors there ) {
        if( ( ngh & here ) == N_NO_NEIGHBORS ) {
            auto other = neighbors.find( pos );
            if( other != neighbors.end() ) {
                ngh = ngh | here;
                other->second.neighborhood = other->second.neighborhood | there;
            }
        }
    };

    // If the point we are about to remove has a neighbor in a given direction
    // unset that neighbor's flag that our current point is its neighbor
    const auto unset_neighbor = [&]( const tripoint &pos,
                                     const explosion_neighbors ngh,
                                     explosion_neighbors here,
                                     explosion_neighbors there ) {
        if( ( ngh & here ) != N_NO_NEIGHBORS ) {
            auto other = neighbors.find( pos );
            if( other != neighbors.end() ) {
                other->second.neighborhood = ( other->second.neighborhood | there ) ^ there;
            }
        }
    };

    // Find all neighborhoods
    for( auto &pr : neighbors ) {
        const tripoint &pt = pr.first;
        explosion_neighbors &ngh = pr.second.neighborhood;

        set_neighbors( tripoint( pt.x - 1, pt.y, pt.z ), ngh, N_WEST, N_EAST );
        set_neighbors( tripoint( pt.x + 1, pt.y, pt.z ), ngh, N_EAST, N_WEST );
        set_neighbors( tripoint( pt.x, pt.y - 1, pt.z ), ngh, N_NORTH, N_SOUTH );
        set_neighbors( tripoint( pt.x, pt.y + 1, pt.z ), ngh, N_SOUTH, N_NORTH );
    }

    // We need to save the layers because we will draw them in reverse order
    std::list< std::map<tripoint, explosion_tile> > layers;
    bool changed;
    while( !neighbors.empty() ) {
        std::map<tripoint, explosion_tile> layer;
        changed = false;
        // Find a layer that can be drawn
        for( const auto &pr : neighbors ) {
            if( pr.second.neighborhood != all_neighbors ) {
                changed = true;
                layer.insert( pr );
            }
        }
        if( !changed ) {
            // An error, but a minor one - let it slide
            return;
        }
        // Remove the layer from the area to process
        for( const auto &pr : layer ) {
            const tripoint &pt = pr.first;
            const explosion_neighbors ngh = pr.second.neighborhood;

            unset_neighbor( tripoint( pt.x - 1, pt.y, pt.z ), ngh, N_WEST, N_EAST );
            unset_neighbor( tripoint( pt.x + 1, pt.y, pt.z ), ngh, N_EAST, N_WEST );
            unset_neighbor( tripoint( pt.x, pt.y - 1, pt.z ), ngh, N_NORTH, N_SOUTH );
            unset_neighbor( tripoint( pt.x, pt.y + 1, pt.z ), ngh, N_SOUTH, N_NORTH );
            neighbors.erase( pr.first );
        }

        layers.push_front( std::move( layer ) );
    }

#if defined(TILES)
    if( !use_tiles ) {
        draw_custom_explosion_curses( *this, layers );
        return;
    }

    explosion_animation anim;
    // We need to draw all explosions up to now
    std::map<tripoint, explosion_tile> combined_layer;
    for( const auto &layer : layers ) {
        combined_layer.insert( layer.begin(), layer.end() );
        tilecontext->init_custom_explosion_layer( combined_layer );
        if( is_layer_visible( layer ) ) {
            anim.progress();
        }
    }

    tilecontext->void_custom_explosion();
#else
    draw_custom_explosion_curses( *this, layers );
#endif
}

namespace {

void draw_bullet_curses(map &m, const tripoint &t, char const bullet, const tripoint *const p)
{
    if ( !is_point_visible( t ) ) {
        return;
    }

    const tripoint vp = g->u.pos() + g->u.view_offset;

    if( p != nullptr && p->z == vp.z ) {
        m.drawsq( g->w_terrain, g->u, *p, false, true, vp );
    }

    if( vp.z != t.z ) {
        return;
    }

    mvwputch(g->w_terrain, getmaxy( g->w_terrain ) / 2 + (t.y - vp.y), getmaxx( g->w_terrain ) / 2 + (t.x - vp.x), c_red, bullet);
    bullet_animation().progress();
}

} ///namespace

#if defined(TILES)
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(const tripoint &t, int const i, const std::vector<tripoint> &trajectory, char const bullet)
{
    //TODO signature and impl could be changed to eliminate these params

    (void)i;          //unused
    (void)trajectory; //unused

    if (!use_tiles) {
        draw_bullet_curses(m, t, bullet, nullptr);
        return;
    }

    if( !is_point_visible( t ) ) {
        return;
    }

    static std::string const bullet_unknown  {};
    static std::string const bullet_normal   {"animation_bullet_normal"};
    static std::string const bullet_flame    {"animation_bullet_flame"};
    static std::string const bullet_shrapnel {"animation_bullet_shrapnel"};

    std::string const &bullet_type =
        (bullet == '*') ? bullet_normal
      : (bullet == '#') ? bullet_flame
      : (bullet == '`') ? bullet_shrapnel
      : bullet_unknown;

    tilecontext->init_draw_bullet( t, bullet_type );
    bullet_animation().progress();
    tilecontext->void_bullet();
}
#else
void game::draw_bullet(const tripoint &t, int const i, const std::vector<tripoint> &trajectory, char const bullet)
{
    draw_bullet_curses( m, t, bullet, &trajectory[i] );
}
#endif

namespace {
void draw_hit_mon_curses( const tripoint &center, const monster &m, player const& u, bool const dead )
{
    tripoint const p = relative_view_pos( u, center );
    hit_animation( p.x, p.y, red_background(m.type->color), dead ? "%" : m.symbol());
}

} // namespace

#if defined(TILES)
void game::draw_hit_mon( const tripoint &p, const monster &m, bool const dead )
{
    if (!use_tiles) {
        draw_hit_mon_curses( p, m, u, dead );
        return;
    }

    tilecontext->init_draw_hit( p, m.type->id.str() );

    bullet_animation().progress();
}
#else
void game::draw_hit_mon( const tripoint &p, const monster &m, bool const dead )
{
    draw_hit_mon_curses( p, m, u, dead );
}
#endif

namespace {
void draw_hit_player_curses(game const& g, player const &p, const int dam)
{
    nc_color const col = (!dam) ? yellow_background(p.symbol_color())
                                : red_background(p.symbol_color());

    tripoint const q = relative_view_pos( g.u, p.pos() );
    if( q.z == 0 ) {
        hit_animation( q.x, q.y, col, p.symbol() );
    }
}
} //namespace

#if defined(TILES)
void game::draw_hit_player(player const &p, const int dam)
{
    if (!use_tiles) {
        draw_hit_player_curses(*this, p, dam);
        return;
    }

    static std::string const player_male   {"player_male"};
    static std::string const player_female {"player_female"};
    static std::string const npc_male      {"npc_male"};
    static std::string const npc_female    {"npc_female"};

    std::string const& type = p.is_player() ? (p.male ? player_male : player_female)
                                            : (p.male ? npc_male    : npc_female);
    tilecontext->init_draw_hit( p.pos(), type );
    bullet_animation().progress();
}
#else
void game::draw_hit_player(player const &p, const int dam)
{
    draw_hit_player_curses(*this, p, dam);
}
#endif

/* Line drawing code, not really an animation but should be separated anyway */
void line_drawer::draw( terrain_window &w )
{
    for( const tripoint &p : points ) {
        if( trail ) {
            g->m.drawsq( w, g->u, p, true, true );
        } else {
            Creature *const critter = g->critter_at( p, true );

            // NPCs and monsters get drawn with inverted colors
            if( critter && g->u.sees( *critter ) ) {
                const point sp = w.to_screen_coord( critter->pos() );
                wmove( w, sp.y, sp.x );
                critter->draw( w, true );
            } else {
                g->m.drawsq( w, g->u, p, true, true, w.center() );
            }
        }
    }

    if( !trail ) {
        const tripoint mp = points.empty() ? g->u.pos() : points.back();
        const point sc = w.to_screen_coord( mp );
        mvwputch( w, sc.y, sc.x, c_white, 'X' );
    }
}

#if defined(TILES)
void line_drawer::draw( cata_tiles &tilecontext )
{
    if( trail ) {
        tilecontext.init_draw_line( pos, points, "line_trail", false );
    } else {
        if( !u.sees( pos ) ) {
            return;
        }
        tilecontext.init_draw_line( pos, points, "line_target", true );
    }
}
#endif

// 110 is shortly after basic map drawing and before anything after it.
weather_drawer::weather_drawer( weather_printable w ) : terrain_window_drawer( 110 ),
    wprint_ptr( new weather_printable( std::move( w ) ) ) { }

weather_drawer::~weather_drawer() = default;

void weather_drawer::draw( terrain_window &win )
{
    const weather_printable &w = *wprint_ptr;
    for( auto const &drop : w.vdrops ) {
        mvwputch( win, drop.second, drop.first, w.colGlyph, w.cGlyph );
    }
}
#ifdef TILES
void weather_drawer::draw( cata_tiles &tilecontext )
{
    weather_printable &w = *wprint_ptr;
    static std::string const weather_acid_drop {"weather_acid_drop"};
    static std::string const weather_rain_drop {"weather_rain_drop"};
    static std::string const weather_snowflake {"weather_snowflake"};

    std::string weather_name;
    switch( w.wtype ) {
        // Acid weathers; uses acid droplet tile, fallthrough intended
        case WEATHER_ACID_DRIZZLE:
        case WEATHER_ACID_RAIN:
            weather_name = weather_acid_drop;
            break;
        // Normal rainy weathers; uses normal raindrop tile, fallthrough intended
        case WEATHER_DRIZZLE:
        case WEATHER_RAINY:
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            weather_name = weather_rain_drop;
            break;
        // Snowy weathers; uses snowflake tile, fallthrough intended
        case WEATHER_FLURRIES:
        case WEATHER_SNOW:
        case WEATHER_SNOWSTORM:
            weather_name = weather_snowflake;
            break;
        default:
            break;
    }

    tilecontext.init_draw_weather( std::move( w ), std::move( weather_name ) );
}
#endif

void sct_drawer::draw( terrain_window &win )
{
    for( auto const &text : SCT.vSCT ) {
        // z-component is ignored anyway
        const point sp = win.to_screen_coord( tripoint( text.getPosX(), text.getPosY(), 0 ) );
        if( !win.contains( sp ) ) {
            continue;
        }

        bool const is_old = text.getStep() >= SCT.iMaxSteps / 2;

        nc_color const col1 = msgtype_to_color( text.getMsgType( "first" ),  is_old );
        nc_color const col2 = msgtype_to_color( text.getMsgType( "second" ), is_old );

        mvwprintz( win, sp.y, sp.x, col1, "%s", text.getText( "first" ).c_str() );
        wprintz( win, col2, "%s", text.getText( "second" ).c_str() );
    }
}
#ifdef TILES
void sct_drawer::draw( cata_tiles &tilecontext )
{
    tilecontext.init_draw_sct();
}
#endif

void zones_drawer::draw( terrain_window &w )
{
    if( end.x < start.x || end.y < start.y || end.z < start.z ) {
        return;
    }

    nc_color    const col = invert_color( c_light_green );
    std::string const line( end.x - start.x + 1, '~' );
    int         const x = start.x - offset.x;

    for( int y = start.y; y <= end.y; ++y ) {
        mvwprintz( w, y - offset.y, x, col, line.c_str() );
    }
}
#if defined(TILES)
void zones_drawer::draw( cata_tiles &tilecontext )
{
    tilecontext.init_draw_zones( start, end, offset );
}
#endif

void critter_drawer::draw( terrain_window &w )
{
    for( const Creature &critter : g->all_creatures() ) {
        const tripoint &pos = critter.pos();
        if( !w.contains( pos ) ) {
            continue;
        }
        const point sc = w.to_screen_coord( pos );
        if( pos.z != w.center().z && g->m.has_zlevels() ) {
            if( pos.z == w.center().z - 1 && ( debug_mode || u.sees( critter ) ) &&
                g->m.valid_move( pos, pos + tripoint( 0, 0, 1 ), false, true ) ) {
                // Monster is below
                // TODO: Make this show something more informative than just green 'v'
                // TODO: Allow looking at this mon with look command
                // TODO: Redraw this after weather etc. animations
                mvwputch( w, sc.y, sc.x, c_green_cyan, 'v' );
            }
            continue;
        }
        if( u.sees( critter ) || &critter == &u ) {
            const point sp = w.to_screen_coord( critter.pos() );
            wmove( w, sp.y, sp.x );
            critter.draw( w, false );
            return;
        }
        if( u.sees_with_infrared( critter ) ) {
            mvwputch( w, sc.y, sc.x, c_red, '?' );
        }
    }
}

void scent_vision_drawer::draw( terrain_window &w )
{
    if( !u.has_active_bionic( bionic_id( "bio_scent_vision" ) ) ) {
        return;
    }
    // @todo implement this for different z-levels
    if( u.pos().z != w.center().z ) {
        return;
    }
    for( const tripoint &p : w.map_range() ) {
        if( scent.get( p ) == 0 ) {
            continue;
        }
        // leave some space around the character undisturbed
        if( square_dist( p, u.pos() ) <= 2 ) {
            continue;
        }
        const point sc = w.to_screen_coord( p );
        if( g->critter_at( p ) ) {
            mvwputch( w, sc.y, sc.x, c_white, '?' );
        } else {
            mvwputch( w, sc.y, sc.x, c_magenta, '#' );
        }
    }
}

void footsteps_drawer::draw( terrain_window &w )
{
    const int z = w.center().z;
    for( const auto &footstep : sounds::get_footstep_markers() ) {
        const point sp = w.to_screen_coord( footstep );
        const char glyph = footstep.z == z ? '?' : ( footstep.z > z ? '^' : 'v' );
        mvwputch( w, sp.y, sp.x, c_yellow, glyph );
    }
}

void basic_map_drawer::draw( terrain_window &w )
{
    m.draw( w, w.center() );
}

void vehicle_direction_drawer::draw_indicator( terrain_window &w, const nc_color &col, const rl_vec2d &dir ) const
{
    const float r = 10.0; // arbitrary distance away
    const tripoint indicator_offset( r * dir.x, r * dir.y, u.pos().z );
    const point sp = w.to_screen_coord( indicator_offset + u.pos() );
    mvwputch( w, sp.y, sp.x, col, 'X' );
}

void vehicle_direction_drawer::draw( terrain_window &w )
{
    if( !u.controlling_vehicle ) {
        return;
    }
    if( !get_option<bool>( "VEHICLE_DIR_INDICATOR" ) ) {
        return;
    }
    vehicle *const veh = m.veh_at( u.pos() );
    if( !veh ) {
        return;
    }
    draw_indicator( w, c_dark_gray, veh->face_vec() );
    draw_indicator( w, c_white, veh->dir_vec() );
}

static std::vector<tripoint> create_line_vector( const tripoint &start, const tripoint &end )
{
    if( start == end ) {
        //Draw point
        return { start };
    } else {
        //Draw trail
        return line_to( start, end, 0, 0 );
    }
}

trail_to_square_drawer::trail_to_square_drawer( const tripoint &s, const tripoint &e,
        const bool f ) : line_drawer( e, create_line_vector( s, e ), false ), draw_final_x( f ) { }

void trail_to_square_drawer::draw( terrain_window &w )
{
    line_drawer::draw( w );
    if( draw_final_x ) {
        const char sym = end.z == w.center().z ? 'X' : ( end.z > w.center().z ? '^' : 'v' );
        const point sp = w.to_screen_coord( end );
        mvwputch( w, sp.y, sp.x, c_white, sym );
    }
}
