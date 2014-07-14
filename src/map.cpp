#include "map.h"
#include "lightmap.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "options.h"
#include "item_factory.h"
#include "mapbuffer.h"
#include "translations.h"
#include "monstergenerator.h"
#include <cmath>
#include <cassert>
#include <stdlib.h>
#include <fstream>
#include "debug.h"
#include "messages.h"
#include "mapsharing.h"

extern bool is_valid_in_w_terrain(int,int);

#include "overmapbuffer.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define dbg(x) DebugLog((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

map::map(int mapsize, int z_min, int z_max)
: my_MAPSIZE(mapsize)
, my_ZMIN(z_min)
, my_ZMAX(z_max)
{
    assert(mapsize > 0);
    assert(z_min <= 0 && z_max >= 0);
    assert(z_min <= z_max);
    grid.resize( my_MAPSIZE * my_MAPSIZE * ( 1 + my_ZMAX - my_ZMIN ), NULL );
    psm_cache.resize(grid.size());
    dbg( D_INFO ) << "map::map(" << mapsize << ", " << z_min << ", " << z_max << ")";
    veh_in_active_range = true;
    transparency_cache_dirty = true;
    outside_cache_dirty = true;
    memset(veh_exists_at, 0, sizeof(veh_exists_at));
}

map::~map()
{
}

VehicleList map::get_vehicles(){
   return get_vehicles(0,0,my_ZMIN,SEEX*my_MAPSIZE, SEEY*my_MAPSIZE,my_ZMAX);
}

VehicleList map::get_vehicles(const int sx, const int sy, const int ex, const int ey)
{
    return get_vehicles(sx, sy, 0, ex, ey, 0);
}

VehicleList map::get_vehicles(const int sx, const int sy, const int sz, const int ex, const int ey, const int ez)
{
 const int chunk_sx = std::max( 0, (sx / SEEX) - 1 );
 const int chunk_ex = std::min( my_MAPSIZE - 1, (ex / SEEX) + 1 );
 const int chunk_sy = std::max( 0, (sy / SEEY) - 1 );
 const int chunk_ey = std::min( my_MAPSIZE - 1, (ey / SEEY) + 1 );
    const int chunk_sz = std::max(sz, my_ZMIN);
    const int chunk_ez = std::min(ez, my_ZMAX);
 VehicleList vehs;

 for(int cx = chunk_sx; cx <= chunk_ex; ++cx) {
  for(int cy = chunk_sy; cy <= chunk_ey; ++cy) {
   for(int cz = chunk_sz; cz <= chunk_ez; ++cz) {
   submap *current_submap = get_submap_at_grid( tripoint( cx, cy, cz ) );
   for( auto &elem : current_submap->vehicles ) {
    wrapped_vehicle w;
    w.v = elem;
    w.x = w.v->posx + cx * SEEX;
    w.y = w.v->posy + cy * SEEY;
    w.i = cx;
    w.j = cy;
    w.z = cz;
    vehs.push_back(w);
   }
  }
 }
 }

 return vehs;
}

vehicle* map::veh_at(const int x, const int y, int &part_num) const
{
    return veh_at(tripoint(x, y, 0), part_num);
}

vehicle* map::veh_at(const tripoint &p, int &part_num) const
{
    // This function is called A LOT. Move as much out of here as possible.
    if (!veh_in_active_range || !inbounds(p)) {
        // Out-of-bounds - null vehicle
        return NULL;
    }
    if (!veh_exists_at[p.x][p.y][p.z - my_ZMIN]) {
        // cache cache indicates no vehicle. This should optimize a great deal.
        return NULL;
    }
    const auto it = veh_cached_parts.find( p );
    if( it != veh_cached_parts.end() ) {
        part_num = it->second.second;
        return it->second.first;
    }
    debugmsg ("vehicle part cache cache indicated vehicle not found: %d %d %d", p.x, p.y, p.z);
    return NULL;
}

point map::veh_part_coordinates(const int x, const int y)
{
    int part_num;
    vehicle* veh = veh_at(x, y, part_num);

    if(veh == nullptr) {
        return point(0,0);
    }

    auto part = veh->parts[part_num];
    return point(part.mount_dx, part.mount_dy);
}

vehicle* map::veh_at(const tripoint &p) const
{
    int part = 0;
    return veh_at(p, part);
}

vehicle* map::veh_at(const int x, const int y) const
{
 int part = 0;
    return veh_at(tripoint(x, y, 0), part);
}

void map::reset_vehicle_cache()
{
    clear_vehicle_cache();
    // Cache all vehicles
    veh_in_active_range = false;
    for( const auto & elem : vehicle_list ) {
        update_vehicle_cache( elem, true );
    }
}

void map::update_vehicle_cache( vehicle *veh, const bool brand_new )
{
    veh_in_active_range = true;
    if( !brand_new ) {
        // Existing must be cleared
        auto it = veh_cached_parts.begin();
        const auto end = veh_cached_parts.end();
        while( it != end ) {
            if( it->second.first == veh ) {
                const auto &p = it->first;
                if( inbounds( p ) ) {
                    veh_exists_at[p.x][p.y][p.z - my_ZMIN] = false;
                }
                veh_cached_parts.erase( it++ );
            } else {
                ++it;
            }
        }
    }
    // Get parts
    std::vector<vehicle_part> &parts = veh->parts;
    const int gx = veh->global_x();
    const int gy = veh->global_y();
    const int gz = veh->global_z();
    int partid = 0;
    for( std::vector<vehicle_part>::iterator it = parts.begin(),
         end = parts.end(); it != end; ++it, ++partid ) {
        if( it->removed ) {
            continue;
        }
        const tripoint p( gx + it->precalc_dx[0],
                       gy + it->precalc_dy[0], gz );
        veh_cached_parts.insert( std::make_pair( p,
                                 std::make_pair( veh, partid ) ) );
        if( inbounds( p ) ) {
            veh_exists_at[p.x][p.y][p.z - my_ZMIN] = true;
        }
    }
}

void map::clear_vehicle_cache()
{
    while( veh_cached_parts.size() ) {
        const auto part = veh_cached_parts.begin();
        const auto &p = part->first;
        if( inbounds( p.x, p.y ) ) {
            veh_exists_at[p.x][p.y][p.z - my_ZMIN] = false;
        }
        veh_cached_parts.erase( part );
    }
}

void map::update_vehicle_list( const tripoint &gp )
{
    const auto to = get_submap_at_grid( gp );
    for( auto & elem : to->vehicles ) {
        vehicle_list.insert( elem );
        elem->smx = gp.x;
        elem->smy = gp.y;
        elem->smz = gp.z;
    }
}

void map::remove_from_vehicle_cache(vehicle *veh)
{
    for(t_vehicle_cache::iterator it = veh_cached_parts.begin(); it != veh_cached_parts.end(); ) {
        if(it->second.first != veh) {
            ++it;
            continue;
        }
        const tripoint &p = it->first;
        veh_exists_at[p.x][p.y][p.z - my_ZMIN] = false;
        veh_cached_parts.erase(it++);
    }
}

void map::board_vehicle(int x, int y, player *p)
{
    if (!p) {
        debugmsg ("map::board_vehicle: null player");
        return;
    }

    int part = 0;
    vehicle *veh = veh_at(x, y, part);
    if (!veh) {
        if(p->grab_point.x == 0 && p->grab_point.y == 0) {
            debugmsg ("map::board_vehicle: vehicle not found");
        }
        return;
    }

    const int seat_part = veh->part_with_feature (part, VPFLAG_BOARDABLE);
    if (seat_part < 0) {
        debugmsg ("map::board_vehicle: boarding %s (not boardable)",
                  veh->part_info(part).name.c_str());
        return;
    }
    if (veh->parts[seat_part].has_flag(vehicle_part::passenger_flag)) {
        player *psg = veh->get_passenger (seat_part);
        debugmsg ("map::board_vehicle: passenger (%s) is already there",
                  psg ? psg->name.c_str() : "<null>");
        unboard_vehicle( x, y );
    }
    veh->parts[seat_part].set_flag(vehicle_part::passenger_flag);
    veh->parts[seat_part].passenger_id = p->getID();

    p->posx = x;
    p->posy = y;
    p->in_vehicle = true;
    if (p == &g->u &&
        (x < SEEX * int(my_MAPSIZE / 2) || y < SEEY * int(my_MAPSIZE / 2) ||
         x >= SEEX * (1 + int(my_MAPSIZE / 2)) ||
         y >= SEEY * (1 + int(my_MAPSIZE / 2))   )) {
        g->update_map(x, y, 0);
    }
}

void map::unboard_vehicle(const int x, const int y)
{
    int part = 0;
    vehicle *veh = veh_at(x, y, part);
    player *passenger = NULL;
    if (!veh) {
        debugmsg ("map::unboard_vehicle: vehicle not found");
        // Try and force unboard the player anyway.
        if( g->u.xpos() == x && g->u.ypos() == y ) {
            passenger = &(g->u);
        } else {
            int npcdex = g->npc_at( x, y );
            if( npcdex != -1 ) {
                passenger = g->active_npc[npcdex];
            }
        }
        if( passenger ) {
            passenger->in_vehicle = false;
            passenger->driving_recoil = 0;
            passenger->controlling_vehicle = false;
        }
        return;
    }
    const int seat_part = veh->part_with_feature (part, VPFLAG_BOARDABLE, false);
    if (seat_part < 0) {
        debugmsg ("map::unboard_vehicle: unboarding %s (not boardable)",
                  veh->part_info(part).name.c_str());
        return;
    }
    passenger = veh->get_passenger(seat_part);
    if (!passenger) {
        debugmsg ("map::unboard_vehicle: passenger not found");
        return;
    }
    passenger->in_vehicle = false;
    passenger->driving_recoil = 0;
    passenger->controlling_vehicle = false;
    veh->parts[seat_part].remove_flag(vehicle_part::passenger_flag);
    veh->skidding = true;
}

void map::destroy_vehicle (vehicle *veh)
{
    if (!veh) {
        debugmsg("map::destroy_vehicle was passed NULL");
        return;
    }
    submap * const current_submap = get_submap_at_grid( tripoint( veh->smx, veh->smy, veh->smz ) );
    for (size_t i = 0; i < current_submap->vehicles.size(); i++) {
        if (current_submap->vehicles[i] == veh) {
            vehicle_list.erase(veh);
            remove_from_vehicle_cache(veh);
            reset_vehicle_cache();
            current_submap->vehicles.erase (current_submap->vehicles.begin() + i);
            delete veh;
            return;
        }
    }
    debugmsg ("destroy_vehicle can't find it! name=%s, x=%d, y=%d, z=%d", veh->name.c_str(), veh->smx, veh->smy, veh->smz);
}

// TODO: Z
bool map::displace_vehicle (int &x, int &y, const int dx, const int dy, bool test)
{
    const int x2 = x + dx;
    const int y2 = y + dy;
    int srcx = x;
    int srcy = y;
    int dstx = x2;
    int dsty = y2;

    if (!inbounds(srcx, srcy)){
        add_msg( m_debug, "map::displace_vehicle: coords out of bounds %d,%d->%d,%d",
                        srcx, srcy, dstx, dsty);
        return false;
    }

 // TODO: Z
    int src_offset_x, src_offset_y, dst_offset_x, dst_offset_y;
    submap * const src_submap = get_submap_at(srcx, srcy, src_offset_x, src_offset_y);
    submap * const dst_submap = get_submap_at(dstx, dsty, dst_offset_x, dst_offset_y);

    if (test) {
        return src_submap != dst_submap;
    }

    // first, let's find our position in current vehicles vector
    int our_i = -1;
    for (size_t i = 0; i < src_submap->vehicles.size(); i++) {
        if (src_submap->vehicles[i]->posx == src_offset_x &&
              src_submap->vehicles[i]->posy == src_offset_y) {
            our_i = i;
            break;
        }
    }
    if (our_i < 0) {
        vehicle *v = veh_at(x, y);
        for( auto & smap : grid ) {
            for (size_t i = 0; i < smap->vehicles.size(); i++) {
                if (smap->vehicles[i] == v) {
                    our_i = i;
                    const_cast<submap*&>(src_submap) = smap;
                    break;
                }
            }
        }
    }
    if (our_i < 0) {
        add_msg( m_debug, "displace_vehicle our_i=%d", our_i );
        return false;
    }
    // move the vehicle
    vehicle *veh = src_submap->vehicles[our_i];
    // don't let it go off grid
    if (!inbounds(x2, y2)) {
        veh->stop();
        // Silent debug
        dbg(D_ERROR) << "map:displace_vehicle: Stopping vehicle, displaced dx=" << dx << ", dy=" << dy;
        return false;
    }

    // record every passenger inside
    std::vector<int> psg_parts = veh->boarded_parts();
    std::vector<player *> psgs;
    for (auto &p : psg_parts) {
        psgs.push_back(veh->get_passenger(p));
    }

    const int rec = abs(veh->velocity) / 5 / 100;

    bool need_update = false;
    int upd_x, upd_y;
    // move passengers
    for (size_t i = 0; i < psg_parts.size(); i++) {
        player *psg = psgs[i];
        const int p = psg_parts[i];
        if (!psg) {
            debugmsg ("empty passenger part %d pcoord=%d,%d u=%d,%d?", p,
                         veh->global_x() + veh->parts[p].precalc_dx[0],
                         veh->global_y() + veh->parts[p].precalc_dy[0],
                                  g->u.posx, g->u.posy);
            veh->parts[p].remove_flag(vehicle_part::passenger_flag);
            continue;
        }
        // add recoil
        psg->driving_recoil = rec;
        // displace passenger taking in account vehicle movement (dx, dy)
        // and turning: precalc_dx/dy [0] contains previous frame direction,
        // and precalc_dx/dy[1] should contain next direction
        psg->posx += dx + veh->parts[p].precalc_dx[1] - veh->parts[p].precalc_dx[0];
        psg->posy += dy + veh->parts[p].precalc_dy[1] - veh->parts[p].precalc_dy[0];
        if (psg == &g->u) { // if passenger is you, we need to update the map
            need_update = true;
            upd_x = psg->posx;
            upd_y = psg->posy;
        }
    }

    veh->shed_loose_parts();
    for (auto &p : veh->parts) {
        p.precalc_dx[0] = p.precalc_dx[1];
        p.precalc_dy[0] = p.precalc_dy[1];
    }

    veh->posx = dst_offset_x;
    veh->posy = dst_offset_y;
    if (src_submap != dst_submap) {
        // TODO: Z
        veh->set_submap_moved( int( x2 / SEEX ), int( y2 / SEEY ), 0 );
        dst_submap->vehicles.push_back( veh );
        src_submap->vehicles.erase( src_submap->vehicles.begin() + our_i );
    }

    // Need old coords to check for remote control
    bool remote = veh->remote_controlled( &g->u );

    x += dx;
    y += dy;

    if( remote ) {
        g->setremoteveh( veh );
    }

    update_vehicle_cache(veh);

    bool was_update = false;
    if (need_update &&
          (upd_x < SEEX * int(my_MAPSIZE / 2) || upd_y < SEEY *int(my_MAPSIZE / 2) ||
          upd_x >= SEEX * (1+int(my_MAPSIZE / 2)) ||
          upd_y >= SEEY * (1+int(my_MAPSIZE / 2)))) {
        // map will shift, so adjust vehicle coords we've been passed
        if (upd_x < SEEX * int(my_MAPSIZE / 2)) {
            x += SEEX;
        } else if (upd_x >= SEEX * (1+int(my_MAPSIZE / 2))) {
            x -= SEEX;
        }
        if (upd_y < SEEY * int(my_MAPSIZE / 2)) {
            y += SEEY;
        } else if (upd_y >= SEEY * (1+int(my_MAPSIZE / 2))) {
            y -= SEEY;
        }
        g->update_map(upd_x, upd_y, 0);
        was_update = true;
    }

    return (src_submap != dst_submap) || was_update;
}

void map::on_vehicle_moved() {
    set_outside_cache_dirty();
    set_transparency_cache_dirty();
}

void map::vehmove()
{
    // give vehicles movement points
    {
        VehicleList vehs = get_vehicles();
        for( auto &vehs_v : vehs ) {
            vehicle *veh = vehs_v.v;
            veh->gain_moves();
            veh->slow_leak();
        }
    }

    // 15 equals 3 >50mph vehicles, or up to 15 slow (1 square move) ones
    for( int count = 0; count < 15; count++ ) {
        if( !vehproceed() ) {
            break;
        } else {
            on_vehicle_moved();
        }
    }
    // Process item removal on the vehicles that were modified this turn.
    for( const auto &elem : dirty_vehicle_list ) {
        ( elem )->part_removal_cleanup();
    }
    dirty_vehicle_list.clear();
}

bool map::vehproceed()
{
    VehicleList vehs = get_vehicles();
    vehicle* veh = NULL;
    float max_of_turn = 0;
    int x; int y;
    for( auto &vehs_v : vehs ) {
        if( vehs_v.v->of_turn > max_of_turn ) {
            veh = vehs_v.v;
            x = vehs_v.x;
            y = vehs_v.y;
            max_of_turn = veh->of_turn;
        }
    }
    if(!veh) { return false; }

    if (!inbounds(x, y)) {
        dbg( D_INFO ) << "stopping out-of-map vehicle. (x,y)=(" << x << "," << y << ")";
        veh->stop();
        veh->of_turn = 0;
        return true;
    }

    bool pl_ctrl = veh->player_in_control(&g->u);

    // k slowdown first.
    int slowdown = veh->skidding? 200 : 20; // mph lost per tile when coasting
    float kslw = (0.1 + veh->k_dynamics()) / ((0.1) + veh->k_mass());
    slowdown = (int) ceil(kslw * slowdown);
    if (abs(slowdown) > abs(veh->velocity)) {
        veh->stop();
    } else if (veh->velocity < 0) {
      veh->velocity += slowdown;
    } else {
      veh->velocity -= slowdown;
    }

    //low enough for bicycles to go in reverse.
    if (veh->velocity && abs(veh->velocity) < 20) {
        veh->stop();
    }

    if(veh->velocity == 0) {
        veh->of_turn -= .321f;
        return true;
    }

    std::vector<int> float_indices = veh->all_parts_with_feature(VPFLAG_FLOATS, false);
    if (float_indices.empty()) {
        // sink in water?
        std::vector<int> wheel_indices = veh->all_parts_with_feature(VPFLAG_WHEEL, false);
        int num_wheels = wheel_indices.size(), submerged_wheels = 0;
        for (int w = 0; w < num_wheels; w++) {
            const int p = wheel_indices[w];
            const int px = x + veh->parts[p].precalc_dx[0];
            const int py = y + veh->parts[p].precalc_dy[0];
            // deep water
            if (ter_at(px, py).has_flag(TFLAG_DEEP_WATER)) {
                submerged_wheels++;
            }
        }
        // submerged wheels threshold is 2/3.
        if (num_wheels && (float)submerged_wheels / num_wheels > .666) {
            add_msg(m_bad, _("Your %s sank."), veh->name.c_str());
            if (pl_ctrl) {
                veh->unboard_all();
            }
            // destroy vehicle (sank to nowhere)
            destroy_vehicle(veh);
            return true;
        }
    } else {

        int num = float_indices.size(), moored = 0;
        for (int w = 0; w < num; w++) {
            const int p = float_indices[w];
            const int px = x + veh->parts[p].precalc_dx[0];
            const int py = y + veh->parts[p].precalc_dy[0];

            if (!has_flag("SWIMMABLE", px, py)) {
                moored++;
            }
        }

        if (moored > num - 1) {
            veh->stop();
            veh->of_turn = 0;

            add_msg(m_info, _("Your %s is beached."), veh->name.c_str());

            return true;
        }

    }
    // One-tile step take some of movement
    //  terrain cost is 1000 on roads.
    // This is stupid btw, it makes veh magically seem
    //  to accelerate when exiting rubble areas.
    float ter_turn_cost = 500.0 * move_cost_ter_furn (x,y) / abs(veh->velocity);

    //can't afford it this turn?
    if(ter_turn_cost >= veh->of_turn) {
        veh->of_turn_carry = veh->of_turn;
        veh->of_turn = 0;
        return true;
    }

    veh->of_turn -= ter_turn_cost;

    // if not enough wheels, mess up the ground a bit.
    if (!veh->valid_wheel_config()) {
        veh->velocity += veh->velocity < 0 ? 2000 : -2000;
        for (auto &p : veh->parts) {
            const int px = x + p.precalc_dx[0];
            const int py = y + p.precalc_dy[0];
            const ter_id &pter = ter(px, py);
            if (pter == t_dirt || pter == t_grass) {
                ter_set(px, py, t_dirtmound);
            }
        }
    }

    if (veh->skidding) {
        if (one_in(4)) { // might turn uncontrollably while skidding
            veh->turn (one_in(2) ? -15 : 15);
        }
    }
    else if (pl_ctrl && rng(0, 4) > g->u.skillLevel("driving") && one_in(20)) {
        add_msg(m_warning, _("You fumble with the %s's controls."), veh->name.c_str());
        veh->turn (one_in(2) ? -15 : 15);
    }
    // eventually send it skidding if no control
    if (!veh->boarded_parts().size() && one_in (10)) {
        veh->skidding = true;
    }
    tileray mdir; // the direction we're moving
    if (veh->skidding) { // if skidding, it's the move vector
        mdir = veh->move;
    } else if (veh->turn_dir != veh->face.dir()) {
        mdir.init (veh->turn_dir); // driver turned vehicle, get turn_dir
    } else {
      mdir = veh->face;          // not turning, keep face.dir
    }
    mdir.advance (veh->velocity < 0? -1 : 1);
    const int dx = mdir.dx();           // where do we go
    const int dy = mdir.dy();           // where do we go
    bool can_move = true;
    // calculate parts' mount points @ next turn (put them into precalc[1])
    veh->precalc_mounts(1, veh->skidding ? veh->turn_dir : mdir.dir());

    int dmg_1 = 0;

    std::vector<veh_collision> veh_veh_colls;
    std::vector<veh_collision> veh_misc_colls;

    if (veh->velocity == 0) { can_move = false; }
    // find collisions
    int vel1 = veh->velocity/100; //velocity of car before collision
    veh->collision( veh_veh_colls, veh_misc_colls, dx, dy, can_move, dmg_1 );

    bool veh_veh_coll_flag = false;
    // Used to calculate the epicenter of the collision.
    point epicenter1(0, 0);
    point epicenter2(0, 0);

    if(veh_veh_colls.size()) { // we have dynamic crap!
        // effects of colliding with another vehicle:
        // transfers of momentum, skidding,
        // parts are damaged/broken on both sides,
        // remaining times are normalized,
        veh_veh_coll_flag = true;
        veh_collision c = veh_veh_colls[0]; //Note: What´s with collisions with more than 2 vehicles?
        vehicle* veh2 = (vehicle*) c.target;
        add_msg(m_bad, _("The %1$s's %2$s collides with the %3$s's %4$s."),
                       veh->name.c_str(),  veh->part_info(c.part).name.c_str(),
                       veh2->name.c_str(), veh2->part_info(c.target_part).name.c_str());

        // for reference, a cargo truck weighs ~25300, a bicycle 690,
        //  and 38mph is 3800 'velocity'
        rl_vec2d velo_veh1 = veh->velo_vec();
        rl_vec2d velo_veh2 = veh2->velo_vec();
        float m1 = veh->total_mass();
        float m2 = veh2->total_mass();
        //Energy of vehicle1 annd vehicle2 before collision
        float E = 0.5 * m1 * velo_veh1.norm() * velo_veh1.norm() +
            0.5 * m2 * velo_veh2.norm() * velo_veh2.norm();

        //collision_axis
        int x_cof1 = 0, y_cof1 = 0, x_cof2 = 0, y_cof2 = 0;
        veh ->center_of_mass(x_cof1, y_cof1);
        veh2->center_of_mass(x_cof2, y_cof2);
        rl_vec2d collision_axis_y;

        collision_axis_y.x = ( veh->global_x() + x_cof1 ) -  ( veh2->global_x() + x_cof2 );
        collision_axis_y.y = ( veh->global_y() + y_cof1 ) -  ( veh2->global_y() + y_cof2 );
        collision_axis_y = collision_axis_y.normalized();
        rl_vec2d collision_axis_x = collision_axis_y.get_vertical();
        // imp? & delta? & final? reworked:
        // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
        // as per http://en.wikipedia.org/wiki/Elastic_collision
        //velocity of veh1 before collision in the direction of collision_axis_y
        float vel1_y = collision_axis_y.dot_product(velo_veh1);
        float vel1_x = collision_axis_x.dot_product(velo_veh1);
        //velocity of veh2 before collision in the direction of collision_axis_y
        float vel2_y = collision_axis_y.dot_product(velo_veh2);
        float vel2_x = collision_axis_x.dot_product(velo_veh2);
        // e = 0 -> inelastic collision
        // e = 1 -> elastic collision
        float e = get_collision_factor(vel1_y/100 - vel2_y/100);

        //velocity after collision
        float vel1_x_a = vel1_x;
        // vel1_x_a = vel1_x, because in x-direction we have no transmission of force
        float vel2_x_a = vel2_x;
        //transmission of force only in direction of collision_axix_y
        //equation: partially elastic collision
        float vel1_y_a = ( m2 * vel2_y * ( 1 + e ) + vel1_y * ( m1 - m2 * e) ) / ( m1 + m2);
        //equation: partially elastic collision
        float vel2_y_a = ( m1 * vel1_y * ( 1 + e ) + vel2_y * ( m2 - m1 * e) ) / ( m1 + m2);
        //add both components; Note: collision_axis is normalized
        rl_vec2d final1 = collision_axis_y * vel1_y_a + collision_axis_x * vel1_x_a;
        //add both components; Note: collision_axis is normalized
        rl_vec2d final2 = collision_axis_y * vel2_y_a + collision_axis_x * vel2_x_a;

        //Energy after collision
        float E_a = 0.5 * m1 * final1.norm() * final1.norm() +
            0.5 * m2 * final2.norm() * final2.norm();
        float d_E = E - E_a;  //Lost energy at collision -> deformation energy
        float dmg = std::abs( d_E / 1000 / 2000 );  //adjust to balance damage
        float dmg_veh1 = dmg * 0.5;
        float dmg_veh2 = dmg * 0.5;

        int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
        for( auto &veh_veh_coll : veh_veh_colls ) {
            veh_collision tmp_c = veh_veh_coll;
            if(veh2 == (vehicle*) tmp_c.target) { coll_parts_cnt++; }
        }

        float dmg1_part = dmg_veh1 / coll_parts_cnt;
        float dmg2_part = dmg_veh2 / coll_parts_cnt;

        //damage colliding parts (only veh1 and veh2 parts)
        for( auto &veh_veh_coll : veh_veh_colls ) {
            veh_collision tmp_c = veh_veh_coll;

            if(veh2 == (vehicle*) tmp_c.target) {
                int parm1 = veh->part_with_feature (tmp_c.part, VPFLAG_ARMOR);
                if (parm1 < 0) {
                    parm1 = tmp_c.part;
                }
                int parm2 = veh2->part_with_feature (tmp_c.target_part, VPFLAG_ARMOR);
                if (parm2 < 0) {
                    parm2 = tmp_c.target_part;
                }
                epicenter1.x += veh->parts[parm1].mount_dx;
                epicenter1.y += veh->parts[parm1].mount_dy;
                veh->damage(parm1, dmg1_part, 1);

                epicenter2.x += veh2->parts[parm2].mount_dx;
                epicenter2.y += veh2->parts[parm2].mount_dy;
                veh2->damage(parm2, dmg2_part, 1);
            }
        }
        epicenter1.x /= coll_parts_cnt;
        epicenter1.y /= coll_parts_cnt;
        epicenter2.x /= coll_parts_cnt;
        epicenter2.y /= coll_parts_cnt;


        if (dmg2_part > 100) {
            // shake veh because of collision
            veh2->damage_all(dmg2_part / 2, dmg2_part, 1, epicenter2);
        }

        dmg_1 += dmg1_part;

        veh->move.init (final1.x, final1.y);
        veh->velocity = final1.norm();
        // shrug it off if the change is less than 8mph.
        if(dmg_veh1 > 800) {
            veh->skidding = 1;
        }
        veh2->move.init(final2.x, final2.y);
        veh2->velocity = final2.norm();
        if(dmg_veh2 > 800) {
            veh2->skidding = 1;
        }
        //give veh2 the initiative to proceed next before veh1
        float avg_of_turn = (veh2->of_turn + veh->of_turn) / 2;
        if(avg_of_turn < .1f)
            avg_of_turn = .1f;
        veh->of_turn = avg_of_turn * .9;
        veh2->of_turn = avg_of_turn * 1.1;
    }

    for( auto &veh_misc_coll : veh_misc_colls ) {

        point collision_point( veh->parts[veh_misc_coll.part].mount_dx,
                               veh->parts[veh_misc_coll.part].mount_dy );
        int coll_dmg = veh_misc_coll.imp;
        //Shock damage
        veh->damage_all(coll_dmg / 2, coll_dmg, 1, collision_point);
    }

    int coll_turn = 0;
    if (dmg_1 > 0) {
        int vel1_a = veh->velocity / 100; //velocity of car after collision
        int d_vel = abs(vel1 - vel1_a);

        std::vector<int> ppl = veh->boarded_parts();

        for (auto &ps : ppl) {
            player *psg = veh->get_passenger (ps);
            if (!psg) {
                debugmsg ("throw passenger: empty passenger at part %d", ps);
                continue;
            }

            bool throw_from_seat = 0;
            if (veh->part_with_feature (ps, VPFLAG_SEATBELT) == -1) {
                throw_from_seat = d_vel * rng(80, 120) / 100 > (psg->str_cur * 1.5 + 5);
            }

            //damage passengers if d_vel is too high
            if(d_vel > 60* rng(50,100)/100 && !throw_from_seat) {
                int dmg = d_vel/4*rng(70,100)/100;
                psg->hurtall(dmg);
                if (psg == &g->u) {
                    add_msg(m_bad, _("You take %d damage by the power of the impact!"), dmg);
                } else if (psg->name.length()) {
                    add_msg(m_bad, _("%s takes %d damage by the power of the impact!"),
                                   psg->name.c_str(), dmg);
                }
            }

            if (throw_from_seat) {
                if (psg == &g->u) {
                    add_msg(m_bad, _("You are hurled from the %s's seat by the power of the impact!"),
                                   veh->name.c_str());
                } else if (psg->name.length()) {
                    add_msg(m_bad, _("%s is hurled from the %s's seat by the power of the impact!"),
                                   psg->name.c_str(), veh->name.c_str());
                }
                unboard_vehicle(x + veh->parts[ps].precalc_dx[0],
                                     y + veh->parts[ps].precalc_dy[0]);
                g->fling_creature(psg, mdir.dir() + rng(0, 60) - 30,
                                           (vel1 - psg->str_cur < 10 ? 10 :
                                            vel1 - psg->str_cur));
            } else if (veh->part_with_feature (ps, "CONTROLS") >= 0) {
                // FIXME: should actually check if passenger is in control,
                // not just if there are controls there.
                const int lose_ctrl_roll = rng (0, dmg_1);
                if (lose_ctrl_roll > psg->dex_cur * 2 + psg->skillLevel("driving") * 3) {
                    if (psg == &g->u) {
                        add_msg(m_warning, _("You lose control of the %s."), veh->name.c_str());
                    } else if (psg->name.length()) {
                        add_msg(m_warning, _("%s loses control of the %s."), psg->name.c_str());
                    }
                    int turn_amount = (rng (1, 3) * sqrt((double)vel1_a) / 2) / 15;
                    if (turn_amount < 1) {
                        turn_amount = 1;
                    }
                    turn_amount *= 15;
                    if (turn_amount > 120) {
                        turn_amount = 120;
                    }
                    coll_turn = one_in (2)? turn_amount : -turn_amount;
                }
            }
        }
    }
    if(veh_veh_coll_flag) return true;

    // now we're gonna handle traps we're standing on (if we're still moving).
    // this is done here before displacement because
    // after displacement veh reference would be invdalid.
    // damn references!
    if (can_move) {
        std::vector<int> wheel_indices = veh->all_parts_with_feature("WHEEL", false);
        for (auto &w : wheel_indices) {
            const int wheel_x = x + veh->parts[w].precalc_dx[0];
            const int wheel_y = y + veh->parts[w].precalc_dy[0];
            if (one_in(2)) {
                if( displace_water( tripoint(wheel_x, wheel_y, 0)) && pl_ctrl ) {
                    add_msg(m_warning, _("You hear a splash!"));
                }
            }
            veh->handle_trap( wheel_x, wheel_y, w );
            auto &item_vec = i_at( wheel_x, wheel_y );
            for( auto it = item_vec.begin(); it != item_vec.end(); ) {
                item *smashed_item = get_item( wheel_x, wheel_y, it );
                smashed_item->damage += rng( 0, 3 );
                if( smashed_item->damage > 4 ) {
                    it = i_rem( wheel_x, wheel_y, it );
                } else {
                    ++it;
                }
            }
        }
    }

    int last_turn_dec = 1;
    if (veh->last_turn < 0) {
        veh->last_turn += last_turn_dec;
        if (veh->last_turn > -last_turn_dec) { veh->last_turn = 0; }
    } else if (veh->last_turn > 0) {
        veh->last_turn -= last_turn_dec;
        if (veh->last_turn < last_turn_dec) { veh->last_turn = 0; }
    }

    if (can_move) {
        // accept new direction
        if (veh->skidding) {
            veh->face.init (veh->turn_dir);
            if(pl_ctrl) {
                veh->possibly_recover_from_skid();
            }
        } else {
            veh->face = mdir;
        }
        veh->move = mdir;
        if (coll_turn) {
            veh->skidding = true;
            veh->turn (coll_turn);
        }
        // accept new position
        // if submap changed, we need to process grid from the beginning.
        displace_vehicle (x, y, dx, dy);
    } else { // can_move
        veh->stop();
    }
    // If the PC is in the currently moved vehicle, adjust the
    // view offset.
    if (g->u.controlling_vehicle && veh_at(g->u.posx, g->u.posy) == veh) {
        g->calc_driving_offset(veh);
    }
    // redraw scene
    g->draw();
    return true;
}

bool map::displace_water (const tripoint &p)
{
    if (has_flag("SWIMMABLE", p) && !has_flag(TFLAG_DEEP_WATER, p)) // shallow water
    { // displace it
        int dis_places = 0, sel_place = 0;
        for (int pass = 0; pass < 2; pass++)
        { // we do 2 passes.
        // first, count how many non-water places around
        // then choose one within count and fill it with water on second pass
            if (pass)
            {
                sel_place = rng (0, dis_places - 1);
                dis_places = 0;
            }
            for (int tx = -1; tx <= 1; tx++)
                for (int ty = -1; ty <= 1; ty++)
                {
                    const tripoint t(p.x + tx, p.y + ty, p.z);
                    if ((!tx && !ty)
                            || move_cost_ter_furn(t) == 0
                            || has_flag(TFLAG_DEEP_WATER, t))
                        continue;
                    ter_id ter0 = ter (t);
                    if (ter0 == t_water_sh ||
                        ter0 == t_water_dp)
                        continue;
                    if (pass && dis_places == sel_place)
                    {
                       ter_set(t, t_water_sh);
                       ter_set(p, t_dirt);
                        return true;
                    }
                    dis_places++;
                }
        }
    }
    return false;
}

void map::set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture)
{
    set(tripoint(x, y, 0), new_terrain, new_furniture);
}

void map::set(const tripoint &p, const ter_id new_terrain, const furn_id new_furniture)
{
    furn_set(p, new_furniture);
    ter_set(p, new_terrain);
}

void map::set(const int x, const int y, const std::string new_terrain, const std::string new_furniture) {
    set(tripoint(x, y, 0), new_terrain, new_furniture);
}

void map::set(const tripoint &p, const std::string new_terrain, const std::string new_furniture) {
    furn_set(p, new_furniture);
    ter_set(p, new_terrain);
}

std::string map::name(const int x, const int y) const
{
    return name(tripoint(x, y, 0));
}

std::string map::name(const tripoint &p) const
{
    return has_furn(p) ? furn_at(p).name : ter_at(p).name;
}

bool map::has_furn(const int x, const int y) const
{
    return has_furn(tripoint(x, y, 0));
}

bool map::has_furn(const tripoint &p) const
{
  return furn(p) != f_null;
}


std::string map::get_furn(const int x, const int y) const {
    return get_furn(tripoint(x, y, 0));
}

std::string map::get_furn(const tripoint &p) const {
    return furn_at(p).id;
}

furn_t & map::furn_at(const int x, const int y) const
{
    return furn_at(tripoint(x, y, 0));
}

furn_t & map::furn_at(const tripoint &p) const
{
    return furnlist[ furn(p) ];
 }

furn_id map::furn(const int x, const int y) const
{
    return furn(tripoint(x, y, 0));
}

furn_id map::furn(const tripoint &p) const
{
    if (!inbounds(p)) {
        return f_null;
    }
    int lx, ly;
    const submap *sm = get_submap_at( p, lx, ly );
    return sm->frn[lx][ly];
}


void map::furn_set(const int x, const int y, const furn_id new_furniture)
{
    furn_set(tripoint(x, y, 0), new_furniture);
 }

void map::furn_set(const tripoint &p, const furn_id new_furniture)
{
    if (!inbounds(p)) {
        return;
    }
    int lx, ly;
    submap *sm = get_submap_at( p, lx, ly );
    sm->frn[lx][ly] = new_furniture;
    const int nonant = get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) );
    // set the dirty flags
    // TODO: consider checking if the transparency value actually changes
    // TODO: Z make this a bit more specific,
    // TODO: Z avoid it alltogether in some cases
    psm_cache[nonant].vflags = 0;
    set_transparency_cache_dirty();
    sm->frn[lx][ly] = new_furniture;
}

void map::furn_set(const int x, const int y, const std::string new_furniture) {
    furn_set(tripoint(x, y, 0), new_furniture);
}

void map::furn_set(const tripoint &p, const std::string new_furniture) {
    if ( furnmap.find(new_furniture) == furnmap.end() ) {
        return;
    }
    furn_set(p, (furn_id) furnmap[ new_furniture ].loadid );
}

bool map::can_move_furniture( const int x, const int y, player * p ) const {
    return can_move_furniture(tripoint(x, y, 0), p);
}

bool map::can_move_furniture( const tripoint &pnt, player * p ) const {
    const furn_t &furniture_type = furn_at(pnt);
    int required_str = furniture_type.move_str_req;

    // Object can not be moved (or nothing there)
    if (required_str < 0) { return false; }

    if( p != NULL && p->str_cur < required_str ) { return false; }

    return true;
}

std::string map::furnname(const int x, const int y) const {
    return furnname(tripoint(x, y, 0));
}

std::string map::furnname(const tripoint &p) const {
    return furn_at(p).name;
}
/*
 * Get the terrain integer id. This is -not- a number guaranteed to remain
 * the same across revisions; it is a load order, and can change when mods
 * are loaded or removed. The old t_floor style constants will still work but
 * are -not- guaranteed; if a mod removes t_lava, t_lava will equal t_null;
 * New terrains added to the core game generally do not need this, it's
 * retained for high performance comparisons, save/load, and gradual transition
 * to string terrain.id
 */
ter_id map::ter(const int x, const int y) const {
    return ter(tripoint(x, y, 0));
 }

ter_id map::ter(const tripoint &p) const {
    if (!inbounds(p)) {
        return t_null;
    }
    int lx, ly;
    const submap *sm = get_submap_at( p, lx, ly );
    return sm->ter[lx][ly];
}

/*
 * Get the terrain string id. This will remain the same across revisions,
 * unless a mod eliminates or changes it. Generally this is less efficient
 * than ter_id, but only an issue if thousands of comparisons are made.
 */
std::string map::get_ter(const int x, const int y) const {
    return get_ter(tripoint(x, y, 0));
}

std::string map::get_ter(const tripoint &p) const {
    return ter_at(p).id;
}

/*
 * Get the terrain harvestable string (what will get harvested from the terrain)
 */
std::string map::get_ter_harvestable(const int x, const int y) const {
    return ter_at(x, y).harvestable;
}

/*
 * Get the terrain transforms_into id (what will the terrain transforms into)
 */
ter_id map::get_ter_transforms_into(const int x, const int y) const {
    return (ter_id)termap[ ter_at(x, y).transforms_into ].loadid;
}

/*
 * Get the harvest season from the terrain
 */
int map::get_ter_harvest_season(const int x, const int y) const {
    return ter_at(x, y).harvest_season;
}

/*
 * Get a reference to the actual terrain struct.
 */
ter_t & map::ter_at(const int x, const int y) const
{
    return ter_at(tripoint(x, y, 0));
}

ter_t & map::ter_at(const tripoint &p) const
{
    return terlist[ ter(p) ];
}

/*
 * set terrain via string; this works for -any- terrain id
 */
void map::ter_set(const int x, const int y, const std::string new_terrain) {
    ter_set(tripoint(x, y, 0), new_terrain);
}

void map::ter_set(const tripoint &p, const std::string new_terrain) {
    if ( termap.find(new_terrain) == termap.end() ) {
        return;
    }
    ter_set(p, (ter_id)termap[ new_terrain ].loadid );
}

/*
 * set terrain via builtin t_keyword; only if defined, and will not work
 * for mods
 */
void map::ter_set(const int x, const int y, const ter_id new_terrain) {
    ter_set(tripoint(x, y, 0), new_terrain);
}

void map::ter_set(const tripoint &p, const ter_id new_terrain) {
    if (!inbounds(p)) {
        return;
    }

    // set the dirty flags
    // TODO: consider checking if the transparency value actually changes
    set_transparency_cache_dirty();
    set_outside_cache_dirty();

    int lx, ly;
    submap *sm = get_submap_at( p, lx, ly );
    sm->ter[lx][ly] = new_terrain;
    const int nonant = get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) );
    // TODO: Z make this a bit more specific,
    // TODO: Z avoid it alltogether in some cases
    psm_cache[nonant].vflags = 0;
}

std::string map::tername(const int x, const int y) const
{
    return tername(tripoint(x, y, 0));
}

std::string map::tername(const tripoint &p) const
{
    return ter_at(p).name;
}

std::string map::features(const int x, const int y) const
{
    return features( tripoint( x, y, 0 ) );
}

std::string map::features(const tripoint &p) const
{
    // This is used in an info window that is 46 characters wide, and is expected
    // to take up one line.  So, make sure it does that.
    // FIXME: can't control length of localized text.
    // Make the caller wrap properly, if it does not already.
    std::string ret;
    if (is_bashable(p)) {
        ret += _("Smashable. ");
    }
    if (has_flag("DIGGABLE", p)) {
        ret += _("Diggable. ");
    }
    if (has_flag("ROUGH", p)) {
        ret += _("Rough. ");
    }
    if (has_flag("UNSTABLE", p)) {
        ret += _("Unstable. ");
    }
    if (has_flag("SHARP", p)) {
        ret += _("Sharp. ");
    }
    if (has_flag("FLAT", p)) {
        ret += _("Flat. ");
    }
    return ret;
}

int map::move_cost(const int x, const int y, const vehicle *ignored_vehicle) const
{
    return move_cost(tripoint(x, y, 0), ignored_vehicle);
}

int map::move_cost(const tripoint &p, const vehicle *ignored_vehicle) const
{
    int cost = move_cost_ter_furn(p); // covers !inbounds check too
    if ( cost == 0 ) {
        return 0;
    }
    if (veh_in_active_range && veh_exists_at[p.x][p.y][p.y][p.z - my_ZMIN]) {
        const auto it = veh_cached_parts.find( p );
        if( it != veh_cached_parts.end() ) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh != ignored_vehicle) {  // moving past vehicle cost
                const int dpart = veh->part_with_feature(vpart, VPFLAG_OBSTACLE);
                if (dpart >= 0 && (!veh->part_flag(dpart, VPFLAG_OPENABLE) || !veh->parts[dpart].open)) {
                    return 0;
                } else {
                    const int ipart = veh->part_with_feature(vpart, VPFLAG_AISLE);
                    if (ipart >= 0) {
                        return 2;
                    }
                    return 8;
                }
            }
        }
    }
//    cost+= field_at(p).move_cost(); // <-- unimplemented in all cases
    return cost > 0 ? cost : 0;
}

int map::move_cost_ter_furn(const int x, const int y) const
{
    return move_cost_ter_furn(tripoint(x, y, 0));
}

int map::move_cost_ter_furn(const tripoint &p) const
{
    if (!inbounds(p)) {
        return 0;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    const int tercost = terlist[ current_submap->ter[lx][ly] ].movecost;
    if ( tercost == 0 ) {
        return 0;
    }
    const int furncost = furnlist[ current_submap->get_furn(lx, ly) ].movecost;
    if ( furncost < 0 ) {
        return 0;
    }
    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

int map::combined_movecost(const int x1, const int y1,
                           const int x2, const int y2,
                           const vehicle *ignored_vehicle, const int modifier) const
{
    return combined_movecost(
        tripoint(x1, y1, 0), tripoint(x2, y2, 0), ignored_vehicle, modifier);
}

int map::combined_movecost(const tripoint &p1,
                           const tripoint &p2,
                           const vehicle *ignored_vehicle, const int modifier) const
{
    int cost1 = move_cost(p1, ignored_vehicle);
    int cost2 = move_cost(p2, ignored_vehicle);
    // 50 moves taken per move_cost (70.71.. diagonally)
    int mult = (trigdist && p1.x != p2.x && p1.y != p2.y ? 71 : 50);
    return (cost1 + cost2 + modifier) * mult / 2;
}

bool map::has_flag(const std::string &flag, const int x, const int y) const
{
    return has_flag(flag, tripoint(x, y, 0));
}

bool map::has_flag(const std::string &flag, const tripoint &p) const
{
    static const std::string flag_str_REDUCE_SCENT("REDUCE_SCENT"); // construct once per runtime, slash delay 90%
    if (!inbounds(p)) {
        return false;
    }
    // veh_at const no bueno
    if (veh_in_active_range && veh_exists_at[p.x][p.y][p.z - my_ZMIN && flag_str_REDUCE_SCENT == flag) {
        const auto it = veh_cached_parts.find( p );
        if( it != veh_cached_parts.end() ) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    return has_flag_ter_or_furn(flag, p);
}

bool map::can_put_items(const int x, const int y) const
{
    return can_put_items(tripoint(x, y, 0));
}

bool map::can_put_items(const tripoint &p) const
{
    return !has_flag("NOITEM", p) && !has_flag("SEALED", p);
}

bool map::has_flag_ter(const std::string & flag, const int x, const int y) const
{
    return has_flag_ter(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter(const std::string & flag, const tripoint &p) const
{
    return ter_at(p).has_flag(flag);
}

bool map::has_flag_furn(const std::string & flag, const int x, const int y) const
{
    return has_flag_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_furn(const std::string & flag, const tripoint &p) const
{
    if (inbounds(p)) {
        return false;
    }
    return furn_at(p).has_flag(flag);
}

bool map::has_flag_ter_or_furn(const std::string & flag, const int x, const int y) const
{
    return has_flag_ter_or_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter_or_furn(const std::string & flag, const tripoint &p) const
{
    return has_flag_furn(flag, p) || has_flag_ter(flag, p);
}

bool map::has_flag_ter_and_furn(const std::string & flag, const int x, const int y) const
{
    return has_flag_ter_and_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter_and_furn(const std::string & flag, const tripoint &p) const
{
    return ter_at(p).has_flag(flag) && furn_at(p).has_flag(flag);
}
/////
bool map::has_flag(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag(flag, tripoint(x, y, 0));
}

bool map::has_flag(const ter_bitflags flag, const tripoint &p) const
{
    if (!inbounds(p)) {
        return false;
    }
    // veh_at const no bueno
    if (veh_in_active_range && veh_exists_at[p.x][p.y][p.z - my_ZMIN && flag == TFLAG_REDUCE_SCENT) {
        const auto it = veh_cached_parts.find( p );
        if( it != veh_cached_parts.end() ) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    return has_flag_ter_or_furn(flag, p);
}

bool map::has_flag_ter(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag_ter(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter(const ter_bitflags flag, const tripoint &p) const
{
    return ter_at(p).has_flag(flag);
}

bool map::has_flag_furn(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_furn(const ter_bitflags flag, const tripoint &p) const
{
    return furn_at(p).has_flag(flag);
}

bool map::has_flag_ter_or_furn(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag_ter_or_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter_or_furn(const ter_bitflags flag, const tripoint &p) const
{
    return ter_at(p).has_flag(flag) || furn_at(p).has_flag(flag);
}

bool map::has_flag_ter_and_furn(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag_ter_and_furn(flag, tripoint(x, y, 0));
}

bool map::has_flag_ter_and_furn(const ter_bitflags flag, const tripoint &p) const
{
    return ter_at(p).has_flag(flag) && furn_at(p).has_flag(flag);
}

/////
bool map::is_bashable(const int x, const int y) const
{
    return is_bashable(tripoint(x, y, 0));
}

bool map::is_bashable(const tripoint &p) const
{
    if (!inbounds(p)) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at " << p;
        return false;
    }

    if (veh_in_active_range && veh_exists_at[p.x][p.y][p.z - my_ZMIN) {
        const auto it = veh_cached_parts.find( p );
        if( it != veh_cached_parts.end() ) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    if ( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
        return true;
    } else if ( ter_at(p).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_ter(const int x, const int y) const
{
    return is_bashable_ter( tripoint( x, y, 0 ) );
}

bool map::is_bashable_ter(const tripoint &p) const
{
    if ( ter_at(p).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_furn(const int x, const int y) const
{
    return is_bashable_furn(tripoint(x, y, 0));
}

bool map::is_bashable_furn(const tripoint &p) const
{
    if ( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_ter_furn(const int x, const int y) const
{
    return is_bashable_ter_furn(tripoint(x, y, 0));
}

bool map::is_bashable_ter_furn(const tripoint &p) const
{
    return is_bashable_furn(p) || is_bashable_ter(p);
}

int map::bash_strength(const int x, const int y) const
{
    return bash_strength(tripoint(x, y, 0));
}

int map::bash_strength(const tripoint &p) const
{
    if ( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
        return furn_at(p).bash.str_max;
    } else if ( ter_at(p).bash.str_max != -1 ) {
        return ter_at(p).bash.str_max;
    }
    return -1;
}

int map::bash_resistance(const int x, const int y) const
{
    return bash_resistance(tripoint(x, y, 0));
}

int map::bash_resistance(const tripoint &p) const
{
    if ( has_furn(p) && furn_at(p).bash.str_min != -1 ) {
        return furn_at(p).bash.str_min;
    } else if ( ter_at(p).bash.str_min != -1 ) {
        return ter_at(p).bash.str_min;
    }
    return -1;
}

int map::bash_rating(const int str, const int x, const int y) const
{
    return bash_rating(str, tripoint(x, y, 0));
}

int map::bash_rating(const int str, const tripoint &p) const
{
    if (!is_bashable(p)) {
        return -1;
    }
    bool furn_smash = false;
    bool ter_smash = false;
    if ( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
        furn_smash = true;
    } else if ( ter_at(p).bash.str_max != -1 ) {
        ter_smash = true;
    }

    if (!furn_smash && !ter_smash) {
    //There must be a vehicle there!
        return 10;
    }

    int bash_min = 0;
    int bash_max = 0;
    if (furn_smash) {
        bash_min = furn_at(p).bash.str_min;
        bash_max = furn_at(p).bash.str_max;
    } else {
        bash_min = ter_at(p).bash.str_min;
        bash_max = ter_at(p).bash.str_max;
    }
    if (str < bash_min) {
        return 0;
    } else if (str >= bash_max) {
        return 10;
    }

    return (10 * (str - bash_min)) / (bash_max - bash_min);
}

void map::make_rubble(const int x, const int y, furn_id rubble_type, bool items, ter_id floor_type, bool overwrite)
{
    make_rubble(tripoint(x, y, 0), rubble_type, items, floor_type, overwrite);
}

void map::make_rubble(const tripoint &p, furn_id rubble_type, bool items, ter_id floor_type, bool overwrite)
{
    if (overwrite) {
        ter_set(p, floor_type);
        furn_set(p, rubble_type);
    } else {
        // First see if there is existing furniture to destroy
        if (is_bashable_furn(p)) {
            destroy_furn(p, true);
        }
        // Leave the terrain alone unless it interferes with furniture placement
        if (move_cost(p) <= 0 && is_bashable_ter(p)) {
            destroy(p, true);
        }
        // Check again for new terrain after potential destruction
        if (move_cost(p) <= 0) {
            ter_set(p, floor_type);
        }

        furn_set(p, rubble_type);
    }
    if (items) {
        //Still hardcoded, but a step up from the old stuff due to being in only one place
        if (rubble_type == f_wreckage) {
            item chunk("steel_chunk", calendar::turn);
            item scrap("scrap", calendar::turn);
            item pipe("pipe", calendar::turn);
            item wire("wire", calendar::turn);
            add_item_or_charges(p, chunk);
            add_item_or_charges(p, scrap);
            if (one_in(5)) {
                add_item_or_charges(p, pipe);
                add_item_or_charges(p, wire);
            }
        } else if (rubble_type == f_rubble_rock) {
            item rock("rock", calendar::turn);
            int rock_count = rng(1, 3);
            for (int i = 0; i < rock_count; i++) {
                add_item_or_charges(p, rock);
            }
        } else if (rubble_type == f_rubble) {
            item splinter("splinter", calendar::turn);
            item nail("nail", calendar::turn);
            int splinter_count = rng(2, 8);
            int nail_count = rng(5, 10);
            for (int i = 0; i < splinter_count; i++) {
                add_item_or_charges(p, splinter);
            }
            for (int i = 0; i < nail_count; i++) {
                add_item_or_charges(p, nail);
            }
        }
    }
}

/**
 * Returns whether or not the terrain at the given location can be dived into
 * (by monsters that can swim or are aquatic or nonbreathing).
 * @param x The x coordinate to look at.
 * @param y The y coordinate to look at.
 * @return true if the terrain can be dived into; false if not.
 */
bool map::is_divable(const int x, const int y) const
{
    return is_divable(tripoint(x, y, 0));
}

bool map::is_divable(const tripoint &p) const
{
    return has_flag("SWIMMABLE", p) && has_flag(TFLAG_DEEP_WATER, p);
}

bool map::is_outside(const int x, const int y) const
{
    return is_outside(tripoint(x, y, 0));
}

bool map::is_outside(const tripoint &p) const
{
    if(p.z > 0) {
        // For now, TODO: Z
        return true;
    } else if(p.z < 0) {
        // For now, TODO: Z
        return false;
    }
    if(!inbounds(p)) {
        return true;
    }
    return outside_cache[p.x][p.y];
}

// MATERIALS-TODO: Use fire resistance
bool map::flammable_items_at(const int x, const int y)
{
    return flammable_items_at(tripoint(x, y, 0));
}

bool map::flammable_items_at(const tripoint &p)
{
    for (auto &i : i_at(p)) {
        int vol = i.volume();
        if (i.made_of("paper") || i.made_of("powder") ||
              i.type->id == "whiskey" || i.type->id == "vodka" ||
              i.type->id == "rum" || i.type->id == "tequila" ||
              i.type->id == "single_malt_whiskey" || i.type->id == "gin" ||
              i.type->id == "moonshine" || i.type->id == "brandy") {
            return true;
        }
        if ((i.made_of("wood") || i.made_of("veggy")) && (i.burnt < 1 || vol <= 10)) {
            return true;
        }
        if ((i.made_of("cotton") || i.made_of("wool")) && (vol <= 5 || i.burnt < 1)) {
            return true;
        }
        if (i.is_ammo() && i.ammo_type() != "battery" &&
              i.ammo_type() != "nail" && i.ammo_type() != "BB" &&
              i.ammo_type() != "bolt" && i.ammo_type() != "arrow" &&
              i.ammo_type() != "pebble" && i.ammo_type() != "fishspear" &&
              i.ammo_type() != "NULL") {
            return true;
        }
    }
    return false;
}

bool map::moppable_items_at(const int x, const int y)
{
    for (auto &i : i_at(x, y)) {
        if (i.made_of(LIQUID)) {
            return true;
        }
    }
    const field &fld = field_at(x, y);
    if(fld.findField(fd_blood) != 0 || fld.findField(fd_blood_veggy) != 0 ||
          fld.findField(fd_blood_insect) != 0 || fld.findField(fd_blood_invertebrate) != 0
          || fld.findField(fd_gibs_flesh) != 0 || fld.findField(fd_gibs_veggy) != 0 ||
          fld.findField(fd_gibs_insect) != 0 || fld.findField(fd_gibs_invertebrate) != 0
          || fld.findField(fd_bile) != 0 || fld.findField(fd_slime) != 0 ||
          fld.findField(fd_sludge) != 0) {
        return true;
    }
    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative(veh->parts[vpart].mount_dx, veh->parts[vpart].mount_dy);
        for(auto &i : parts_here) {
            if(veh->parts[i].blood > 0) {
                return true;
            }
        }
    }
    return false;
}

point map::random_outdoor_tile()
{
 std::vector<point> options;
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 if (options.empty()) // Nowhere is outdoors!
  return point(-1, -1);

 return options[rng(0, options.size() - 1)];
}

bool map::has_adjacent_furniture(const int x, const int y)
{
    return has_adjacent_furniture(tripoint(x, y, 0));
}

bool map::has_adjacent_furniture(const tripoint &p)
{
    static const int cx[4] = { 0, -1, 0, 1};
    static const int cy[4] = {-1,  0, 1, 0};
    for (int i = 0; i < 4; i++)
    {
        const tripoint pn(p.x + cx[i], p.y + cy[i], p.z);
        if ( has_furn(pn) && furn_at(pn).has_flag("BLOCKSDOOR") ) {
            return true;
        }
    }
 return false;
}

bool map::has_nearby_fire(int x, int y, int radius)
{
    for(int dx = -radius; dx <= radius; dx++) {
        for(int dy = -radius; dy <= radius; dy++) {
            const point p(x + dx, y + dy);
            if( get_field( p, fd_fire ) != nullptr ) {
                return true;
            }
            if (ter(p.x, p.y) == t_lava) {
                return true;
            }
        }
    }
    return false;
}

void map::mop_spills(const int x, const int y) {
    for( auto it = i_at(x, y).begin(); it != i_at(x, y).end(); ) {
        if( it->made_of(LIQUID) ) {
            it = i_rem( x, y, it );
        } else {
            it++;
        }
    }
    remove_field( x, y, fd_blood );
    remove_field( x, y, fd_blood_veggy );
    remove_field( x, y, fd_blood_insect );
    remove_field( x, y, fd_blood_invertebrate );
    remove_field( x, y, fd_gibs_flesh );
    remove_field( x, y, fd_gibs_veggy );
    remove_field( x, y, fd_gibs_insect );
    remove_field( x, y, fd_gibs_invertebrate );
    remove_field( x, y, fd_bile );
    remove_field( x, y, fd_slime );
    remove_field( x, y, fd_sludge );
    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative( veh->parts[vpart].mount_dx,
                                                              veh->parts[vpart].mount_dy );
        for( auto &elem : parts_here ) {
            veh->parts[elem].blood = 0;
        }
    }
}

void map::create_spores(const int x, const int y, Creature* source)
{
    // TODO: Infect NPCs?
    monster spore(GetMType("mon_spore"));
    int mondex;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            mondex = g->mon_at(i, j);
            if (move_cost(i, j) > 0 || (i == x && j == y)) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(i, j) &&
                        !g->zombie(mondex).type->in_species("FUNGUS")) {
                        add_msg(_("The %s is covered in tiny spores!"),
                                g->zombie(mondex).name().c_str());
                    }
                    monster &critter = g->zombie( mondex );
                    if( !critter.make_fungus() ) {
                        critter.die( source ); // counts as kill by player
                    }
                } else if (g->u.posx == i && g->u.posy == j) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.add_env_effect("spores", bp_head, 3, 90, bp_head)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.add_env_effect("spores", bp_torso, 3, 90, bp_torso)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_l, 3, 90, bp_arm_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_r, 3, 90, bp_arm_r)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_l, 3, 90, bp_leg_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_r, 3, 90, bp_leg_r)) {
                        hit = true;
                    }
                    if (hit) {
                        add_msg(m_warning, _("You're covered in tiny spores!"));
                    }
                } else if (((i == x && j == y) || one_in(4)) &&
                           g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(i, j);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

int map::collapse_check(const int x, const int y)
{
    int num_supports = 0;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (i == x && j == y) {
                continue;
            }
            if (has_flag("COLLAPSES", x, y)) {
                if (has_flag("COLLAPSES", i, j)) {
                    num_supports++;
                } else if (has_flag("SUPPORTS_ROOF", i, j)) {
                    num_supports += 2;
                }
            } else if (has_flag("SUPPORTS_ROOF", x, y)) {
                if (has_flag("SUPPORTS_ROOF", i, j) && !has_flag("COLLAPSES", i, j)) {
                    num_supports += 3;
                }
            }
        }
    }
    return 1.7 * num_supports;
}

void map::collapse_at(const int x, const int y)
{
    destroy (x, y, false);
    crush(x, y);
    make_rubble(x, y);
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if ((i == x && j == y)) {
                continue;
            }
            if (has_flag("COLLAPSES", i, j) && one_in(collapse_check(i, j))) {
                destroy (i, j, false);
            // We only check for rubble spread if it doesn't already collapse to prevent double crushing
            } else if (has_flag("FLAT", i, j) && one_in(8)) {
                crush(i, j);
                make_rubble(i, j);
            }
        }
    }
}

std::pair<bool, bool> map::bash(const int x, const int y, const int str,
                                bool silent, bool destroy, vehicle *bashing_vehicle )
{
    return bash(tripoint(x, y, 0), str, silent, destroy, bashing_vehicle);
}

std::pair<bool, bool> map::bash(const tripoint &p, const int str, bool silent, bool destroy, vehicle *bashing_vehicle)
{
    bool success = false;
    int sound_volume = 0;
    std::string sound;
    bool smashed_something = false;
    if( get_field( p, fd_web ) != nullptr ) {
        smashed_something = true;
        remove_field(p, fd_web);
    }

    // Destroy glass items, spilling their contents.
    std::vector<item> smashed_contents;
    auto &bashed_items = i_at(p);
    for( auto bashed_item = bashed_items.begin(); bashed_item != bashed_items.end(); ) {
        // the check for active supresses molotovs smashing themselves with their own explosion
        if (bashed_item->made_of("glass") && !bashed_item->active && one_in(2)) {
            sound = _("glass shattering");
            sound_volume = 12;
            smashed_something = true;
            for( auto bashed_content : bashed_item->contents ) {
                smashed_contents.push_back( bashed_content );
            }
            bashed_item = i_rem( p, bashed_item );
        } else {
            ++bashed_item;
        }
    }
    // Now plunk in the contents of the smashed items.
    spawn_items( p.x, p.y, smashed_contents );

    // Smash vehicle if present
    int vpart;
    vehicle *veh = veh_at(p, vpart);
    if (veh && veh != bashing_vehicle) {
        veh->damage (vpart, str, 1);
        sound = _("crash!");
        sound_volume = 18;
        smashed_something = true;
        success = true;
    } else {
        // Else smash furniture or terrain
        bool smash_furn = false;
        bool smash_ter = false;
        map_bash_info *bash = NULL;

        if ( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
            bash = &(furn_at(p).bash);
            smash_furn = true;
        } else if ( ter_at(p).bash.str_max != -1 ) {
            bash = &(ter_at(p).bash);
            smash_ter = true;
        }
        // TODO: what if silent is true? What if this was done by a hulk, not the player?
        if (has_flag("ALARMED", p) && !g->event_queued(EVENT_WANTED)) {
            g->sound(p.x, p.y, 40, _("An alarm sounds!"));
            g->u.add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                  pgettext("memorial_female", "Set off an alarm."));
            g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->get_abs_levx(), g->get_abs_levy());
        }

        if ( bash != NULL && (!bash->destroy_only || destroy)) {
            int smin = bash->str_min;
            int smax = bash->str_max;
            int sound_vol = bash->sound_vol;
            int sound_fail_vol = bash->sound_fail_vol;
            if (destroy) {
                success = true;
            } else {
                if ( bash->str_min_blocked != -1 || bash->str_max_blocked != -1 ) {
                    if( has_adjacent_furniture(p) ) {
                        if ( bash->str_min_blocked != -1 ) {
                            smin = bash->str_min_blocked;
                        }
                        if ( bash->str_max_blocked != -1 ) {
                            smax = bash->str_max_blocked;
                        }
                    }
                }
                if ( str >= smin && str >= rng(bash->str_min_roll, bash->str_max_roll)) {
                    success = true;
                }
            }

            if (success || destroy) {
                // Clear out any partially grown seeds
                if (has_flag_ter_or_furn("PLANT", p)) {
                    for (size_t i = 0; i < i_at(p).size(); i++) {
                        i_rem(p, i);
                    }
                }

                if (smash_furn) {
                    if (has_flag_furn("FUNGUS", p)) {
                        create_spores(p.x, p.y);
                    }
                } else if (smash_ter) {
                    if (has_flag_ter("FUNGUS", p)) {
                        create_spores(p.x, p.y);
                    }
                }

                if (destroy) {
                    sound_volume = smax;
                } else {
                    if (sound_vol == -1) {
                        sound_volume = std::min(int(smin * 1.5), smax);
                    } else {
                        sound_volume = sound_vol;
                    }
                }
                sound = _(bash->sound.c_str());
                // Set this now in case the ter_set below changes this
                bool collapses = has_flag("COLLAPSES", p) && smash_ter;
                bool supports = has_flag("SUPPORTS_ROOF", p) && smash_ter;
                if (smash_furn == true) {
                    furn_set(p, bash->furn_set);
                    // Hack alert.
                    // Signs have cosmetics associated with them on the submap since
                    // furniture can't store dynamic data to disk. To prevent writing
                    // mysteriously appearing for a sign later built here, remove the
                    // writing from the submap.
                    delete_signage(p);
                } else if (smash_ter == true) {
                    ter_set(p, bash->ter_set);
                } else {
                    debugmsg( "data/json/terrain.json does not have %s.bash.ter_set set!",
                              ter_at(p).id.c_str() );
                }

                spawn_item_list(bash->items, p.x, p.y);
                if (bash->explosive > 0) {
                    g->explosion(p.x, p.y, bash->explosive, 0, false);
                }

                if (collapses) {
                    collapse_at(p.x, p.y);
                }
                // Check the flag again to ensure the new terrain doesn't support anything
                if (supports && !has_flag("SUPPORTS_ROOF", p)) {
                    for (int i = p.x - 1; i <= p.x + 1; i++) {
                        for (int j = p.y - 1; j <= p.y + 1; j++) {
                            const tripoint p2( i, j, p.z );
                            if (p2 == p || !has_flag("COLLAPSES", p2)) {
                                continue;
                            }
                            if (one_in(collapse_check(p2.x, p2.y))) {
                                collapse_at(p2.x, p2.y);
                            }
                        }
                    }
                }
                smashed_something = true;
            } else {
                if (sound_fail_vol == -1) {
                    sound_volume = 12;
                } else {
                    sound_volume = sound_fail_vol;
                }
                sound = _(bash->sound_fail.c_str());
                smashed_something = true;
            }
        } else {
            furn_id furnid = furn(p);
            if ( furnid == f_skin_wall || furnid == f_skin_door || furnid == f_skin_door_o ||
                 furnid == f_skin_groundsheet || furnid == f_canvas_wall || furnid == f_canvas_door ||
                 furnid == f_canvas_door_o || furnid == f_groundsheet || furnid == f_fema_groundsheet) {
                if (str >= rng(0, 6) || destroy) {
                    // Special code to collapse the tent if destroyed
                    tripoint tent(-1, -1, -1);
                    // Find the center of the tent
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            const tripoint pn(p.x + i, p.y + j, p.z);
                            if (furn(pn) == f_groundsheet ||
                                furn(pn) == f_fema_groundsheet ||
                                furn(pn) == f_skin_groundsheet){
                                tent = pn;
                                break;
                            }
                        }
                    }
                    // Never found tent center, bail out
                    if (tent.x == -1) {
                        smashed_something = true;
                    }
                    // Take the tent down
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            const tripoint pn(tent.x + i, tent.y + j, tent.z);
                            if (furn(pn) == f_groundsheet) {
                                spawn_item(pn, "broketent");
                            }
                            if (furn(pn) == f_skin_groundsheet) {
                                spawn_item(pn, "damaged_shelter_kit");
                            }
                            furn_id check_furn = furn(pn);
                            if (check_furn == f_skin_wall || check_furn == f_skin_door ||
                                  check_furn == f_skin_door_o || check_furn == f_skin_groundsheet ||
                                  check_furn == f_canvas_wall || check_furn == f_canvas_door ||
                                  check_furn == f_canvas_door_o || check_furn == f_groundsheet ||
                                  check_furn == f_fema_groundsheet) {
                                furn_set(pn, f_null);
                            }
                        }
                    }

                    sound_volume = 8;
                    sound = _("rrrrip!");
                    smashed_something = true;
                    success = true;
                } else {
                    sound_volume = 8;
                    sound = _("slap!");
                    smashed_something = true;
                }
            // Made furniture seperate from the other tent to facilitate destruction
            } else if (furnid == f_center_groundsheet || furnid == f_large_groundsheet ||
                     furnid == f_large_canvas_door || furnid == f_large_canvas_wall ||
                     furnid == f_large_canvas_door_o) {
                if (str >= rng(0, 6) || destroy) {
                    // Special code to collapse the tent if destroyed
                    tripoint tent(-1, -1, -1);
                    // Find the center of the tent
                    for (int i = -2; i <= 2; i++) {
                        for (int j = -2; j <= 2; j++) {
                            const tripoint pn(p.x + i, p.y + j, p.z);
                            if (furn(pn) == f_center_groundsheet){
                                tent = pn;
                                break;
                            }
                        }
                    }
                    // Never found tent center, bail out
                    if (tent.x == -1) {
                        smashed_something = true;
                    }
                    // Take the tent down
                    for (int i = -2; i <= 2; i++) {
                        for (int j = -2; j <= 2; j++) {
                            const tripoint pn(tent.x + i, tent.y + j, tent.z);
                            if (furn(pn) == f_center_groundsheet) {
                                spawn_item(pn, "largebroketent");
                            }
                            furn_set(pn, f_null);
                        }
                    }
                    sound_volume = 8;
                    sound = _("rrrrip!");
                    smashed_something = true;
                    success = true;
                } else {
                    sound_volume = 8;
                    sound = _("slap!");
                    smashed_something = true;
                }
            }
        }
    }
    if( move_cost(p) <= 0  && !smashed_something ) {
        sound = _("thump!");
        sound_volume = 18;
        smashed_something = true;
    }
    if( !sound.empty() && !silent) {
        g->sound( p.x, p.y, sound_volume, sound);
    }
    return std::pair<bool, bool> (smashed_something, success);
}

void map::spawn_item_list(const std::vector<map_bash_item_drop> &items, int x, int y) {
    for( auto &items_i : items ) {
        const map_bash_item_drop &drop = items_i;
        int chance = drop.chance;
        if ( chance == -1 || rng(0, 100) >= chance ) {
            int numitems = drop.amount;

            if ( drop.minamount != -1 ) {
                numitems = rng( drop.minamount, drop.amount );
            }
            if ( numitems > 0 ) {
                // spawn_item(x,y, drop.itemtype, numitems); // doesn't abstract amount || charges
                item new_item(drop.itemtype, calendar::turn);
                if ( new_item.count_by_charges() ) {
                    new_item.charges = numitems;
                    numitems = 1;
                }
                const bool varsize = new_item.has_flag( "VARSIZE" );
                for(int a = 0; a < numitems; a++ ) {
                    if( varsize && one_in( 3 ) ) {
                        new_item.item_tags.insert( "FIT" );
                    } else if( varsize ) {
                        // might have been added previously
                        new_item.item_tags.erase( "FIT" );
                    }
                    add_item_or_charges(x, y, new_item);
                }
            }
        }
    }
}

void map::destroy(const int x, const int y, const bool silent)
{
    destroy(tripoint(x, y, 0), silent);
}

void map::destroy(const tripoint &p, const bool silent)
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while (count <= 25 && bash(p, 999, silent, true).second) {
        count++;
    }
}

void map::destroy_furn(const int x, const int y, const bool silent)
{
    destroy_furn(tripoint(x, y, 0), silent);
}

void map::destroy_furn(const tripoint &p, const bool silent)
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while (count <= 25 && furn(p) != f_null && bash(p, 999, silent, true).second) {
        count++;
    }
}

void map::crush(const int x, const int y)
{
    crush( tripoint( x, y, 0 ) );
}

void map::crush(const tripoint &p)
{
    int veh_part;
    player *crushed_player = nullptr;
    //The index of the NPC at (x,y), or -1 if there isn't one
    int npc_index = g->npc_at(p);
    if( g->u.pos() == p ) {
        crushed_player = &(g->u);
    } else if( npc_index != -1 ) {
        crushed_player = static_cast<player *>(g->active_npc[npc_index]);
    }

    if( crushed_player != nullptr ) {
        bool player_inside = false;
        if( crushed_player->in_vehicle ) {
            vehicle *veh = veh_at(p, veh_part);
            player_inside = (veh && veh->is_inside(veh_part));
        }
        if (!player_inside) { //If there's a player at p and he's not in a covered vehicle...
            //This is the roof coming down on top of us, no chance to dodge
            crushed_player->add_msg_player_or_npc( m_bad, _("You are crushed by the falling debris!"),
                                                   _("<npcname> is crushed by the falling debris!") );
            int dam = rng(0, 40);
            // Torso and head take the brunt of the blow
            body_part hit = bp_head;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .25 ) );
            hit = bp_torso;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .45 ) );
            // Legs take the next most through transfered force
            hit = bp_leg_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            hit = bp_leg_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            // Arms take the least
            hit = bp_arm_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );
            hit = bp_arm_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );

            // Pin whoever got hit
            crushed_player->add_effect("crushed", 1, num_bp, true);
        }
    }

    //The index of the monster at p, or -1 if there isn't one
    int mon = g->mon_at(p);
    if (mon != -1 && size_t(mon) < g->num_zombies()) {  //If there's a monster at p...
        monster* monhit = &(g->zombie(mon));
        // 25 ~= 60 * .45 (torso)
        monhit->deal_damage(nullptr, bp_torso, damage_instance(DT_BASH, rng(0,25)));

        // Pin whoever got hit
        monhit->add_effect("crushed", 1, num_bp, true);
    }

    vehicle *veh = veh_at(p, veh_part);
    if (veh) {
        veh->damage(veh_part, rng(0, veh->parts[veh_part].hp), 1, false);
    }
}

void map::shoot(const int x, const int y, int &dam,
                const bool hit_items, const std::set<std::string>& ammo_effects)
{
    if (dam < 0)
    {
        return;
    }

    if (has_flag("ALARMED", x, y) && !g->event_queued(EVENT_WANTED))
    {
        g->sound(x, y, 30, _("An alarm sounds!"));
        g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->get_abs_levx(), g->get_abs_levy());
    }

    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if (veh)
    {
        const bool inc = (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"));
        dam = veh->damage (vpart, dam, inc? 2 : 0, hit_items);
    }

    ter_t terrain = ter_at(x, y);
    if( 0 == terrain.id.compare("t_wall_wood_broken") ||
        0 == terrain.id.compare("t_wall_log_broken") ||
        0 == terrain.id.compare("t_door_b") ) {
        if (hit_items || one_in(8)) { // 1 in 8 chance of hitting the door
            dam -= rng(20, 40);
            if (dam > 0) {
                g->sound(x, y, 10, _("crash!"));
                ter_set(x, y, t_dirt);
            }
        }
        else {
            dam -= rng(0, 1);
        }
    } else if( 0 == terrain.id.compare("t_door_c") ||
               0 == terrain.id.compare("t_door_locked") ||
               0 == terrain.id.compare("t_door_locked_peep") ||
               0 == terrain.id.compare("t_door_locked_alarm") ) {
        dam -= rng(15, 30);
        if (dam > 0) {
            g->sound(x, y, 10, _("smash!"));
            ter_set(x, y, t_door_b);
        }
    } else if( 0 == terrain.id.compare("t_door_boarded") ||
               0 == terrain.id.compare("t_door_boarded_damaged") ||
               0 == terrain.id.compare("t_rdoor_boarded") ||
               0 == terrain.id.compare("t_rdoor_boarded_damaged") ) {
        dam -= rng(15, 35);
        if (dam > 0) {
            g->sound(x, y, 10, _("crash!"));
            ter_set(x, y, t_door_b);
        }
    } else if( 0 == terrain.id.compare("t_window_domestic_taped") ||
               0 == terrain.id.compare("t_curtains") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(1, 5);
        }
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
                spawn_item(x, y, "sheet", 1);
                spawn_item(x, y, "stick");
                spawn_item(x, y, "string_36");
            }
        }
    } else if( 0 == terrain.id.compare("t_window_domestic") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
                spawn_item(x, y, "sheet", 1);
                spawn_item(x, y, "stick");
                spawn_item(x, y, "string_36");
            }
        }
    } else if( 0 == terrain.id.compare("t_window_taped") ||
               0 == terrain.id.compare("t_window_alarm_taped") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(1, 5);
        }
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
            }
        }
    } else if( 0 == terrain.id.compare("t_window") ||
               0 == terrain.id.compare("t_window_alarm") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
            }
        }
    } else if( 0 == terrain.id.compare("t_window_boarded") ) {
        dam -= rng(10, 30);
        if (dam > 0) {
            g->sound(x, y, 16, _("glass breaking!"));
            ter_set(x, y, t_window_frame);
        }
    } else if( 0 == terrain.id.compare("t_wall_glass_h") ||
               0 == terrain.id.compare("t_wall_glass_v") ||
               0 == terrain.id.compare("t_wall_glass_h_alarm") ||
               0 == terrain.id.compare("t_wall_glass_v_alarm") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0,5);
        } else {
            dam -= rng(1,8);
            if (dam > 0) {
                g->sound(x, y, 20, _("glass breaking!"));
                ter_set(x, y, t_floor);
            }
        }
    } else if( 0 == terrain.id.compare("t_reinforced_glass_v") ||
               0 == terrain.id.compare("t_reinforced_glass_h") ) {
        // reinforced glass stops most bullets
        // laser beams are attenuated
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 8);
        } else {
            //Greatly weakens power of bullets
            dam -= 40;
            if (dam <= 0) {
                add_msg(_("The shot is stopped by the reinforced glass wall!"));
            } else if (dam >= 40) {
                //high powered bullets penetrate the glass, but only extremely strong
                // ones (80 before reduction) actually destroy the glass itself.
                g->sound(x, y, 20, _("glass breaking!"));
                ter_set(x, y, t_floor);
            }
        }
    } else if( 0 == terrain.id.compare("t_paper") ) {
        dam -= rng(4, 16);
        if (dam > 0) {
            g->sound(x, y, 8, _("rrrrip!"));
            ter_set(x, y, t_dirt);
        }
        if (ammo_effects.count("INCENDIARY")) {
            add_field(x, y, fd_fire, 1);
        }
    } else if( 0 == terrain.id.compare("t_gas_pump") ) {
        if (hit_items || one_in(3)) {
            if (dam > 15) {
                if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME")) {
                    g->explosion(x, y, 40, 0, true);
                } else {
                    for (int i = x - 2; i <= x + 2; i++) {
                        for (int j = y - 2; j <= y + 2; j++) {
                            if (move_cost(i, j) > 0 && one_in(3)) {
                                    spawn_item(i, j, "gasoline");
                            }
                        }
                    }
                    g->sound(x, y, 10, _("smash!"));
                }
                ter_set(x, y, t_gas_pump_smashed);
            }
            dam -= 60;
        }
    } else if( 0 == terrain.id.compare("t_vat") ) {
        if (dam >= 10) {
            g->sound(x, y, 20, _("ke-rash!"));
            ter_set(x, y, t_floor);
        } else {
            dam = 0;
        }
    } else {
        if (move_cost(x, y) == 0 && light_transparency(x, x) == LIGHT_TRANSPARENCY_SOLID) {
            dam = 0; // TODO: Bullets can go through some walls?
        } else {
            dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
        }
    }

    if (ammo_effects.count("TRAIL") && !one_in(4)) {
        add_field(x, y, fd_smoke, rng(1, 2));
    }

    if (ammo_effects.count("STREAM") && !one_in(3)) {
        add_field(x, y, fd_fire, rng(1, 2));
    }

    if (ammo_effects.count("STREAM_BIG") && !one_in(4)) {
        add_field(x, y, fd_fire, 2);
    }

    if (ammo_effects.count("LIGHTNING")) {
        add_field(x, y, fd_electricity, rng(2, 3));
    }

    if (ammo_effects.count("PLASMA") && one_in(2)) {
        add_field(x, y, fd_plasma, rng(1, 2));
    }

    if (ammo_effects.count("LASER")) {
        add_field(x, y, fd_laser, 2);
    }

    // Set damage to 0 if it's less
    if (dam < 0) {
        dam = 0;
    }

    // Check fields?
    const field_entry *fieldhit = get_field( point( x, y ), fd_web );
    if( fieldhit != nullptr ) {
        if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME")) {
            add_field(x, y, fd_fire, fieldhit->getFieldDensity() - 1);
        } else if (dam > 5 + fieldhit->getFieldDensity() * 5 &&
                   one_in(5 - fieldhit->getFieldDensity())) {
            dam -= rng(1, 2 + fieldhit->getFieldDensity() * 2);
            remove_field(tripoint(x, y, 0),fd_web);
        }
    }

    // Now, destroy items on that tile.
    if ((move_cost(x, y) == 2 && !hit_items) || !inbounds(x, y)) {
        return; // Items on floor-type spaces won't be shot up.
    }

    for( auto target_item = i_at(x, y).begin(); target_item != i_at(x, y).end(); ) {
        bool destroyed = false;
        int chance = ( target_item->volume() > 0 ? target_item->volume() : 1);
        // volume dependent chance

        if( dam > target_item->bash_resist() && one_in(chance) ) {
            get_item(x, y, target_item)->damage++;
        }
        if( target_item->damage >= 5 ) {
            destroyed = true;
        }

        if (destroyed) {
            spawn_items( x, y, target_item->contents );
            target_item = i_rem( x, y, target_item );
        } else {
            ++target_item;
        }
    }
}

bool map::hit_with_acid( const int x, const int y )
{
    if( move_cost( x, y ) != 0 ) {
        return false;    // Didn't hit the tile!
    }
    const ter_id t = ter( x, y );
    if( t == t_wall_glass_v || t == t_wall_glass_h || t == t_wall_glass_v_alarm || t == t_wall_glass_h_alarm ||
        t == t_vat ) {
        ter_set( x, y, t_floor );
    } else if( t == t_door_c || t == t_door_locked || t == t_door_locked_peep || t == t_door_locked_alarm ) {
        if( one_in( 3 ) ) {
            ter_set( x, y, t_door_b );
        }
    } else if( t == t_door_bar_c || t == t_door_bar_o || t == t_door_bar_locked || t == t_bars ) {
        ter_set( x, y, t_floor );
        add_msg( m_warning, _( "The metal bars melt!" ) );
    } else if( t == t_door_b ) {
        if( one_in( 4 ) ) {
            ter_set( x, y, t_door_frame );
        } else {
            return false;
        }
    } else if( t == t_window || t == t_window_alarm ) {
        ter_set( x, y, t_window_empty );
    } else if( t == t_wax ) {
        ter_set( x, y, t_floor_wax );
    } else if( t == t_gas_pump || t == t_gas_pump_smashed ) {
        return false;
    } else if( t == t_card_science || t == t_card_military ) {
        ter_set( x, y, t_card_reader_broken );
    }
    return true;
}

// returns true if terrain stops fire
bool map::hit_with_fire(const int x, const int y)
{
    if (move_cost(x, y) != 0)
        return false; // Didn't hit the tile!

    // non passable but flammable terrain, set it on fire
    if (has_flag("FLAMMABLE", x, y) || has_flag("FLAMMABLE_ASH", x, y))
    {
        add_field(x, y, fd_fire, 3);
    }
    return true;
}

bool map::marlossify(const tripoint &p)
{
    if (one_in(25) && (ter_at(p).movecost != 0 && !has_furn(p))
            && !ter_at(p).has_flag(TFLAG_DEEP_WATER)) {
        ter_set(p, t_marloss);
        return true;
    }
    for (int i = 0; i < 25; i++) {
        if(!g->spread_fungus(p)) {
            return true;
        }
    }
    return false;
}

bool map::open_door(const int x, const int y, const bool inside, const bool check_only)
{
 const auto &ter = ter_at(x, y);
 const auto &furn = furn_at(x, y);
 if ( !ter.open.empty() && ter.open != "t_null" ) {
     if ( termap.find( ter.open ) == termap.end() ) {
         debugmsg("terrain %s.open == non existant terrain '%s'\n", ter.id.c_str(), ter.open.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if(!check_only) {
         ter_set(x, y, ter.open );
     }
     return true;
 } else if ( !furn.open.empty() && furn.open != "t_null" ) {
     if ( furnmap.find( furn.open ) == furnmap.end() ) {
         debugmsg("terrain %s.open == non existant furniture '%s'\n", furn.id.c_str(), furn.open.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if(!check_only) {
         furn_set(x, y, furn.open );
     }
     return true;
 }
 return false;
}

void map::translate(const ter_id from, const ter_id to)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from)
    ter_set(x, y, to);
  }
 }
}

//This function performs the translate function within a given radius of the player.
void map::translate_radius(const ter_id from, const ter_id to, float radi, int uX, int uY)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from){
    //float radiX = 0.0;
    float radiX = sqrt(float((uX-x)*(uX-x) + (uY-y)*(uY-y)));
    if (radiX <= radi){
      ter_set(x, y, to);}
    }
   }
  }
 }

bool map::close_door(const int x, const int y, const bool inside, const bool check_only)
{
 const auto &ter = ter_at(x, y);
 const auto &furn = furn_at(x, y);
 if ( !ter.close.empty() && ter.close != "t_null" ) {
     if ( termap.find( ter.close ) == termap.end() ) {
         debugmsg("terrain %s.close == non existant terrain '%s'\n", ter.id.c_str(), ter.close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if (!check_only) {
        ter_set(x, y, ter.close );
     }
     return true;
 } else if ( !furn.close.empty() && furn.close != "t_null" ) {
     if ( furnmap.find( furn.close ) == furnmap.end() ) {
         debugmsg("terrain %s.close == non existant furniture '%s'\n", furn.id.c_str(), furn.close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if (!check_only) {
         furn_set(x, y, furn.close );
     }
     return true;
 }
 return false;
}

const std::string map::get_signage(const int x, const int y) const
{
    return get_signage(tripoint(x, y, 0));
}

const std::string map::get_signage(const tripoint &p) const
{
    if (!inbounds(p)) {
        return "";
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    return current_submap->get_signage(lx, ly);
}
void map::set_signage(const int x, const int y, std::string message) const
{
    set_signage(tripoint(x, y, 0), message);
}
void map::set_signage(const tripoint &p, const std::string &message) const
{
    if (!inbounds(p)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    current_submap->set_signage(lx, ly, message);
}
void map::delete_signage(const int x, const int y) const
{
    delete_signage(tripoint(x, y, 0));
}
void map::delete_signage(const tripoint &p) const
{
    if (!inbounds(p)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    current_submap->delete_signage(lx, ly);
}

int map::get_radiation(const int x, const int y) const
{
    return get_radiation(tripoint(x, y, 0));
}

int map::get_radiation(const tripoint &p) const
{
    if (!inbounds(p)) {
        return 0;
    }
    int lx, ly;
    submap *sm = get_submap_at( p, lx, ly );
    return sm->get_radiation( lx, ly );
}

void map::set_radiation(const int x, const int y, const int value)
{
    return set_radiation(tripoint(x, y, 0), value);
}

void map::set_radiation(const tripoint &p, const int value)
{
    if (!inbounds(p)) {
        return;
    }
    int lx, ly;
    submap *sm = get_submap_at( p, lx, ly );
    sm->set_radiation( lx, ly, value);
}

void map::adjust_radiation(const int x, const int y, const int delta)
{
    adjust_radiation(tripoint(x, y, 0), delta);
}

void map::adjust_radiation(const tripoint &p, const int delta)
{
    if (!inbounds(p)) {
        return;
    }
    int lx, ly;
    submap *sm = get_submap_at( p, lx, ly );
    int current_radiation = sm->get_radiation( lx, ly );
    sm->set_radiation( lx, ly, current_radiation + delta );
}

int& map::temperature(const int x, const int y)
{
    return temperature(tripoint(x, y, 0));
}

int& map::temperature(const tripoint &p)
{
    if (!inbounds(p)) {
  null_temperature = 0;
  return null_temperature;
 }
    return get_submap_at( p )->temperature;
}

void map::set_temperature(const int x, const int y, int new_temperature)
{
    set_temperature(tripoint(x, y, 0), new_temperature);
}

void map::set_temperature(const tripoint &p, int new_temperature)
{
    temperature(p) = new_temperature;
    temperature(tripoint(p.x + SEEX, p.y, p.z)) = new_temperature;
    temperature(tripoint(p.x, p.y + SEEY, p.z)) = new_temperature;
    temperature(tripoint(p.x + SEEX, p.y + SEEY, p.z)) = new_temperature;
}

const std::vector<item> &map::i_at( const int x, const int y ) const
{
    return i_at(tripoint(x, y, 0));
}

const std::vector<item> &map::i_at( const tripoint &p) const
{
    if( !inbounds(p) ) {
        nulitems.clear();
        return nulitems;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->itm[lx][ly];
}

std::vector<item> &map::i_at_mutable( const int x, const int y )
{
    return i_at_mutable(tripoint(x, y, 0));
}

std::vector<item> &map::i_at_mutable( const tripoint &p )
{
    if( !inbounds(p) ) {
        nulitems.clear();
        return nulitems;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->itm[lx][ly];
}

item *map::get_item( const int x, const int y, const int i )
{
    return get_item( tripoint( x, y, 0 ), i );
}

item *map::get_item( const tripoint &p, const int i )
{
    auto &item_stack = i_at_mutable( p );
    if( item_stack.size() > (unsigned int)i ) {
        return &item_stack[i];
    }
    return nullptr;
}

item *map::get_item( const int x, const int y, std::vector<item>::const_iterator i )
{
    return get_item( tripoint( x, y, 0 ), i );
}

item *map::get_item( const tripoint &p, std::vector<item>::const_iterator i )
{
    int offset = std::distance( i_at(p).begin(), i );
    return get_item( p, offset );
}

itemslice map::i_stacked(std::vector<item>& items)
{
    //create a new container for our stacked items
    itemslice islice;

    //iterate through all items in the vector
    for( auto &items_it : items ) {
        if( items_it.count_by_charges() ) {
            // Those exists as a single item all the item anyway
            islice.push_back( std::make_pair( &items_it, 1 ) );
            continue;
        }
        bool list_exists = false;

        //iterate through stacked item lists
        for( auto &elem : islice ) {
            //check if the ID exists
            item *first_item = elem.first;
            if( first_item->type->id == items_it.type->id ) {
                //we've found the list of items with the same type ID

                if( first_item->stacks_with( items_it ) ) {
                    //add it to the existing list
                    elem.second++;
                    list_exists = true;
                    break;
                }
            }
        }

        if(!list_exists) {
            //insert the list into islice
            islice.push_back( std::make_pair( &items_it, 1 ) );
        }

    } //end items loop

    return islice;
}


bool map::sees_some_items(const int x, const int y, const player &u)
{
    return sees_some_items( tripoint( x, y, 0 ), u );
}

bool map::sees_some_items(const tripoint &p, const player &u)
{
    // can only see items if there are any items.
    return !i_at( p ).empty() && could_see_items( p, u );
}

bool map::could_see_items(const tripoint &p, const player &u)
{
    const bool container = has_flag_ter_or_furn("CONTAINER", p);
    const bool sealed = has_flag_ter_or_furn("SEALED", p);
    if (sealed && container) {
        // never see inside of sealed containers
        return false;
    }
    if (container) {
        // can see inside of containers if adjacent or
        // on top of the container
        return (abs(p.x - u.posx) <= 1 && abs(p.y - u.posy) <= 1 && abs(p.z - u.zpos()) <= 1);
    }
    return true;
}

item map::water_from(const int x, const int y)
{
    item ret("water", 0);
    if (ter(x, y) == t_water_sh && one_in(3))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_water_dp && one_in(4))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_sewage)
        ret.poison = rng(1, 7);
    return ret;
}
item map::swater_from(const int x, const int y)
{
    (void)x; (void)y;
    item ret("salt_water", 0);

    return ret;
}
item map::acid_from(const int x, const int y)
{
    (void)x; (void)y; //all acid is acid, i guess?
    item ret("water_acid", 0);
    return ret;
}

std::vector<item>::const_iterator map::i_rem( const int x, const int y,
                                              std::vector<item>::const_iterator it )
{
    return i_rem(tripoint(x, y, 0), it);
}

std::vector<item>::const_iterator map::i_rem( const tripoint &p,
                                              std::vector<item>::const_iterator it )
{
    int offset = std::distance( i_at(p).begin(), it );
    return i_at_mutable(p).begin() + i_rem( p, offset );
}

int map::i_rem(const int x, const int y, const int index)
{
    return i_rem(tripoint(x, y, 0), index);
}

int map::i_rem(const tripoint &p, const int index)
{
    if (index > (int)i_at(p).size() - 1) {
        return index;
    }
    return std::distance( i_at_mutable(p).begin(),
                          i_at_mutable(p).erase(i_at_mutable(p).begin() + index ) );
}

void map::i_rem(const int x, const int y, item* it)
{
    i_rem(tripoint(x, y, 0), it);
}

void map::i_rem(const tripoint &p, item* it)
{
    std::vector<item>& map_items = i_at_mutable(p);

    for( auto iter = map_items.begin(); iter < map_items.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        if(it == &*iter) {
            map_items.erase(iter);
            break;
        }
    }
}

void map::i_clear(const int x, const int y)
{
    i_clear(tripoint(x, y, 0));
}

void map::i_clear(const tripoint &p)
{
    // TODO: active item counter!
    i_at_mutable(p).clear();
}

void map::spawn_an_item(const int x, const int y, item new_item,
                        const long charges, const int damlevel)
{
    spawn_an_item(tripoint(x, y, 0), new_item, charges, damlevel);
}

void map::spawn_an_item(const tripoint &p, item new_item,
                        const long charges, const int damlevel)
{
    if( charges && new_item.charges > 0 ) {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container();
    if( (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) ||
        has_flag("DESTROY_ITEM", p) ) {
        return;
    }
    // bounds checking for damage level
    if( damlevel < -1 ) {
        new_item.damage = -1;
    } else if( damlevel > 4 ) {
        new_item.damage = 4;
    } else {
        new_item.damage = damlevel;
    }
    add_item_or_charges(p, new_item);
}

void map::spawn_items(const int x, const int y, const std::vector<item> &new_items)
{
    if (!inbounds(x, y) || has_flag("DESTROY_ITEM", x, y)) {
        return;
    }
    const bool swimmable = has_flag("SWIMMABLE", x, y);
    for( auto new_item : new_items ) {

        if (new_item.made_of(LIQUID) && swimmable) {
            continue;
        }
        if (new_item.is_armor() && new_item.has_flag("PAIRED") && x_in_y(4, 5)) {
            item new_item2 = new_item;
            new_item.make_handed( LEFT );
            new_item2.make_handed( RIGHT );
            add_item_or_charges(x, y, new_item2);
        }
        add_item_or_charges(x, y, new_item);
    }
}

void map::spawn_artifact(const int x, const int y)
{
    add_item_or_charges( x, y, item( new_artifact(), 0 ) );
}

void map::spawn_natural_artifact(const int x, const int y, artifact_natural_property prop)
{
    add_item_or_charges( x, y, item( new_natural_artifact( prop ), 0 ) );
}

//New spawn_item method, using item factory
// added argument to spawn at various damage levels
void map::spawn_item(const int x, const int y, const std::string &type_id,
                     const unsigned quantity, const long charges,
                     const unsigned birthday, const int damlevel, const bool rand)
{
    spawn_item(tripoint(x, y, 0), type_id, quantity, charges, birthday, damlevel, rand);
}

void map::spawn_item(const tripoint &p, const std::string &type_id,
                     const unsigned quantity, const long charges,
                     const unsigned birthday, const int damlevel, const bool rand)
{
    if(type_id == "null") {
        return;
    }
    if(item_is_blacklisted(type_id)) {
        return;
    }
    // recurse to spawn (quantity - 1) items
    for(unsigned i = 1; i < quantity; i++)
    {
        spawn_item(p, type_id, 1, charges, birthday, damlevel);
    }
    // spawn the item
    item new_item(type_id, birthday, rand);
    if( one_in( 3 ) && new_item.has_flag( "VARSIZE" ) ) {
        new_item.item_tags.insert( "FIT" );
    }
    spawn_an_item(p, new_item, charges, damlevel);
}

int map::max_volume(const int x, const int y) const
{
    return max_volume(tripoint(x, y, 0));
}

int map::max_volume(const tripoint &p) const
{
    const ter_t &ter = ter_at(p);
    if (has_furn(p)) {
        return furn_at(p).max_volume;
    }
    return ter.max_volume;
}

// total volume of all the things
int map::stored_volume(const int x, const int y) {
    return stored_volume(tripoint(x, y, 0));
}

int map::stored_volume(const tripoint &p) {
    if(!inbounds(p)) {
        return 0;
    }
    int cur_volume = 0;
    for( auto &n : i_at(p) ) {
        cur_volume += n.volume();
    }
    return cur_volume;
}

// free space
int map::free_volume(const int x, const int y) {
    return free_volume(tripoint(x, y, 0));
}

int map::free_volume(const tripoint &p) {
   const int maxvolume = this->max_volume(p);
   if(!inbounds(p)) return 0;
   return ( maxvolume - stored_volume(p) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool map::is_full(const int x, const int y, const int addvolume, const int addnumber ) {
   const int maxitems = MAX_ITEM_IN_SQUARE; // (game.h) 1024
   const int maxvolume = this->max_volume(x, y);

   if( ! (inbounds(x, y) && move_cost(x, y) > 0 && !has_flag("NOITEM", x, y) ) ) {
       return true;
   }

   if ( addvolume == -1 ) {
       if ( i_at(x, y).size() < maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( i_at(x, y).size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

// adds an item to map point, or stacks charges.
// returns false if item exceeds tile's weight limits or item count. This function is expensive, and meant for
// user initiated actions, not mapgen!
// overflow_radius > 0: if x,y is full, attempt to drop item up to overflow_radius squares away, if x,y is full
bool map::add_item_or_charges(const int x, const int y, item new_item, int overflow_radius) {
    return add_item_or_charges(tripoint(x, y, 0), new_item, overflow_radius);
}

bool map::add_item_or_charges(const tripoint &p, item new_item, int overflow_radius) {

    if(!inbounds(p) ) {
        return false;
    }
    if( (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) ||
            has_flag("DESTROY_ITEM", p) || new_item.has_flag("NO_DROP") ) {
        // Silently fail on mundane things that prevent item spawn.
        return false;
    }
    if (has_flag_ter(TFLAG_NOFLOOR, p) && p.z > my_ZMIN) {
        return add_item_or_charges(tripoint(p.x, p.y, p.z - 1), new_item, overflow_radius);
    }


    bool tryaddcharges = (new_item.charges  != -1 && new_item.count_by_charges());
    std::vector<tripoint> ps = closest_points_first(overflow_radius, p);
    for( auto p_it = ps.begin(); p_it != ps.end(); p_it++ ) {
        if( !inbounds(*p_it) || new_item.volume() > this->free_volume(*p_it) ||
            has_flag("DESTROY_ITEM", *p_it) || has_flag("NOITEM", *p_it) ) {
            continue;
        }

        if( tryaddcharges ) {
            for( auto &i : i_at_mutable( *p_it ) ) {
                if( i.merge_charges( new_item ) ) {
                    return true;
                }
            }
        }
        if( i_at( *p_it ).size() < MAX_ITEM_IN_SQUARE ) {
            add_item( *p_it, new_item, MAX_ITEM_IN_SQUARE );
            return true;
        }
    }
    return false;
}

// Place an item on the map, despite the parameter name, this is not necessaraly a new item.
// WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
// map::add_item_or_charges
void map::add_item(const int x, const int y, item new_item, const int maxitems)
{
    add_item(tripoint(x, y, 0), new_item, maxitems);
}

void map::add_item(const tripoint &p, item new_item, const int maxitems)
{
    if (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) {
        return;
    }
    if (!inbounds(p)) {
        return;
    }
    if (has_flag("DESTROY_ITEM", p) || ((int)i_at(p).size() >= maxitems)) {
        return;
    }
    if (new_item.has_flag("ACT_IN_FIRE") && get_field( p, fd_fire ) != nullptr ) {
        new_item.active = true;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);
    current_submap->itm[lx][ly].push_back(new_item);
    if( new_item.needs_processing() ) {
        current_submap->active_item_count++;
    }
}

// Check if it's in a fridge and is food, set the fridge
// date to current time, and also check contents.
static void apply_in_fridge(item &it)
{
    if (it.is_food() && it.fridge == 0) {
        it.fridge = (int) calendar::turn;
        // cool down of the HOT flag, is unsigned, don't go below 1
        if ((it.has_flag("HOT")) && (it.item_counter > 10)) {
            it.item_counter -= 10;
        }
        // This sets the COLD flag, and doesn't go above 600
        if ((it.has_flag("EATEN_COLD")) && (!it.has_flag("COLD"))) {
            it.item_tags.insert("COLD");
            it.active = true;
        }
        if ((it.has_flag("COLD")) && (it.item_counter <= 590) && it.fridge > 0) {
            it.item_counter += 10;
        }
    }
    if (it.is_container()) {
        for( auto &elem : it.contents ) {
            apply_in_fridge( elem );
        }
    }
}

static bool process_item( std::vector<item> &items, size_t n, tripoint location, bool activate )
{
    // make a temporary copy, remove the item (in advance)
    // and use that copy to process it
    item temp_item = items[n];
    items.erase( items.begin() + n );
    if( !temp_item.process( nullptr, location, activate ) ) {
        // Not destroyed, must be inserted again, but make sure
        // we don't insert far behind the end of the vector
        n = std::min( items.size(), n );
        items.insert( items.begin() + n, temp_item );
        // Other note: the address of the items vector is
        // not affected by any explosion, but they could reduce
        // the amount of items in it.
        return false;
    }
    return true;
}

void map::process_active_items()
{
    process_items(
        true,
        [] ( std::vector<item> &items, size_t n, tripoint location, vehicle *cur_veh, int part ) {
            if( cur_veh ) {
                const bool fridge_here = cur_veh->fridge_on && cur_veh->part_flag(part, VPFLAG_FRIDGE);
                item &it = items[n];
                if( fridge_here ) {
                    apply_in_fridge(it);
                }
                if( cur_veh->recharger_on && it.has_flag("RECHARGE") &&
                    cur_veh->part_with_feature(part, VPFLAG_RECHARGE) >= 0 ) {
                    int full_charge = dynamic_cast<it_tool*>(it.type)->max_charges;
                    if (it.has_flag("DOUBLE_AMMO")) {
                        full_charge = full_charge * 2;
                    }
                    if (it.is_tool() && full_charge > it.charges ) {
                        if (one_in(10)) {
                            it.charges++;
                        }
                    }
                }
            }
            if( !items[n].needs_processing() ) {
                return false;
            }
            return process_item( items, n, location, false );
        } );
}

template<typename T>
void map::process_items( bool active, T processor )
{
    for( int gx = 0; gx < my_MAPSIZE; gx++ ) {
        for( int gy = 0; gy < my_MAPSIZE; gy++ ) {
            for (int gz = my_ZMIN; gz <= my_ZMAX; gz++) {
            submap *const current_submap = get_submap_at_grid( tripoint( gx, gy, gz ) );
            // Vehicles first in case they get blown up and drop active items on the map.
            if( !current_submap->vehicles.empty() ) {
                process_items_in_vehicles(current_submap, processor);
            }
            if( !active || current_submap->active_item_count > 0) {
                process_items_in_submap(current_submap, gx, gy, gz, processor);
            }
            }
        }
    }
}

template<typename T>
void map::process_items_in_submap( submap *const current_submap, int gridx, int gridy, int gridz, T processor )
{
    for (int i = 0; i < SEEX; i++) {
        for (int j = 0; j < SEEY; j++) {
            tripoint location( gridx * SEEX + i, gridy * SEEY + j, gridz );
            std::vector<item> &items = current_submap->itm[i][j];
            //Do a count-down loop, as some items may be removed
            for (size_t n = 0; n < items.size(); n++) {
                bool previously_active = items[n].active;
                if( processor( items, n, location, nullptr, 0 ) ) {
                    // Item is destroyed, don't reinsert it.
                    // Note: this might lead to items not being processed:
                    // vector: 10 glass items, mininuke, mininuke
                    // the first nuke explodes, destroys some of the glass items
                    // now the index of the second nuke is not 11, but less, but
                    // one can not know which it is now.
                    current_submap->active_item_count -= previously_active;
                    n--;
                } else if( previously_active && !items[n].active ) {
                    current_submap->active_item_count--;
                } else if( !previously_active && items[n].active ) {
                    current_submap->active_item_count++;
                }
            }
        }
    }
}

template<typename T>
void map::process_items_in_vehicles( submap *const current_submap, T processor )
{
    std::vector<vehicle*> &veh_in_nonant = current_submap->vehicles;
    // a copy, important if the vehicle list changes because a
    // vehicle got destroyed by a bomb (an active item!), this list
    // won't change, but veh_in_nonant will change.
    std::vector<vehicle*> vehicles = veh_in_nonant;
    for( auto &vehicles_v : vehicles ) {
        vehicle *cur_veh = vehicles_v;
        if (std::find(veh_in_nonant.begin(), veh_in_nonant.end(), cur_veh) == veh_in_nonant.end()) {
            // vehicle not in the vehicle list of the nonant, has been
            // destroyed (or moved to another nonant?)
            // Can't be sure that it still exists, so skip it
            continue;
        }
        process_items_in_vehicle(cur_veh, current_submap, processor);
    }
}

template<typename T>
void map::process_items_in_vehicle(vehicle *cur_veh, submap *const current_submap, T processor)
{
    item tmp_item;
    std::vector<int> cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, false);
    for(size_t part_index = 0; part_index < cargo_parts.size(); part_index++) {
        const int part = cargo_parts[part_index];
        vehicle_part &vp = cur_veh->parts[part];
        // Cache the mount position and the type, to identify
        // the vehicle part in case cur_veh->parts got changed
        const point mnt(vp.precalc_dx[0], vp.precalc_dy[0]);
        const int vp_type = vp.iid;
        // TODO: Z
        const tripoint item_location( cur_veh->global_x() + vp.precalc_dx[0],
                                   cur_veh->global_y() + vp.precalc_dy[0], 0 );
        std::vector<item> *items_in_part = &vp.items;
        for( size_t n = 0; n < items_in_part->size(); n++ ) {
            if( !processor( *items_in_part, n, item_location, cur_veh, part ) ) {
                // If the item was NOT destroyed, we can skip the remainder,
                // which handles fallout from the vehicle being damaged.
                continue;
            }
            n--; // to process the correct next item.
            // item does not exist anymore, might have been an exploding bomb,
            // check if the vehicle is still valid (does exist)
            std::vector<vehicle*> &veh_in_nonant = current_submap->vehicles;
            if(std::find(veh_in_nonant.begin(), veh_in_nonant.end(), cur_veh) == veh_in_nonant.end()) {
                // Nope, vehicle is not in the vehicle list of the submap,
                // it might have moved to another submap (unlikely)
                // or be destroyed, anywaay it does not need to be processed here
                return;
            }
            // Vehicle still valid, find the current vehicle part again,
            // the list of cargo parts might have changed (image a part with
            // a low index has been removed by an explosion, all the other
            // parts would move up to fill the gap).
            cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, false);
            for(part_index = 0; part_index < cargo_parts.size(); part_index++) {
                vehicle_part &vp = cur_veh->parts[cargo_parts[part_index]];
                if(mnt.x == vp.precalc_dx[0] && mnt.y == vp.precalc_dy[0] && vp_type == vp.iid) {
                    // Found it, this is the vehicle part we are currently iterating overall
                    // update the item vector (if the part index changed,
                    // the address of the item vector changed, too.
                    items_in_part = &vp.items;
                    break;
                }
            }
            if (part_index >= cargo_parts.size()) {
                // went over all parts and did not found the current cargo part
                // it is gone, now we bail out, because we can not know
                // which cargo part to do next, it could be one that has
                // already been handled.
                return;
            }
            // Now we can continue as if nothing happened, the item has
            // already been remove and it is not put back,
        }
    }
}

std::list<item> use_amount_vehicle( std::vector<item> &vec, const itype_id type, int &quantity,
                                    const bool use_container )
{
    std::list<item> ret;
    for (std::vector<item>::iterator a = vec.begin(); a != vec.end() && quantity > 0; ) {
        if (a->use_amount(type, quantity, use_container, ret)) {
            a = vec.erase(a);
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> use_amount_map( map *m, int x, int y, const itype_id type, int &quantity,
                                const bool use_container )
{
    std::list<item> ret;
    for( auto a = m->i_at(x, y).begin(); a != m->i_at(x, y).end() && quantity > 0; ) {
        if( m->get_item(x, y, a)->use_amount(type, quantity, use_container, ret) ) {
            a = m->i_rem( x, y, a );
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> map::use_amount_square(const int x, const int y, const itype_id type,
                                       int &quantity, const bool use_container)
{
  std::list<item> ret;
  int vpart = -1;
  vehicle *veh = veh_at(x,y, vpart);

  if (veh) {
    const int cargo = veh->part_with_feature(vpart, "CARGO");
    if (cargo >= 0) {
      std::list<item> tmp = use_amount_vehicle( veh->parts[cargo].items, type,
                                                quantity, use_container );
      ret.splice(ret.end(), tmp);
    }
  }
  std::list<item> tmp = use_amount_map( this, x, y, type, quantity, use_container);
  ret.splice(ret.end(), tmp);
  return ret;
}

std::list<item> map::use_amount(const point origin, const int range, const itype_id type,
                                const int amount, const bool use_container)
{
  std::list<item> ret;
  int quantity = amount;
  for (int radius = 0; radius <= range && quantity > 0; radius++) {
    for (int x = origin.x - radius; x <= origin.x + radius; x++) {
      for (int y = origin.y - radius; y <= origin.y + radius; y++) {
        if (rl_dist(origin.x, origin.y, x, y) >= radius) {
          std::list<item> tmp;
          tmp = use_amount_square(x, y, type, quantity, use_container);
          ret.splice(ret.end(), tmp);
        }
      }
    }
  }
  return ret;
}

std::list<item> use_charges_from_vehicle(std::vector<item> &vec, const itype_id type, long &quantity)
{
    std::list<item> ret;
    for (std::vector<item>::iterator a = vec.begin(); a != vec.end() && quantity > 0; ) {
        if( a->use_charges(type, quantity, ret)) {
            a = vec.erase(a);
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> use_charges_from_map( map *m, int x, int y, const itype_id type, long &quantity)
{
    std::list<item> ret;
    for( auto a = m->i_at(x, y).begin(); a != m->i_at(x, y).end() && quantity > 0; ) {
        if( m->get_item(x, y, a)->use_charges(type, quantity, ret) ) {
            a = m->i_rem( x, y, a );
        } else {
            ++a;
        }
    }
    return ret;
}

long remove_charges_in_list(const itype *type, map *m, int x, int y, long quantity)
{
    size_t i = 0;
    auto &items = m->i_at(x, y);
    for( ; i < items.size(); i++) {
        if( m->i_at(x, y)[i].type == type) {
            break;
        }
    }

    if( i < items.size() ) {
        item *target = m->get_item( x, y, i );
        if( target->charges > quantity) {
            target->charges -= quantity;
            return quantity;
        } else {
            const long charges = target->charges;
            target->charges = 0;
            if( target->destroyed_at_zero_charges() ) {
                m->i_rem( x, y, i );
            }
            return charges;
        }
    }
    return 0;
}

void use_charges_from_furn( const furn_t &f, const itype_id &type, long &quantity,
                            map *m, int x, int y, std::list<item> &ret )
{
    itype *itt = f.crafting_pseudo_item_type();
    if (itt == NULL || itt->id != type) {
        return;
    }
    const itype *ammo = f.crafting_ammo_item_type();
    if (ammo != NULL) {
        item furn_item(itt->id, 0);
        furn_item.charges = remove_charges_in_list(ammo, m, x, y, quantity);
        if (furn_item.charges > 0) {
            ret.push_back(furn_item);
            quantity -= furn_item.charges;
        }
    }
}

std::list<item> map::use_charges(const point origin, const int range,
                                 const itype_id type, const long amount)
{
    std::list<item> ret;
    long quantity = amount;
    for (int radius = 0; radius <= range && quantity > 0; radius++) {
        for (int x = origin.x - radius; x <= origin.x + radius; x++) {
            for (int y = origin.y - radius; y <= origin.y + radius; y++) {
                if (has_furn(x, y) && accessable_furniture(origin.x, origin.y, x, y, range)) {
                    use_charges_from_furn(furn_at(x, y), type, quantity, this, x, y, ret);
                    if (quantity <= 0) {
                        return ret;
                    }
                }
                if(accessable_items( origin.x, origin.y, x, y, range) ) {
                    continue;
                }
                if (rl_dist(origin.x, origin.y, x, y) >= radius) {
                    int vpart = -1;
                    vehicle *veh = veh_at(x, y, vpart);

                    if (veh) { // check if a vehicle part is present to provide water/power
                        const int kpart = veh->part_with_feature(vpart, "KITCHEN");
                        const int weldpart = veh->part_with_feature(vpart, "WELDRIG");
                        const int craftpart = veh->part_with_feature(vpart, "CRAFTRIG");
                        const int forgepart = veh->part_with_feature(vpart, "FORGE");
                        const int chempart = veh->part_with_feature(vpart, "CHEMLAB");
                        const int cargo = veh->part_with_feature(vpart, "CARGO");

                        if (kpart >= 0) { // we have a kitchen, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "water_clean") {
                                ftype = "water";
                            } else if (type == "hotplate") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (weldpart >= 0) { // we have a weldrig, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "welder") {
                                ftype = "battery";
                            } else if (type == "soldering_iron") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (craftpart >= 0) { // we have a craftrig, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "press") {
                                ftype = "battery";
                            } else if (type == "vac_sealer") {
                                ftype = "battery";
                            } else if (type == "dehydrator") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (forgepart >= 0) { // we have a veh_forge, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "forge") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (chempart >= 0) { // we have a chem_lab, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "chemistry_set") {
                                ftype = "battery";
                            } else if (type == "hotplate") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (cargo >= 0) {
                            std::list<item> tmp =
                                use_charges_from_vehicle(veh->parts[cargo].items, type, quantity);
                            ret.splice(ret.end(), tmp);
                            if (quantity <= 0) {
                                return ret;
                            }
                        }
                    }

                    std::list<item> tmp = use_charges_from_map( this, x, y, type, quantity);
                    ret.splice(ret.end(), tmp);
                    if (quantity <= 0) {
                        return ret;
                    }
                }
            }
        }
    }
    return ret;
}

std::list<std::pair<tripoint, item *> > map::get_rc_items( int x, int y, int z )
{
    std::list<std::pair<tripoint, item *> > rc_pairs;
    tripoint pos;
    for( pos.x = 0; pos.x < SEEX * MAPSIZE; pos.x++ ) {
        if( x != -1 && x != pos.x ) {
            continue;
        }
        for( pos.y = 0; pos.y < SEEY * MAPSIZE; pos.y++ ) {
            if( y != -1 && y != pos.y ) {
                continue;
            }
            for( pos.z = my_ZMIN; pos.z <= my_ZMAX; pos.z++ ) {
            if( z != -1 && z != pos.z ) {
                continue;
            }
            auto &item_stack = i_at_mutable( pos );
            for( auto &elem : item_stack ) {
                if( elem.has_flag( "RADIO_ACTIVATION" ) || elem.has_flag( "RADIO_CONTAINER" ) ) {
                    rc_pairs.push_back( std::make_pair( pos, &( elem ) ) );
                }
            }
            }
        }
    }

    return rc_pairs;
}

static bool trigger_radio_item( std::string signal, std::vector<item> &items, int n, tripoint pos )
{
    bool trigger_item = false;
    if( items[n].has_flag("RADIO_ACTIVATION") && items[n].has_flag(signal) ) {
        g->sound(pos.x, pos.y, 6, "beep.");
        if( items[n].has_flag("BOMB") ) {
            // Set charges to 0 to ensure it detonates.
            items[n].charges = 0;
        }
        trigger_item = true;
    } else if( items[n].has_flag("RADIO_CONTAINER") && !items[n].contents.empty() &&
               items[n].contents[0].has_flag( signal ) ) {
        // A bomb is the only thing meaningfully placed in a container,
        // If that changes, this needs logic to handle the alternative.
        itype_id bomb_type = items[n].contents[0].type->id;

        items[n].make(bomb_type);
        items[n].charges = 0;
        trigger_item = true;
    }
    if( trigger_item ) {
        return process_item( items, n, pos, true );
    }
    return false;
}

void map::trigger_rc_items( std::string signal )
{
    process_items(
        false,
        [&] ( std::vector<item> &items, size_t n, tripoint pos, vehicle *, int ) {
            return trigger_radio_item( signal, items, n, pos );
        } );
}

std::string map::trap_get(const int x, const int y) const {
    return trap_get(tripoint(x, y, 0));
}

std::string map::trap_get(const tripoint &p) const {
    return traplist[ tr_at(p) ]->id;
}

void map::trap_set(const int x, const int y, const std::string & sid) {
    trap_set(tripoint(x, y, 0), sid);
}

void map::trap_set(const tripoint &p, const std::string & sid) {
    if ( trapmap.find(sid) == trapmap.end() ) {
        return;
    }
    add_trap(p, (trap_id)trapmap[ sid ]);
}

void map::trap_set(const int x, const int y, const trap_id id) {
    add_trap(tripoint(x, y, 0), id);
}

void map::trap_set(const tripoint &p, const trap_id id) {
    add_trap(p, id);
}
// todo: to be consistent with ???_at(...) this should return ref to the actual trap object
trap_id map::tr_at(const int x, const int y) const
{
    return tr_at(tripoint(x, y, 0));
}

trap_id map::tr_at(const tripoint &p) const
{
 if (!inbounds(p)) {
  return tr_null;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/

 int lx, ly;
 submap * const current_submap = get_submap_at(p, lx, ly);

 if (terlist[ current_submap->ter[lx][ly] ].trap != tr_null) {
  return terlist[ current_submap->ter[lx][ly] ].trap;
 }

 return current_submap->get_trap(lx, ly);
}

void map::add_trap(const int x, const int y, const trap_id t)
{
    add_trap(tripoint(x, y, 0), t);
}

void map::add_trap(const tripoint &p, const trap_id t)
{
    if (!inbounds(p)) { return; }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    // If there was already a trap here, remove it.
    if (current_submap->get_trap(lx, ly) != tr_null) {
        remove_trap(p);
    }

    current_submap->set_trap(lx, ly, t);
    if (t != tr_null) {
        traplocs[t].insert(point(p.x, p.y));
    }
}

void map::disarm_trap(const int x, const int y)
{
    disarm_trap(tripoint(x, y, 0));
}

void map::disarm_trap(const tripoint &p)
{
    int skillLevel = g->u.skillLevel("traps");

    if (tr_at(p) == tr_null) {
        debugmsg("Tried to disarm a trap where there was none (%d %d %d)", p.x, p.y, p.z);
        return;
    }

    trap* tr = traplist[tr_at(p)];
    const int tSkillLevel = g->u.skillLevel("traps");
    const int diff = tr->get_difficulty();
    int roll = rng(tSkillLevel, 4 * tSkillLevel);

    // Some traps are not actual traps. Skip the rolls, different message and give the option to grab it right away.
    if (tr->get_avoidance() ==  0 && tr->get_difficulty() == 0) {
        add_msg(_("You take down the %s."), tr->name.c_str());
        std::vector<itype_id> comp = tr->components;
        for (auto &i : comp) {
            if (i != "null") {
                spawn_item(p, i, 1, 1);
                remove_trap(p);
            }
        }
        return;
    }

    while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50) {
        roll++;
    }
    if (roll >= diff) {
        add_msg(_("You disarm the trap!"));
        std::vector<itype_id> comp = tr->components;
        for (auto &i : comp) {
            if (i != "null") {
                spawn_item(p, i, 1, 1);
            }
        }
        if (tr_at(p) == tr_engine) {
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i != 0 || j != 0) {
                        remove_trap(tripoint( p.x + i, p.y + j, p.z ));
                    }
                }
            }
        }
        if (tr_at(p) == tr_shotgun_1 || tr_at(p) == tr_shotgun_2) {
            spawn_item(p,"shot_00",1,2);
        }
        remove_trap(p);
        if(diff > 1.25 * skillLevel) { // failure might have set off trap
            g->u.practice( "traps", 1.5*(diff - skillLevel) );
        }
    } else if (roll >= diff * .8) {
        add_msg(_("You fail to disarm the trap."));
        if(diff > 1.25 * skillLevel) {
            g->u.practice( "traps", 1.5*(diff - skillLevel) );
        }
    } else {
        add_msg(m_bad, _("You fail to disarm the trap, and you set it off!"));
        tr->trigger(&g->u, p);
        if(diff - roll <= 6) {
            // Give xp for failing, but not if we failed terribly (in which
            // case the trap may not be disarmable).
            g->u.practice( "traps", 2*diff );
        }
    }
}

void map::remove_trap(const int x, const int y)
{
    remove_trap(tripoint(x, y, 0));
}

void map::remove_trap(const tripoint &p)
{
    if (!inbounds(p)) { return; }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    trap_id t = current_submap->get_trap(lx, ly);
    if (t != tr_null) {
        if (g != NULL && this == &g->m) {
            g->u.add_known_trap(p, "tr_null");
        }
        current_submap->set_trap(lx, ly, tr_null);
        traplocs[t].erase(point(p.x, p.y));
    }
}
/*
 * Get wrapper for all fields at xy
 */
const field &map::field_at( const int x, const int y ) const
{
    return field_at(tripoint(x, y, 0));
}

field &map::get_field( const int x, const int y )
{
    return get_field(tripoint(x, y, 0));
}

field &map::get_field( const tripoint &p )
{
    if( !inbounds( p ) ) {
        nulfield = field();
        return nulfield;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->fld[lx][ly];
}

int map::adjust_field_age(const point p, const field_id t, const int offset) {
    return set_field_age( p, t, offset, true);
}
int map::adjust_field_age(const tripoint &p, const field_id t, const int offset) {
    return set_field_age( p, t, offset, true);
}

int map::adjust_field_strength(const point p, const field_id t, const int offset) {
    return set_field_strength(p, t, offset, true);
}
int map::adjust_field_strength(const tripoint &p, const field_id t, const int offset) {
    return set_field_strength(p, t, offset, true);
}

/*
 * Set age of field type at point, or increment/decrement if offset=true
 * returns resulting age or -1 if not present.
 */
int map::set_field_age(const point p, const field_id t, const int age, bool isoffset) {
    return set_field_age(tripoint(p.x, p.y, 0), t, age, isoffset);
}

int map::set_field_age(const tripoint &p, const field_id t, const int age, bool isoffset) {
    field_entry * field_ptr = get_field( p, t );
    if ( field_ptr != NULL ) {
        int adj = ( isoffset ? field_ptr->getFieldAge() : 0 ) + age;
        field_ptr->setFieldAge( adj );
        return adj;
    }
    return -1;
}

/*
 * set strength of field type at point, creating if not present, removing if strength is 0
 * returns resulting strength, or 0 for not present
 */
int map::set_field_strength(const point p, const field_id t, const int str, bool isoffset) {
    return set_field_strength(tripoint(p.x, p.y, 0), t, str, isoffset);
}

int map::set_field_strength(const tripoint &p, const field_id t, const int str, bool isoffset) {
    field_entry * field_ptr = get_field( p, t );
    if ( field_ptr != NULL ) {
        int adj = ( isoffset ? field_ptr->getFieldDensity() : 0 ) + str;
        if ( adj > 0 ) {
            field_ptr->setFieldDensity( adj );
            return adj;
        } else {
            remove_field( p, t );
            return 0;
        }
    } else if ( 0 + str > 0 ) {
        return ( add_field( p, t, str, 0 ) ? str : 0 );
    }
    return 0;
}

int map::get_field_age( const point p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? -1 : field_ptr->getFieldAge() );
}
int map::get_field_age( const tripoint &p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? -1 : field_ptr->getFieldAge() );
}

int map::get_field_strength( const point p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? 0 : field_ptr->getFieldDensity() );
}
int map::get_field_strength( const tripoint &p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? 0 : field_ptr->getFieldDensity() );
}

field_entry * map::get_field( const point p, const field_id t ) {
    return get_field(tripoint(p.x, p.y, 0), t);
}

field_entry * map::get_field( const tripoint &p, const field_id t ) {
    if (!inbounds(p.x, p.y))
        return NULL;
    return get_field( p ).findField(t);
}

bool map::add_field(const int x, const int y, const field_id t, const int new_density)
{
    return add_field( tripoint( x, y, 0 ), t, new_density, 0 );
}

bool map::add_field(const tripoint &p, const field_id t, const int new_density)
{
    return add_field( p, t, new_density, 0 );
}

bool map::add_field(const point p, const field_id t, int density, const int age)
{
    return add_field(tripoint(p.x, p.y, 0), t, density, age);
}

bool map::add_field(const tripoint &p, const field_id t, int density, const int age)
{
    if (!inbounds(p) || density <= 0) {
        return false;
    }

    if (density > 3) {
        density = 3;
    }
    if (density <= 0) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    if( current_submap->fld[lx][ly].addField( t, density, age ) ) {
        // TODO: Update overall field_count appropriately.
        // This is the spirit of "fd_null" that it used to be.
        current_submap->field_count++; //Only adding it to the count if it doesn't exist.
    }
    if(g != NULL && this == &g->m && p == g->u.pos() ) {
        creature_in_field( g->u ); //Hit the player with the field if it spawned on top of them.
    }
    return true;
}

void map::remove_field(const int x, const int y, const field_id field_to_remove)
{
    remove_field(tripoint(x, y, 0), field_to_remove);
}

void map::remove_field(const tripoint &p, const field_id field_to_remove)
{
 if (!inbounds(p)) {
  return;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(p, lx, ly);

 if (current_submap->fld[lx][ly].findField(field_to_remove)) { //same as checking for fd_null in the old system
  current_submap->field_count--;
 }
 current_submap->fld[lx][ly].removeField(field_to_remove);
}

computer* map::computer_at(const int x, const int y)
{
 if (!inbounds(x, y))
  return NULL;

 // TODO: Z
 submap * const current_submap = get_submap_at(x, y);

 if (current_submap->comp.name == "") {
  return NULL;
 }
 return &(current_submap->comp);
}

bool map::allow_camp(const int x, const int y, const int radius)
{
    return camp_at(x, y, radius) == NULL;
}

// locate the nearest camp in some radius (default CAMPSIZE)
basecamp* map::camp_at(const int x, const int y, const int radius)
{
    if (!inbounds(x, y)) {
        return NULL;
    }

    const int sx = std::max(0, x / SEEX - radius);
    const int sy = std::max(0, y / SEEY - radius);
    const int ex = std::min(MAPSIZE - 1, x / SEEX + radius);
    const int ey = std::min(MAPSIZE - 1, y / SEEY + radius);

    for (int ly = sy; ly < ey; ++ly) {
        for (int lx = sx; lx < ex; ++lx) {
            submap * const current_submap = get_submap_at(x, y);
            if (current_submap->camp.is_valid()) {
                // we only allow on camp per size radius, kinda
                return &(current_submap->camp);
            }
        }
    }

    return NULL;
}

void map::add_camp(const std::string& name, const int x, const int y)
{
    if (!allow_camp(x, y)) {
        dbg(D_ERROR) << "map::add_camp: Attempting to add camp when one in local area.";
        return;
    }

    // TODO: Z
    get_submap_at(x, y)->camp = basecamp(name, x, y);
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(WINDOW* w, const tripoint &center)
{
 // We only need to draw anything if we're not in tiles mode.
 if(is_draw_tiles_mode()) {
     return;
 }

 g->reset_light_level();
 const int g_light_level = (int)g->light_level();
 const int natural_sight_range = g->u.sight_range(1);
 const int light_sight_range = g->u.sight_range(g_light_level);
 const int lowlight_sight_range = std::max(g_light_level / 2, natural_sight_range);
 const int max_sight_range = g->u.unimpaired_range();
 const bool u_is_boomered = g->u.has_effect("boomered");
 const int u_clairvoyance = g->u.clairvoyance();
 const bool u_sight_impaired = g->u.sight_impaired();
 const bool bio_night_active = g->u.has_active_bionic("bio_night");

 tripoint real(0, 0, center.z);
 const tripoint upos = g->u.pos();
 for  (real.x = center.x - getmaxx(w)/2; real.x <= center.x + getmaxx(w)/2; real.x++) {
  for (real.y = center.y - getmaxy(w)/2; real.y <= center.y + getmaxy(w)/2; real.y++) {
   const int dist = rl_dist(upos, real);
   int sight_range = light_sight_range;
   int low_sight_range = lowlight_sight_range;
   // While viewing indoor areas use lightmap model
   if (!is_outside(real)) {
    sight_range = natural_sight_range;
   // Don't display area as shadowy if it's outside and illuminated by natural light
   //and illuminated by source of light
   } else if (light_at(real) > LL_LOW || dist <= light_sight_range) {
    low_sight_range = std::max(g_light_level, natural_sight_range);
   }

   // I've moved this part above loops without even thinking that
   // this must stay here...
   int real_max_sight_range = light_sight_range > max_sight_range ? light_sight_range : max_sight_range;
   int distance_to_look = DAYLIGHT_LEVEL;
 // TODO: Z
   bool can_see = pl_sees(upos, real, distance_to_look);
   lit_level lit = light_at(real);

   // now we're gonna adjust real_max_sight, to cover some nearby "highlights",
   // but at the same time changing light-level depending on distance,
   // to create actual "gradual" stuff
   // Also we'll try to ALWAYS show LL_BRIGHT stuff independent of where it is...
   if (lit != LL_BRIGHT) {
       if (dist > real_max_sight_range) {
           int intLit = (int)lit - (dist - real_max_sight_range)/2;
           if (intLit < 0) intLit = LL_DARK;
           lit = (lit_level)intLit;
       }
   }
   // additional case for real_max_sight_range
   // if both light_sight_range and max_sight_range were small
   // it means we really have limited visibility (e.g. inside a pit)
   // and we shouldn't touch that
   if (lit > LL_DARK && real_max_sight_range > 1) {
       real_max_sight_range = distance_to_look;
   }

   if ((bio_night_active && dist < 15 && dist > natural_sight_range) || // if bio_night active, blackout 15 tile radius around player
       dist > real_max_sight_range ||
       (dist > light_sight_range &&
         (lit == LL_DARK ||
         (u_sight_impaired && lit != LL_BRIGHT) ||
          !can_see))) {
    if (u_is_boomered)
     mvwputch(w, real.y+getmaxy(w)/2 - center.y, real.x+getmaxx(w)/2 - center.x, c_magenta, '#');
    else
         mvwputch(w, real.y+getmaxy(w)/2 - center.y, real.x+getmaxx(w)/2 - center.x, c_dkgray, '#');
   } else if (dist > light_sight_range && u_sight_impaired && lit == LL_BRIGHT) {
    if (u_is_boomered)
     mvwputch(w, real.y+getmaxy(w)/2 - center.y, real.x+getmaxx(w)/2 - center.x, c_pink, '#');
    else
     mvwputch(w, real.y+getmaxy(w)/2 - center.y, real.x+getmaxx(w)/2 - center.x, c_ltgray, '#');
   } else if (dist <= u_clairvoyance || can_see) {
    drawsq(w, g->u, real.x, real.y, false, true, center,
           (dist > low_sight_range && LL_LIT > lit) ||
           (dist > sight_range && LL_LOW == lit),
           LL_BRIGHT == lit);
   } else {
    mvwputch(w, real.y+getmaxy(w)/2 - center.y, real.x+getmaxx(w)/2 - center.x, c_black,' ');
   }
  }
 }
    g->draw_critter( g->u, center );
}

bool map::allows_vertical_view_down( const tripoint &p ) const
{
    return psm_cache[get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) )].see_down[p.x % SEEX][p.y % SEEY];
}

bool map::blocks_vertical_view_down( const tripoint &p ) const
{
    return !psm_cache[get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) )].see_down[p.x % SEEX][p.y % SEEY];
}

bool map::allows_vertical_view_up( const tripoint &p ) const
{
    return psm_cache[get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) )].see_up[p.x % SEEX][p.y % SEEY];
}

bool map::blocks_vertical_view_up( const tripoint &p ) const
{
    return !psm_cache[get_nonant( tripoint( p.x / SEEX, p.y / SEEY, p.z ) )].see_up[p.x % SEEX][p.y % SEEY];
}

bool map::allows_vertical_air_down( const tripoint &p ) const
{
    return !blocks_vertical_air_down( p );
}

bool map::blocks_vertical_air_down(const tripoint &p) const {
    const ter_id tid = ter(p);
    const ter_t &ter = terlist[tid];
    if (!ter.has_flag("PERMEABLE_FLOOR")) { // not permeable at all.
        return true;
    }
    const furn_id fid = furn(p);
    if (fid == f_null) { // OK, terrain is permeable, no blocking furniture
        return false;
    }
    const furn_t &furn = furnlist[fid];
    if (!furn.has_flag("PERMEABLE_FLOOR")) { // not permeable, blocks the terrain
        return true;
    }
    return false;
}

bool map::allows_vertical_air_up(const tripoint &p) const {
    return !blocks_vertical_air_up(p);
}

bool map::blocks_vertical_air_up(const tripoint &p) const {
    return blocks_vertical_air_down(tripoint(p.x, p.y, p.z + 1));
}

int map::z_level_down_xx( const tripoint &p ) const
{
    int z = 0;
    tripoint pp = p;
    while( pp.z > my_ZMIN ) {
        if( blocks_vertical_view_down( pp ) ) {
            break;
        }
        z--;
        pp.z--;
    }
    return z;
}

// Returns the length (z-levels) of a vertical shaft
// starting at p, and going up until an opaque floor stops it
int map::z_level_up_xx( const tripoint &p ) const
{
    int z = 0;
    tripoint pp = p;
    while( pp.z < my_ZMAX ) {
        if( blocks_vertical_view_up( pp ) ) {
            break;
        }
        z++;
        pp.z++;
    }
    return z;
}

void map::drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert_arg,
                 const bool show_items_arg, const tripoint &center,
                 const bool low_light, const bool bright_light)
{
    // We only need to draw anything if we're not in tiles mode.
    if(is_draw_tiles_mode()) {
        return;
    }

    const int view_center_x_arg = center.x;
    const int view_center_y_arg = center.y;
    bool invert = invert_arg;
    bool show_items = show_items_arg;
    int cx = view_center_x_arg;
    int cy = view_center_y_arg;
    if (!inbounds(x, y))
        return; // Out of bounds
    const int k = x + getmaxx(w)/2 - cx;
    const int j = y + getmaxy(w)/2 - cy;
    nc_color tercol;
    // TODO: Z Stupid it is, to make this static!
    static const ter_id t_open_air = terfind("t_open_air");
    int zv = 0;
    if (ter(tripoint(x, y, center.z)) == t_open_air) {
        zv = z_level_down_xx(tripoint(x, y, center.z));
    }
    const tripoint tpx(x, y, center.z + zv);
    const bool from_above = zv != 0;
    if(!pl_sees(u.pos(), tpx, -1)) {
        mvwputch(w, j, k, c_white, ' ');
        return;
    }
    const ter_id curr_ter = ter(tpx);
    const furn_id curr_furn = furn(tpx);
    const trap_id curr_trap = tr_at(tpx);
    const field &curr_field = get_field(tpx);
    const std::vector<item> &curr_items = i_at(tpx);
    long sym;
    bool hi = false;
    bool graf = false;
    bool draw_item_sym = false;


    ter_t &t = terlist[curr_ter];
    sym = from_above ? t.sym_down : t.sym;
    tercol = t.color;
    if (curr_furn != f_null) {
        const furn_t &f = furnlist[curr_furn];
        if (!from_above) {
            sym = f.sym;
            tercol = f.color;
        } else if (f.sym_down != ' ') {
            sym = f.sym_down;
            tercol = f.color;
        }
    }
    if (has_flag(TFLAG_SWIMMABLE, tpx) && has_flag(TFLAG_DEEP_WATER, tpx) && !u.is_underwater()) {
        show_items = false; // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if (curr_trap != tr_null && traplist[curr_trap]->can_see(g->u, tpx)) {
        tercol = traplist[curr_trap]->color;
        if (traplist[curr_trap]->sym == '%') {
            switch(rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else {
            sym = traplist[curr_trap]->sym;
        }
    }
    if (curr_field.fieldCount() > 0) {
        const field_id& fid = curr_field.fieldSymbol();
        const field_entry* fe = curr_field.findField(fid);
        const field_t& f = fieldlist[fid];
        if (f.sym == '&' || fe == NULL) {
            // Do nothing, a '&' indicates invisible fields.
        } else if (f.sym == '*') {
            // A random symbol.
            switch (rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else {
            // A field symbol '%' indicates the field should not hide
            // items/terrain. When the symbol is not '%' it will
            // hide items (the color is still inverted if there are items,
            // but the tile symbol is not changed).
            // draw_item_sym indicates that the item symbol should be used
            // even if sym is not '.'.
            // As we don't know at this stage if there are any items
            // (that are visible to the player!), we always set the symbol.
            // If there are items and the field does not hide them,
            // the code handling items will override it.
            draw_item_sym = (f.sym == '%');
            // If field priority is > 1, and the field is set to hide items,
            //draw the field as it obscures what's under it.
            if( (f.sym != '%' && f.priority > 1) || (f.sym != '%' && sym == '.'))  {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                sym = f.sym;
            }
            tercol = f.color[fe->getFieldDensity() - 1];
        }
    }

    // If there are items here, draw those instead
    if (show_items && sees_some_items(tpx, g->u)) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if (sym != '.' && sym != '%' && !draw_item_sym) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            sym = curr_items[curr_items.size() - 1].symbol();
            if (!draw_item_sym) {
                tercol = curr_items[curr_items.size() - 1].color();
            }
            if (curr_items.size() > 1) {
                invert = !invert;
            }
        }
    }

    int veh_part = 0;
    vehicle *veh = veh_at(tpx, veh_part);
    if (veh) {
        sym = special_symbol (veh->face.dir_symbol(veh->part_sym(veh_part)));
        tercol = veh->part_color(veh_part);
    }
    // If there's graffiti here, change background color
    if( has_graffiti_at( tpx ) ) {
        graf = true;
    }

    //suprise, we're not done, if it's a wall adjacent to an other, put the right glyph
    if(sym == LINE_XOXO || sym == LINE_OXOX) { //vertical or horizontal
        sym = determine_wall_corner(tpx, sym);
    }

    if (u.has_effect("boomered")) {
        tercol = c_magenta;
    } else if ( u.has_nv() ) {
        tercol = (bright_light) ? c_white : c_ltgreen;
    } else if (low_light) {
        tercol = c_dkgray;
    } else if (u.has_effect("darkness")) {
        tercol = c_dkgray;
    }

    if (invert) {
        mvwputch_inv(w, j, k, tercol, sym);
    } else if (hi) {
        mvwputch_hi (w, j, k, tercol, sym);
    } else if (graf) {
        mvwputch    (w, j, k, red_background(tercol), sym);
    } else {
        mvwputch    (w, j, k, tercol, sym);
    }
}

/*
map::sees based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
bool map::sees(const int Fx, const int Fy, const int Tx, const int Ty,
               const int range, int &tc) const
{
 const int dx = Tx - Fx;
 const int dy = Ty - Fy;
 const int ax = abs(dx) << 1;
 const int ay = abs(dy) << 1;
 const int sx = SGN(dx);
 const int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;

 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false; // Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (light_transparency(x, y) != LIGHT_TRANSPARENCY_SOLID && (inbounds(x,y)));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (light_transparency(x, y) != LIGHT_TRANSPARENCY_SOLID && (inbounds(x,y)));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

bool map::sees(const tripoint &F, const tripoint &T,
               const int range, int &tc) const
{
    if (F.z == T.z) {
        // Use the 2-D algorithm, it should be faster and better anyway
        return sees(F.x, F.y, T.x, T.y, range, tc);
    }
    tc = 0;
    const int dx = T.x - F.x;
    const int dy = T.y - F.y;
    const int dz = T.z - F.z;
    const int ax = abs(dx) << 1;
    const int ay = abs(dy) << 1;
    const int az = abs(dz) << 1;
    const int sx = SGN(dx);
    const int sy = SGN(dy);
    const int sz = SGN(dz);
    tripoint pnt;
    int t = 0;
    int tz;
    int st;

    if (range >= 0 && range < rl_dist(F, T) ) {
        return false; // Out of range!
    }
    if (ax > ay) { // Mostly-horizontal line
        st = SGN(ay - (ax >> 1));
        // Doing it "backwards" prioritizes straight lines before diagonal.
        // T.his will help avoid creating a string of zombies behind you and will
        // promote "mobbing" behavior (zombies surround you to beat on you)
        for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            tz = -ay;
            pnt = F;
            do {
                if (t > 0) {
                    pnt.y += sy;
                    t -= ax;
                }
                if (pnt.z != T.z && tz > 0) {
                    bool ok;
                    if (sz < 0) {
                        ok = allows_vertical_view_down(pnt);
                    } else {
                        ok = allows_vertical_view_up(pnt);
                    }
                    if (tz == 1 && !ok) {
                        break;
                    } else if (ok) {
                        pnt.z += sz;
                        tz -= az;
                    }
                }
                pnt.x += sx;
                t += ay;
                tz += ay;
                if (pnt == T) {
                    tc *= st;
                    return true;
                }
            } while ((light_transparency(pnt) != LIGHT_TRANSPARENCY_SOLID) && (inbounds(pnt)));
        }
        return false;
    } else { // Same as above, for mostly-vertical lines
        st = SGN(ax - (ay >> 1));
        for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            tz = -ax;
            pnt = F;
            do {
                if (t > 0) {
                    pnt.x += sx;
                    t -= ay;
                }
                if (pnt.z != T.z && tz > 0) {
                    bool ok;
                    if (sz < 0) {
                        ok = allows_vertical_view_down(pnt);
                    } else {
                        ok = allows_vertical_view_up(pnt);
                    }
                    if (tz == 1 && !ok) {
                        break;
                    } else if (ok) {
                        pnt.z += sz;
                        tz -= az;
                    }
                }
                pnt.y += sy;
                t += ax;
                tz += ax;
                if (pnt == T) {
                    tc *= st;
                    return true;
                }
            } while ((light_transparency(pnt) != LIGHT_TRANSPARENCY_SOLID) && (inbounds(pnt)));
        }
        return false;
    }
    return false; // Shouldn't ever be reached, but there it is.
}

bool map::clear_path(const int Fx, const int Fy, const int Tx, const int Ty,
                     const int range, const int cost_min, const int cost_max, int &tc) const
{
    const int dx = Tx - Fx;
    const int dy = Ty - Fy;
    const int ax = abs(dx) << 1;
    const int ay = abs(dy) << 1;
    const int sx = SGN(dx);
    const int sy = SGN(dy);
    int x = Fx;
    int y = Fy;
    int t = 0;
    int st;

    if (range >= 0 &&  range < rl_dist(Fx, Fy, Tx, Ty) ) {
        return false; // Out of range!
    }
    if (ax > ay) { // Mostly-horizontal line
        st = SGN(ay - (ax >> 1));
        // Doing it "backwards" prioritizes straight lines before diagonal.
        // This will help avoid creating a string of zombies behind you and will
        // promote "mobbing" behavior (zombies surround you to beat on you)
        for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            x = Fx;
            y = Fy;
            do {
                if (t > 0) {
                    y += sy;
                    t -= ax;
                }
                x += sx;
                t += ay;
                if (x == Tx && y == Ty) {
                    tc *= st;
                    return true;
                }
            } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
                     inbounds(x, y));
        }
        return false;
    } else { // Same as above, for mostly-vertical lines
        st = SGN(ax - (ay >> 1));
        for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            x = Fx;
            y = Fy;
            do {
                if (t > 0) {
                    x += sx;
                    t -= ay;
                }
                y += sy;
                t += ax;
                if (x == Tx && y == Ty) {
                    tc *= st;
                    return true;
                }
            } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
                     inbounds(x,y));
        }
        return false;
    }
    return false; // Shouldn't ever be reached, but there it is.
}

bool map::clear_path(const tripoint &F, const tripoint &T,
                     const int range, const int cost_min, const int cost_max, int &tc) const
{
    return clear_path(F.x, F.y, T.x, T.y, range, cost_min, cost_max, tc);
}

bool map::accessable_items(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const
{
    int junk = 0;
    return (has_flag("SEALED", Tx, Ty) && !has_flag("LIQUIDCONT", Tx, Ty)) ||
        ((Fx != Tx || Fy != Ty) &&
         !clear_path( Fx, Fy, Tx, Ty, range, 1, 100, junk ) );
}

bool map::accessable_items(const tripoint &F, const tripoint &T, const int range) const
{
    return accessable_items(F.x, F.y, T.x, T.y, range);
}

bool map::accessable_furniture(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const
{
    int junk = 0;
    return ((Fx == Tx && Fy == Ty) ||
           clear_path( Fx, Fy, Tx, Ty, range, 1, 100, junk ) );
}

bool map::accessable_furniture(const tripoint &F, const tripoint &T, const int range) const
{
    return accessable_furniture(F.x, F.y, T.x, T.y, range);
}

std::vector<point> map::getDirCircle(const int Fx, const int Fy, const int Tx, const int Ty) const
{
    std::vector<point> vCircle;
    vCircle.resize(8);

    const std::vector<point> vLine = line_to(Fx, Fy, Tx, Ty, 0);
    const std::vector<point> vSpiral = closest_points_first(1, Fx, Fy);
    const std::vector<int> vPos {1,2,4,6,8,7,5,3};

    //  All possible constelations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    int iPosOffset = 0;
    for (unsigned int i = 1; i < vSpiral.size(); i++) {
        if (vSpiral[i].x == vLine[0].x && vSpiral[i].y == vLine[0].y) {
            iPosOffset = i-1;
            break;
        }
    }

    for (unsigned int i = 1; i < vSpiral.size(); i++) {
        if (iPosOffset >= (int)vPos.size()) {
            iPosOffset = 0;
        }

        vCircle[vPos[iPosOffset++]-1] = point(vSpiral[i].x, vSpiral[i].y);
    }

    return vCircle;
}

// Bash defaults to true.
std::vector<point> map::route(const int Fx, const int Fy, const int Tx, const int Ty, const bool can_bash) const
{
    /* TODO: If the origin or destination is out of bound, figure out the closest
     * in-bounds point and go to that, then to the real origin/destination.
     */

 if (!inbounds(Fx, Fy) || !inbounds(Tx, Ty)) {
        int linet;
        if (sees(Fx, Fy, Tx, Ty, -1, linet)) {
            return line_to(Fx, Fy, Tx, Ty, linet);
        } else {
            std::vector<point> empty;
            return empty;
        }
    }
    // First, check for a simple straight line on flat ground
    int linet = 0;
    if (clear_path(Fx, Fy, Tx, Ty, -1, 2, 2, linet)) {
        return line_to(Fx, Fy, Tx, Ty, linet);
    }
    /*
    if (move_cost(Tx, Ty) == 0) {
        debugmsg("%d:%d wanted to move to %d:%d, a %s!", Fx, Fy, Tx, Ty,
                    tername(Tx, Ty).c_str());
    }
    if (move_cost(Fx, Fy) == 0) {
        debugmsg("%d:%d, a %s, wanted to move to %d:%d!", Fx, Fy,
                    tername(Fx, Fy).c_str(), Tx, Ty);
    }
    */
    std::vector<point> open;
    astar_list list[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int score[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int gscore[SEEX * MAPSIZE][SEEY * MAPSIZE];
    point parent[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
    if (Tx < Fx) {
        startx = Tx - 4;
        endx = Fx + 4;
    }
    if (Ty < Fy) {
        starty = Ty - 4;
        endy = Fy + 4;
    }
    if (startx < 0) {
        startx = 0;
    }
    if (starty < 0) {
        starty = 0;
    }
    if (endx > SEEX * my_MAPSIZE - 1) {
        endx = SEEX * my_MAPSIZE - 1;
    }
    if (endy > SEEY * my_MAPSIZE - 1) {
        endy = SEEY * my_MAPSIZE - 1;
    }

    for (int x = startx; x <= endx; x++) {
        for (int y = starty; y <= endy; y++) {
            list  [x][y] = ASL_NONE; // Init to not being on any list
            score [x][y] = 0;        // No score!
            gscore[x][y] = 0;        // No score!
            parent[x][y] = point(-1, -1);
        }
    }
    list[Fx][Fy] = ASL_OPEN;
    open.push_back(point(Fx, Fy));

    bool done = false;

    do {
        //debugmsg("Open.size() = %d", open.size());
        int best = 9999;
        int index = -1;
        for (size_t i = 0; i < open.size(); i++) {
            if (i == 0 || score[open[i].x][open[i].y] < best) {
                best = score[open[i].x][open[i].y];
                index = i;
            }
        }

        std::vector<point> vDirCircle = getDirCircle(open[index].x, open[index].y, Tx, Ty);

        for( auto &elem : vDirCircle ) {
            const int x = elem.x;
            const int y = elem.y;

            if (x == Tx && y == Ty) {
                done = true;
                parent[x][y] = open[index];
            } else if (x >= startx && x <= endx && y >= starty && y <= endy &&
                        (move_cost(x, y) > 0 || (can_bash && is_bashable(x, y)))) {
                if (list[x][y] == ASL_NONE) { // Not listed, so make it open
                    list[x][y] = ASL_OPEN;
                    open.push_back(point(x, y));
                    parent[x][y] = open[index];
                    gscore[x][y] = gscore[open[index].x][open[index].y] + move_cost(x, y) + ((open[index].x - x != 0 && open[index].y - y != 0) ? 1 : 0);
                    if (ter(x, y) == t_door_c) {
                        gscore[x][y] += 4; // A turn to open it and a turn to move there
                    } else if (move_cost(x, y) == 0 && (can_bash && is_bashable(x, y))) {
                        gscore[x][y] += 18; // Worst case scenario with damage penalty
                    }
                    score[x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
                } else if (list[x][y] == ASL_OPEN) { // It's open, but make it our child
                    int newg = gscore[open[index].x][open[index].y] + move_cost(x, y) + ((open[index].x - x != 0 && open[index].y - y != 0) ? 1 : 0);
                    if (ter(x, y) == t_door_c) {
                        newg += 4; // A turn to open it and a turn to move there
                    } else if (move_cost(x, y) == 0 && (can_bash && is_bashable(x, y))) {
                        newg += 18; // Worst case scenario with damage penalty
                    }
                    if (newg < gscore[x][y]) {
                        gscore[x][y] = newg;
                        parent[x][y] = open[index];
                        score [x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
                    }
                }
            }
        }
        list[open[index].x][open[index].y] = ASL_CLOSED;
        open.erase(open.begin() + index);
    } while (!done && !open.empty());

    std::vector<point> tmp;
    std::vector<point> ret;
    if (done) {
        point cur(Tx, Ty);
        while (cur.x != Fx || cur.y != Fy) {
            //debugmsg("Retracing... (%d:%d) => [%d:%d] => (%d:%d)", Tx, Ty, cur.x, cur.y, Fx, Fy);
            tmp.push_back(cur);
            if (rl_dist(cur.x, cur.y, parent[cur.x][cur.y].x, parent[cur.x][cur.y].y)>1){
                debugmsg("Jump in our route! %d:%d->%d:%d", cur.x, cur.y,
                            parent[cur.x][cur.y].x, parent[cur.x][cur.y].y);
                return ret;
            }
            cur = parent[cur.x][cur.y];
        }
        for (int i = tmp.size() - 1; i >= 0; i--) {
            ret.push_back(tmp[i]);
        }
    }
    return ret;
}

int map::coord_to_angle ( const tripoint &a, const tripoint &p )
{
    return coord_to_angle( a.x, a.y, p.x, p.y );
}

int map::coord_to_angle ( const int x, const int y, const int tgtx, const int tgty ) {
  const double DBLRAD2DEG = 57.2957795130823f;
  //const double PI = 3.14159265358979f;
  const double DBLPI = 6.28318530717958f;
  double rad = atan2 ( static_cast<double>(tgty - y), static_cast<double>(tgtx - x) );
  if ( rad < 0 ) rad = DBLPI - (0 - rad);
  return int( rad * DBLRAD2DEG );
}

void map::save()
{
    tripoint gp;
    for( gp.x = 0; gp.x < my_MAPSIZE; gp.x++ ) {
        for( gp.y = 0; gp.y < my_MAPSIZE; gp.y++ ) {
            for( gp.z = my_ZMIN; gp.z <= my_ZMAX; gp.z++ ) {
                saven( gp );
            }
        }
    }
}

void map::load_abs(const int wx, const int wy, const int wz, const bool update_vehicle)
{
    traplocs.clear();
    set_abs_sub( wx, wy, wz );
    tripoint gp;
    for( gp.x = 0; gp.x < my_MAPSIZE; gp.x++ ) {
        for( gp.y = 0; gp.y < my_MAPSIZE; gp.y++ ) {
            for( gp.z = my_ZMIN; gp.z <= my_ZMAX; gp.z++ ) {
                loadn( gp, update_vehicle );
            }
        }
    }
}

void map::load(const int wx, const int wy, const int wz, const bool update_vehicle, overmap *om)
{
    const int awx = om->pos().x * OMAPX * 2 + wx;
    const int awy = om->pos().y * OMAPY * 2 + wy;
    load_abs(awx, awy, wz, update_vehicle);
}

void map::update_traps( const tripoint &gp )
{
    const auto to = get_submap_at_grid( gp );
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEX; y++ ) {
            auto t = to->get_trap( x, y );
            if( t != tr_null ) {
                traplocs[t].insert( point( gp.x * SEEX + x, gp.y * SEEY + y ) );
            }
        }
    }
}

void map::shift( const tripoint &s )
{
    const int sx = s.x;
    const int sy = s.y;
    const int sz = s.z;
// Special case of 0-shift; refresh the map
    if (sx == 0 && sy == 0 && sz == 0) {
        return; // Skip this?
    }
    const int wx = get_abs_sub().x;
    const int wy = get_abs_sub().y;
    const int wz = get_abs_sub().z;
    set_abs_sub( wx + sx, wy + sy, wz + sz );

// if player is in vehicle, (s)he must be shifted with vehicle too
    if( g->u.in_vehicle ) {
        g->u.posx -= sx * SEEX;
        g->u.posy -= sy * SEEY;
        // player is _always_ on z-level 0, so no need to shift it.
    }

    for( vehicle *veh : vehicle_list ) {
        veh->smx += sx;
        veh->smy += sy;
//        veh->smz += sz; TODO
    }

// Clear vehicle list and rebuild after shift
    clear_vehicle_cache();
    vehicle_list.clear();
    // Clear all the traps because they all will be changed and because this is a map, all entries
    // must be removed and reinserted anyway, so remove them all at once to have a clean state.
    traplocs.clear();

    // wx and wy are our position in the world, for saving/loading purposes, they are already shifted!
    // Move grid to oldgrid an reinitialize grid to all NULL
    std::vector<submap*> oldgrid( grid.size(), NULL );
    oldgrid.swap( grid );
    t_per_submap_cache_vector oldpsm_cache( psm_cache.size() );
    oldpsm_cache.swap(psm_cache);
    // Step through each grid and get its grid point before the shift.
    // If that point is valid, we can simply move the submap pointer,
    // else we have to load a new submap.
    tripoint gp;
    for( gp.x = 0; gp.x < my_MAPSIZE; gp.x++ ) {
        for( gp.y = 0; gp.y < my_MAPSIZE; gp.y++ ) {
            for( gp.z = my_ZMIN; gp.z <= my_ZMAX; gp.z++ ) {
                const tripoint oldgp( gp.x + sx, gp.y + sy, gp.z + sz );
                if( valid_gp( oldgp ) ) {
                    // If we can reuse an already loaded submap, we must update some stuff,
                    // that had been cleared, otherwise this would be done directly via loadn.
                    grid[get_nonant( gp )] = oldgrid[get_nonant( oldgp )];
                    update_vehicle_list( gp );
                    update_traps( gp );
                    psm_cache[get_nonant( gp )] = oldpsm_cache[get_nonant( oldgp )];
                } else {
                    loadn( gp, true );
                }
            }
        }
    }
    reset_vehicle_cache();
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
void map::saven( const tripoint gp )
{
    dbg( D_INFO ) << "map::saven(abs_sub[" << abs_sub << "], " << "gp[" << gp << "])";
    submap *submap_to_save = get_submap_at_grid( gp ) ;
    if( submap_to_save == NULL || submap_to_save->ter[0][0] == t_null ) {
        dbg( D_ERROR ) << "map::saven grid NULL!";
        return;
    }
    const int abs_x = abs_sub.x + gp.x;
    const int abs_y = abs_sub.y + gp.y;
    const int abs_z = abs_sub.z + gp.z;
    dbg( D_INFO ) << "map::saven abs_x: " << abs_x << "  abs_y: " << abs_y << "  abs_z: " << abs_z;
    submap_to_save->turn_last_touched = int(calendar::turn);
    MAPBUFFER.add_submap( abs_x, abs_y, abs_z, submap_to_save );
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
void map::loadn( const tripoint gp, const bool update_vehicles ) {

 dbg(D_INFO) << "map::loadn(game[" << g << "], abs_sub["<<abs_sub<<"], gp["<<gp<<"])";

    const tripoint abs( abs_sub.x + gp.x, abs_sub.y + gp.y, abs_sub.z + gp.z );
    const int gridn = get_nonant( gp );
    dbg(D_INFO) << "map::loadn abs: " << abs << "  gridn: " << gridn;

    submap *tmpsub = MAPBUFFER.lookup_submap(abs.x, abs.y, abs.z);
    if( tmpsub == nullptr ) {
        // It doesn't exist; we must generate it!
        dbg( D_INFO | D_WARNING ) << "map::loadn: Missing mapbuffer data. Regenerating.";
        tinymap tmp_map;
        // Each overmap square is two nonants; to prevent overlap, generate only at
        //  squares divisible by 2.
        const int newmapx = abs.x - ( abs( abs.x ) % 2 );
        const int newmapy = abs.y - ( abs( abs.y ) % 2 );
        tmp_map.generate( newmapx, newmapy, abs.z, calendar::turn );
        // This is the same call to MAPBUFFER as above!
        tmpsub = MAPBUFFER.lookup_submap( abs.x, abs.y, abs.z );
        if( tmpsub == nullptr ) {
            dbg( D_ERROR ) << "failed to generate a submap at " << abs;
            debugmsg( "failed to generate a submap at %d,%d,%d", abs.x, abs.y, abs.z );
            return;
        }
    }

    // New submap changes the content of the map and all caches must be recalculated
    set_transparency_cache_dirty();
    set_outside_cache_dirty();
    setsubmap( gridn, tmpsub );
    psm_cache[gridn].vflags = 0;

  // Update vehicle data
  for( std::vector<vehicle*>::iterator it = tmpsub->vehicles.begin(),
        end = tmpsub->vehicles.end(); update_vehicles && it != end; ++it ) {

   // Only add if not tracking already.
   if( vehicle_list.find( *it ) == vehicle_list.end() ) {
    // gridx/y not correct. TODO: Fix
    (*it)->smx = gp.x;
    (*it)->smy = gp.y;
    (*it)->smz = gp.z;
    vehicle_list.insert(*it);
    update_vehicle_cache(*it);
   }
  }

    actualize( gp );
}

bool map::has_rotten_away( item &itm, const tripoint &pnt ) const
{
    if( itm.is_corpse() ) {
        itm.calc_rot( pnt );
        return itm.get_rot() > DAYS( 10 ) && !itm.can_revive();
    } else if( itm.goes_bad() ) {
        itm.calc_rot( pnt );
        return itm.has_rotten_away();
    } else if( itm.type->container && itm.type->container->preserves ) {
        // Containers like tin cans preserves all items inside, they do not rot at all.
        return false;
    } else if( itm.type->container && itm.type->container->seals ) {
        // Items inside rot but do not vanish as the container seals them in.
        for( auto &c : itm.contents ) {
            c.calc_rot( pnt );
        }
        return false;
    } else {
        // Check and remove rotten contents, but always keep the container.
        remove_rotten_items( itm.contents, pnt );
        return false;
    }
}

void map::remove_rotten_items( std::vector<item> &items, const tripoint &pnt ) const
{
    for( auto it = items.begin(); it != items.end(); ) {
        if( has_rotten_away( *it, pnt ) ) {
            it = items.erase( it );
        } else {
            ++it;
        }
    }
}

void map::fill_funnels( const tripoint pnt )
{
    const auto t = tr_at( pnt.x, pnt.y );
    if( t == tr_null || traplist[t]->funnel_radius_mm <= 0 ) {
        // not a funnel at all
        return;
    }
    // Note: the inside/outside cache might not be correct at this time
    if( has_flag_ter_or_furn( TFLAG_INDOORS, pnt.x, pnt.y ) ) {
        return;
    }
    auto &items = i_at( pnt.x, pnt.y );
    size_t biggest_container_index = items.size();
    int maxvolume = 0;
    for( size_t i = 0; i < items.size(); ++i ) {
        if( items[i].is_funnel_container( maxvolume ) ) {
            biggest_container_index = i;
        }
    }
    if( biggest_container_index != items.size() ) {
        retroactively_fill_from_funnel( get_item( pnt.x, pnt.y, biggest_container_index),
                                        t, calendar::turn, pnt );
    }
}

void map::grow_plant( const point pnt )
{
    const auto &furn = furn_at( pnt.x, pnt.y );
    if( !furn.has_flag( "PLANT" ) ) {
        return;
    }
    auto &items = i_at( pnt.x, pnt.y );
    if( items.empty() ) {
        // No seed there anymore, we don't know what kind of plant it was.
        dbg( D_ERROR ) << "a seed item has vanished at " << pnt.x << "," << pnt.y;
        furn_set( pnt.x, pnt.y, f_null );
        return;
    }
    // plantEpoch is half a season; 3 epochs pass from plant to harvest
    const int plantEpoch = DAYS( calendar::season_length() ) / 2;
    // Erase fertilizer tokens, but keep the seed item
    i_rem( pnt.x, pnt.y, 1 );
    auto *seed = get_item( pnt.x, pnt.y, 0 );
    // TODO: the comparisons to the loadid is very fragile. Replace with something more explicit.
    while( calendar::turn > seed->bday + plantEpoch && furn.loadid < f_plant_harvest ) {
        seed->bday += plantEpoch;
        furn_set( pnt.x, pnt.y, static_cast<furn_id>( furn.loadid + 1 ) );
    }
}

void map::restock_fruits( const point pnt, int time_since_last_actualize )
{
    const auto &ter = ter_at( pnt.x, pnt.y );
    //if the fruit-bearing season of the already harvested terrain has passed, make it harvestable again
    if( !ter.has_flag( TFLAG_HARVESTED ) ) {
        return;
    }
    if( ter.harvest_season != calendar::turn.get_season() ||
        time_since_last_actualize >= DAYS( calendar::season_length() ) ) {
        ter_set( pnt.x, pnt.y, ter.transforms_into );
    }
}

void map::actualize( const tripoint &gp )
{
    submap *const tmpsub = get_submap_at_grid( gp );
    const auto time_since_last_actualize = calendar::turn - tmpsub->turn_last_touched;
    const bool do_funnels = ( abs_sub.z >= 0 );

    // check spoiled stuff, and fill up funnels while we're at it
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const tripoint pnt( gp.x * SEEX + x, gp.y * SEEY + y, gp.z + my_ZMIN );

            remove_rotten_items( tmpsub->itm[x][y], pnt );

            const auto trap_here = tmpsub->get_trap( x, y );
            if( trap_here != tr_null ) {
                traplocs[trap_here].insert( point( pnt.x, pnt.y ) );
            }

            if( do_funnels ) {
                fill_funnels( pnt );
            }

            grow_plant( point( pnt.x, pnt.y ) );

            restock_fruits( point( pnt.x, pnt.y ), time_since_last_actualize );
        }
    }

    //Merchants will restock their inventories every three days
    const int merchantRestock = 14400 * 3; //14400 is the length of one day
    //Check for Merchants to restock
    for( auto & i : g->active_npc ) {
        if( i->restock != -1 && calendar::turn > ( i->restock + merchantRestock ) ) {
            i->shop_restock();
            i->restock = int( calendar::turn );
        }
    }

    // the last time we touched the submap, is right now.
    tmpsub->turn_last_touched = calendar::turn;
}

void map::spawn_monsters( const tripoint &gp, mongroup &group, bool ignore_sight )
{
    const int s_range = std::min(SEEX * (MAPSIZE / 2), g->u.sight_range( g->light_level() ) );
    int pop = group.population;
    std::vector<tripoint> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        // When the map shifts because of the player moving (called from game::plmove),
        // the player has still their *old* (not shifted) coordinates.
        // That makes the submaps that have come into view visible (if the sight range
        // is big enough).
        if( gp.x == 0 || gp.y == 0 || gp.x + 1 == MAPSIZE || gp.y + 1 == MAPSIZE || gp.z == my_ZMIN || gp.z == my_ZMAX ) {
            ignore_sight = true;
        }
    }
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            tripoint f( y + SEEX * gp.x, y + SEEY * gp.y, gp.z );
            if( g->critter_at( f ) != nullptr ) {
                continue; // there is already some creature
            }
            if( move_cost( f ) == 0 ) {
                continue; // solid area, impassable
            }
            int t;
            if( !ignore_sight && sees( g->u.pos(), f, s_range, t ) ) {
                continue; // monster must spawn outside the viewing range of the player
            }
            if( has_flag_ter_or_furn( TFLAG_INDOORS, f ) ) {
                continue; // monster must spawn outside.
            }
            locations.push_back( f );
        }
    }
    if( locations.empty() ) {
        // TODO: what now? there is now possible place to spawn monsters, most
        // likely because the player can see all the places.
        dbg( D_ERROR ) << "Empty locations for group " << group.type << " at " << gp.x << "," << gp.y << "," << gp.z;
        return;
    }
    for( int m = 0; m < pop; m++ ) {
        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group.type, &pop );
        if( spawn_details.name == "mon_null" ) {
            continue;
        }
        monster tmp( GetMType( spawn_details.name ) );
        for( int i = 0; i < spawn_details.pack_size; i++) {
            for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
                const size_t index = rng( 0, locations.size() - 1 );
                const tripoint p = locations[index];
                if( !tmp.can_move_to( p ) ) {
                    continue; // target can not contain the monster
                }
                tmp.spawn( p );
                g->add_zombie( tmp );
                locations.erase( locations.begin() + index );
                break;
            }
        }
    }
    // indicates the group is empty, and can be removed later
    group.population = 0;
}

void map::spawn_monsters(bool ignore_sight)
{
    tripoint gp;
    for( gp.x = 0; gp.x < my_MAPSIZE; gp.x++ ) {
        for( gp.y = 0; gp.y < my_MAPSIZE; gp.y++ ) {
        for( gp.z = my_ZMIN; gp.z <= my_ZMAX; gp.z++ ) {
            auto groups = overmap_buffer.groups_at( abs_sub.x + gp.x, abs_sub.y + gp.y, abs_sub.z + gp.z);
            for( auto &mgp : groups ) {
                spawn_monsters( gp, *mgp, ignore_sight );
            }

            submap * const current_submap = get_submap_at_grid( gp );
            for (auto &i : current_submap->spawns) {
                for (int j = 0; j < i.count; j++) {
                    int tries = 0;
                    int mx = i.posx, my = i.posy;
                    monster tmp(GetMType(i.type));
                    tmp.faction_id = i.faction_id;
                    tmp.mission_id = i.mission_id;
                    if (i.name != "NONE") {
                        tmp.unique_name = i.name;
                    }
                    if (i.friendly) {
                        tmp.friendly = -1;
                    }
                    tripoint f(mx + gp.x * SEEX, my + gp.y * SEEY, gp.z);

                    while ((!g->is_empty(f) || !tmp.can_move_to(f)) && tries < 10) {
                        mx = (i.posx + rng(-3, 3)) % SEEX;
                        my = (i.posy + rng(-3, 3)) % SEEY;
                        if (mx < 0) {
                            mx += SEEX;
                        }
                        if (my < 0) {
                            my += SEEY;
                        }
                        f.x = mx + gp.x * SEEX;
                        f.y = my + gp.y * SEEY;
                        tries++;
                    }
                    if (tries != 10) {
                        tmp.spawn( f );
                        g->add_zombie(tmp);
                        add_msg( m_debug, "Spawning %s at %d,%d,%d", tmp.name().c_str(), f.x, f.y, f.z );
                    } else {
                        add_msg( m_debug, "Could not spawn %s near %d,%d,%d", tmp.name().c_str(), f.x, f.y, f.z );
                    }
                }
            }
            current_submap->spawns.clear();
            overmap_buffer.spawn_monster( abs_sub.x + gp.x, abs_sub.y + gp.y, abs_sub.z + gp.z );
        }
        }
    }
}

void map::clear_spawns()
{
    for( auto & smap : grid ) {
        smap->spawns.clear();
    }
}

void map::clear_traps()
{
    for( auto & smap : grid ) {
        for (int x = 0; x < SEEX; x++) {
            for (int y = 0; y < SEEY; y++) {
                smap->set_trap(x, y, tr_null);
            }
        }
    }

    // Forget about all trap locations.
    std::map<trap_id, std::set<point> >::iterator i;
    for(i = traplocs.begin(); i != traplocs.end(); ++i) {
        i->second.clear();
    }
}

const std::set<point> &map::trap_locations(trap_id t) const
{
    const auto it = traplocs.find(t);
    if(it != traplocs.end()) {
        return it->second;
    }
    static std::set<point> empty_set;
    return empty_set;
}

bool map::inbounds(const int x, const int y) const
{
 return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}

bool map::inbounds(const tripoint &p) const
{
    return inbounds( p.x, p.y ) && p.z >= my_ZMIN && p.z <= my_ZMAX;
}

void map::set_graffiti( int x, int y, const std::string &contents )
{
    set_graffiti(tripoint(x, y, 0), contents);
}

void map::set_graffiti( const tripoint &p, const std::string &contents )
{
    if( !inbounds( p ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    current_submap->set_graffiti( lx, ly, contents );
}

void map::delete_graffiti( int x, int y )
{
    delete_graffiti(tripoint(x, y, 0));
}

void map::delete_graffiti( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    current_submap->delete_graffiti( lx, ly );
}

const std::string &map::graffiti_at( int x, int y ) const
{
    return graffiti_at(tripoint(x, y, 0));
}

const std::string &map::graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        static const std::string empty_string;
        return empty_string;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    return current_submap->get_graffiti( lx, ly );
}

bool map::has_graffiti_at( int x, int y ) const
{
    return has_graffiti_at(tripoint(x, y, 0));
}

bool map::has_graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    return current_submap->has_graffiti( lx, ly );
}

long map::determine_wall_corner(const tripoint &p, const long orig_sym)
{
    long sym = orig_sym;
    //LINE_NESW
    const long above = ter_at(tripoint(p.x, p.y-1, p.z)).sym;
    const long below = ter_at(tripoint(p.x, p.y+1, p.z)).sym;
    const long left  = ter_at(tripoint(p.x-1, p.y, p.z)).sym;
    const long right = ter_at(tripoint(p.x+1, p.y, p.z)).sym;

    const bool above_connects = above == sym || (above == '"' || above == '+' || above == '\'');
    const bool below_connects = below == sym || (below == '"' || below == '+' || below == '\'');
    const bool left_connects  = left  == sym || (left  == '"' || left  == '+' || left  == '\'');
    const bool right_connects = right == sym || (right == '"' || right == '+' || right == '\'');

    // -
    // |      this = - and above = | or a connectable
    if(sym == LINE_OXOX &&  (above == LINE_XOXO || above_connects))
    {
        //connects to upper
        if(left_connects)
            sym = LINE_XOOX; // ┘ left coming wall
        else if(right_connects)
            sym = LINE_XXOO;//└   right coming wall
        if(left_connects && right_connects)
            sym = LINE_XXOX; // ┴ passing by
    }

    // |
    // -      this = - and below = | or a connectable
    else if(sym == LINE_OXOX && (below == LINE_XOXO || below_connects))
    {
        //connects to lower
        if(left_connects)
            sym = LINE_OOXX; // ┐ left coming wall
        else if(right_connects)
            sym = LINE_OXXO;//┌   right coming wall
        if(left_connects && right_connects)
            sym = LINE_OXXX; // ┬ passing by
    }

    // -|       this = | and left = - or a connectable
    else if(sym == LINE_XOXO && (left == LINE_OXOX || left_connects))
    {
        //connexts to left
        if(above_connects)
            sym = LINE_XOOX; // ┘ north coming wall
        else if(below_connects )
            sym = LINE_OOXX;//┐   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XOXX; // ┤ passing by
    }

    // |-       this = | and right = - or a connectable
    else if(sym == LINE_XOXO && (right == LINE_OXOX || right_connects))
    {
        //connects to right
        if(above_connects)
            sym = LINE_XXOO; // └ north coming wall
        else if(below_connects)
            sym = LINE_OXXO;// ┌   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XXXO; // ├ passing by
    }

    if(above == LINE_XOXO && left == LINE_OXOX && above == below && left == right)
        sym = LINE_XXXX; // ┼ crossway

    return sym;
}

void map::build_outside_cache()
{
    if (!outside_cache_dirty) {
        return;
    }

    if (g->levz < 0)
    {
        memset(outside_cache, false, sizeof(outside_cache));
        return;
    }
    memset(outside_cache, true, sizeof(outside_cache));

    for(int x = 0; x < SEEX * my_MAPSIZE; x++)
    {
        for(int y = 0; y < SEEY * my_MAPSIZE; y++)
        {
            if( has_flag_ter_or_furn(TFLAG_INDOORS, x, y))
            {
                for( int dx = -1; dx <= 1; dx++ )
                {
                    for( int dy = -1; dy <= 1; dy++ )
                    {
                        if(inbounds(x + dx, y + dy))
                        {
                            outside_cache[x + dx][y + dy] = false;
                        }
                    }
                }
            }
        }
    }

    outside_cache_dirty = false;
}

// TODO: Z Just for now, make something better later
#define transparency_cache(x, y, z) \
    (psm_cache[get_nonant(tripoint((x) / SEEX, (y) / SEEY, (z)))].transparency[(x) % SEEX][(y) % SEEY])

float map::light_transparency(const int x, const int y) const
{
    return light_transparency(tripoint(x, y, 0));
}

float map::light_transparency(const tripoint &p) const
{
    if (!inbounds(p)) {
        return LIGHT_TRANSPARENCY_SOLID;
    }
    return transparency_cache(p.x, p.y, p.z);
 }

// TODO Consider making this just clear the cache and dynamically fill it in as trans() is called
void map::build_transparency_cache()
{
    if( !transparency_cache_dirty ) {
        return;
    }
    for( int x = 0; x < my_MAPSIZE * SEEX; x++ ) {
        for( int y = 0; y < my_MAPSIZE * SEEY; y++ ) {
            for( int z = my_ZMIN; z <= my_ZMAX; z++ ) {
                // TODO: Z use cache validity flag
                const tripoint p(x, y, z);
                float &transp = transparency_cache(x, y, z);
            // Default to fully transparent.
            transp = LIGHT_TRANSPARENCY_CLEAR;

            if( !ter_at(p).transparent || !furn_at(p).transparent ) {
                transp = LIGHT_TRANSPARENCY_SOLID;
                continue;
            }

            const field &curfield = field_at(p);
            for( auto &fld : curfield ) {
                const field_entry * cur = &fld.second;
                    if( !fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1] ) {
                        // Fields are either transparent or not, however we want some to be translucent
                        switch(cur->getFieldType()) {
                        case fd_cigsmoke:
                        case fd_weedsmoke:
                        case fd_cracksmoke:
                        case fd_methsmoke:
                        case fd_relax_gas:
                            transp *= 0.7;
                            break;
                        case fd_smoke:
                        case fd_incendiary:
                        case fd_toxic_gas:
                        case fd_tear_gas:
                            if(cur->getFieldDensity() == 3) {
                                transp = LIGHT_TRANSPARENCY_SOLID;
                            }
                            if(cur->getFieldDensity() == 2) {
                                transp *= 0.5;
                            }
                            break;
                        case fd_nuke_gas:
                            transp *= 0.5;
                            break;
                        default:
                            transp = LIGHT_TRANSPARENCY_SOLID;
                            break;
                        }
                    }
                    // TODO: [lightmap] Have glass reduce light as well
            }
            }
        }
    }
    transparency_cache_dirty = false;
}

void map::build_map_cache()
{
    build_outside_cache();

    build_transparency_cache();

    // Cache all the vehicle stuff in one loop
    VehicleList vehs = get_vehicles();
    for(auto &v : vehs) {
        for (size_t part = 0; part < v.v->parts.size(); part++) {
            int px = v.x + v.v->parts[part].precalc_dx[0];
            int py = v.y + v.v->parts[part].precalc_dy[0];
            const int z = v.z;
            if(inbounds(tripoint(px, py, z))) {
                per_submap_cache &psm = psm_cache[get_nonant( tripoint( px / SEEX, py / SEEY, z) )];
                per_submap_cache *psma = (z == my_ZMAX) ? NULL : &(psm_cache[get_nonant( tripoint( px / SEEX, py / SEEY, z + 1 ) )]);
                const int lx = px % SEEX;
                const int ly = py % SEEY;
                if (v.v->is_inside(part)) {
                    outside_cache[px][py] = false;
                    psm.see_up[lx][ly] = false;
                    if (psma != NULL) {
                        psma->see_down[lx][ly] = false;
                    }
                }
                if (v.v->part_flag(part, "ROOF")) {
                    psm.see_up[lx][ly] = false;
                    if (psma != NULL) {
                        psma->see_down[lx][ly] = false;
                    }
                }
                if (v.v->part_flag(part, VPFLAG_OPAQUE) && v.v->parts[part].hp > 0) {
                    int dpart = v.v->part_with_feature(part , VPFLAG_OPENABLE);
                    if (dpart < 0 || !v.v->parts[dpart].open) {
                        psm.transparency[lx][ly] = LIGHT_TRANSPARENCY_SOLID;
                    }
                }
            }
        }
    }

    build_seen_cache();
    generate_lightmap();
}
#undef transparency_cache

std::vector<point> closest_points_first(int radius, point p)
{
    return closest_points_first(radius, p.x, p.y);
}

//this returns points in a spiral pattern starting at center_x/center_y until it hits the radius. clockwise fashion
//credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
std::vector<point> closest_points_first(int radius, int center_x, int center_y)
{
    std::vector<point> points;
    int X,Y,x,y,dx,dy;
    X = Y = (radius * 2) + 1;
    x = y = dx = 0;
    dy = -1;
    int t = std::max(X,Y);
    int maxI = t * t;
    for(int i = 0; i < maxI; i++)
    {
        if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
        {
            points.push_back(point(x + center_x, y + center_y));
        }
        if( (x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
        {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}

std::vector<tripoint> closest_points_first(int radius, const tripoint &p)
{
    // TODO: Z
    std::vector<point> tmp = closest_points_first(radius, p.x, p.y);
    std::vector<tripoint> res;
    for(std::vector<point>::const_iterator a = tmp.begin(); a != tmp.end(); ++a) {
        res.push_back(tripoint(a->x, a->y, p.z));
    }
    return res;
}

std::vector<tripoint> map::route(const tripoint &F, const tripoint &T, const bool can_bash) const
{
    // TODO: Z
    std::vector<point> tmp = route(F.x, F.y, T.x, T.y, can_bash);
    std::vector<tripoint> res;
    for(std::vector<point>::const_iterator a = tmp.begin(); a != tmp.end(); ++a) {
        res.push_back(tripoint(a->x, a->y, F.z));
    }
    return res;
}
//////////
///// coordinate helpers

point map::getabs(const int x, const int y) const
{
    return point( x + abs_sub.x * SEEX, y + abs_sub.y * SEEY );
}

point map::getlocal(const int x, const int y) const {
    return point( x - abs_sub.x * SEEX, y - abs_sub.y * SEEY );
}

void map::set_abs_sub(const int x, const int y, const int z)
{
    abs_sub = tripoint( x, y, z );
}

tripoint map::get_abs_sub() const
{
   return abs_sub;
}

submap *map::getsubmap( const int grididx ) const
{
    if( grididx < 0 || grididx >= static_cast<int>( grid.size() ) ) {
        debugmsg( "Tried to access invalid grid index %d", grididx );
        return nullptr;
    }
    return grid[grididx];
}

void map::setsubmap( const int grididx, submap * const smap )
{
    if( grididx < 0 || grididx >= my_MAPSIZE * my_MAPSIZE ) {
        debugmsg( "Tried to access invalid grid index %d", grididx );
        return;
    } else if( smap == nullptr ) {
        debugmsg( "Tried to set NULL submap pointer at index %d", grididx );
        return;
    }
    grid[grididx] = smap;
}

submap *map::get_submap_at( const int x, const int y ) const
{
    return get_submap_at( x, y, 0 );
}

submap *map::get_submap_at( const int x, const int y, const int z ) const
{
    return get_submap_at( tripoint( x, y, z ) );
}

submap *map::get_submap_at( const tripoint p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position (%d,%d,%d)", p.x, p.y, p.z );
        return nullptr;
    }
    return get_submap_at_grid( tripoint( p.x / SEEX, p.y / SEEY, p.z ) );
}

submap *map::get_submap_at( const int x, const int y, int &offset_x, int &offset_y ) const
{
    return get_submap_at( x, y, 0, offset_x, offset_y );
}

submap *map::get_submap_at( const int x, const int y, const int z, int &offset_x, int &offset_y ) const
{
    return get_submap_at( tripoint( x, y, z ), offset_x, offset_y );
}

submap *map::get_submap_at( const tripoint p, int &offset_x, int &offset_y ) const
{
    offset_x = p.x % SEEX;
    offset_y = p.y % SEEY;
    return get_submap_at( p );
}

submap *map::get_submap_at_grid( const point gp ) const
{
    return get_submap_at_grid( tripoint( gp.x, gp.y, 0 ) );
}

submap *map::get_submap_at_grid( const tripoint gp ) const
{
    return getsubmap( get_nonant( gp ) );
}

int map::get_nonant( const point gp ) const
{
    return get_nonant( tripoint( gp.x, gp.y, 0 ) );
}

bool map::valid_gp( const tripoint gp ) const
{
    return gp.x >= 0 && gp.x < my_MAPSIZE &&
           gp.y >= 0 && gp.y < my_MAPSIZE &&
           gp.z >= my_ZMIN && gp.z <= my_ZMAX;
}

int map::get_nonant( const tripoint gp ) const
{
    if( gp.x < 0 || gp.x >= my_MAPSIZE || gp.y < 0 || gp.y >= my_MAPSIZE || gp.z < my_ZMIN || gp.z > my_ZMAX ) {
        debugmsg( "Tried to access invalid map position at grid (%d,%d,%d)", gp.x, gp.y, gp.z );
        return 0;
    }
    return gp.x + gp.y * my_MAPSIZE + ( gp.z - my_ZMIN ) * my_MAPSIZE * my_MAPSIZE;
}

tinymap::tinymap(int mapsize, int z_min, int z_max)
: map(mapsize, z_min, z_max)
{
}

ter_id find_ter_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( termap.find(id) == termap.end() ) {
         debugmsg("Can't find termap[%s]",id.c_str());
         return 0;
    }
    return termap[id].loadid;
};

ter_id find_furn_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( furnmap.find(id) == furnmap.end() ) {
         debugmsg("Can't find furnmap[%s]",id.c_str());
         return 0;
    }
    return furnmap[id].loadid;
};
void map::draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2)
{
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        ter_set(i.x, i.y, type);
    }
    ter_set(x1, y1, type);
}
void map::draw_line_ter(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_ter(find_ter_id(type), x1, y1, x2, y2);
}


void map::draw_line_furn(furn_id type, int x1, int y1, int x2, int y2) {
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        furn_set(i.x, i.y, type);
    }
    furn_set(x1, y1, type);
}
void map::draw_line_furn(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_fill_background(ter_id type) {
    draw_square_ter(type, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(std::string type) {
    draw_square_ter(find_ter_id(type), 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(ter_id (*f)()) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(const id_or_id & f) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}


void map::draw_square_ter(ter_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, type);
        }
    }
}
void map::draw_square_ter(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_ter(find_ter_id(type), x1, y1, x2, y2);
}

void map::draw_square_furn(furn_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            furn_set(x, y, type);
        }
    }
}
void map::draw_square_furn(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f());
        }
    }
}

void map::draw_square_ter(const id_or_id & f, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f.get());
        }
    }
}

void map::draw_rough_circle(ter_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                ter_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle(std::string type, int x, int y, int rad) {
    draw_rough_circle(find_ter_id(type), x, y, rad);
}

void map::draw_rough_circle_furn(furn_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                furn_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle_furn(std::string type, int x, int y, int rad) {
    draw_rough_circle(find_furn_id(type), x, y, rad);
}

void map::add_corpse(int x, int y) {
    item body;

    const bool isReviveSpecial = one_in(10);

    if (!isReviveSpecial){
        body.make_corpse("corpse", GetMType("mon_null"), 0);
    } else {
        body.make_corpse("corpse", GetMType("mon_zombie"), 0);
        body.item_tags.insert("REVIVE_SPECIAL");
        body.active = true;
    }

    add_item_or_charges(x, y, body);
    put_items_from_loc( "shoes",  x, y, 0);
    put_items_from_loc( "pants",  x, y, 0);
    put_items_from_loc( "shirts", x, y, 0);
    if (one_in(6)) {
        put_items_from_loc("jackets", x, y, 0);
    }
    if (one_in(15)) {
        put_items_from_loc("bags", x, y, 0);
    }
}

/**
 * Adds vehicles to the current submap, selected from a random weighted
 * distribution of possible vehicles. If the road has a pavement, then set the
 * 'city' flag to true to spawn wrecks. If it doesn't (ie, highway or country
 * road,) then set 'city' to false to spawn far fewer vehicles that are out
 * of gas instead of wrecked.
 * @param city Whether or not to spawn city wrecks.
 * @param facing The direction the spawned car should face (multiple of 90).
 */
void map::add_road_vehicles(bool city, int facing)
{
    if (city) {
        int spawn_type = rng(0, 100);
        if(spawn_type <= 33) {
            //Randomly-distributed wrecks
            int maxwrecks = rng(1, 3);
            for (int nv = 0; nv < maxwrecks; nv++) {
                int vx = rng(0, 19);
                int vy = rng(0, 19);
                int car_type = rng(1, 100);
                if (car_type <= 25) {
                    add_vehicle("car", vx, vy, facing, -1, 1);
                } else if (car_type <= 30) {
                    add_vehicle("policecar", vx, vy, facing, -1, 1);
                } else if (car_type <= 39) {
                    add_vehicle("ambulance", vx, vy, facing, -1, 1);
                } else if (car_type <= 40) {
                    add_vehicle("bicycle_electric", vx, vy, facing, -1, 1);
                } else if (car_type <= 45) {
                    add_vehicle("beetle", vx, vy, facing, -1, 1);
                } else if (car_type <= 48) {
                    add_vehicle("car_sports", vx, vy, facing, -1, 1);
                } else if (car_type <= 50) {
                    add_vehicle("scooter", vx, vy, facing, -1, 1);
                } else if (car_type <= 53) {
                    add_vehicle("scooter_electric", vx, vy, facing, -1, 1);
                } else if (car_type <= 55) {
                    add_vehicle("motorcycle", vx, vy, facing, -1, 1);
                } else if (car_type <= 65) {
                    add_vehicle("hippie_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 70) {
                    add_vehicle("cube_van_cheap", vx, vy, facing, -1, 1);
                } else if (car_type <= 75) {
                    add_vehicle("cube_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 80) {
                    add_vehicle("electric_car", vx, vy, facing, -1, 1);
                } else if (car_type <= 90) {
                    add_vehicle("flatbed_truck", vx, vy, facing, -1, 1);
                } else if (car_type <= 95) {
                    add_vehicle("rv", vx, vy, facing, -1, 1);
                } else if (car_type <= 96) {
                    add_vehicle("lux_rv", vx, vy, facing, -1, 1);
                } else if (car_type <= 98) {
                    add_vehicle("meth_lab", vx, vy, facing, -1, 1);
                } else if (car_type <= 99) {
                    add_vehicle("apc", vx, vy, facing, -1, 1);
                } else {
                    add_vehicle("motorcycle_sidecart", vx, vy, facing, -1, 1);
                }
            }
        } else if(spawn_type <= 66) {
            //Parked vehicles
            int veh_x = 0;
            int veh_y = 0;
            if(facing == 0) {
                veh_x = rng(4, 16);
                veh_y = 17;
            } else if(facing == 90) {
                veh_x = 6;
                veh_y = rng(4, 16);
            } else if(facing == 180) {
                veh_x = rng(4, 16);
                veh_y = 6;
            } else if(facing == 270) {
                veh_x = 17;
                veh_y = rng(4, 16);
            }
            int veh_type = rng(0, 100);
            if(veh_type <= 70) {
                add_vehicle("car", veh_x, veh_y, facing, -1, 1);
            } else if(veh_type <= 92) {
                add_vehicle("electric_car", veh_x, veh_y, facing, -1, 1);
            } else if(veh_type <= 95) {
                add_vehicle("road_roller", veh_x, veh_y, facing, -1, 1);
            } else {
                add_vehicle("policecar", veh_x, veh_y, facing, -1, 1);
            }
        } else if(spawn_type <= 99) {
            //Totally clear section of road
            return;
        } else {
            //Road-blocking obstacle of some kind.
            int block_type = rng(0, 100);
            if(block_type <= 75) {
                //Jack-knifed semi
                int semi_x = 0;
                int semi_y = 0;
                int trailer_x = 0;
                int trailer_y = 0;
                if(facing == 0) {
                    semi_x = rng(0, 16);
                    semi_y = rng(14, 16);
                    trailer_x = semi_x + 4;
                    trailer_y = semi_y - 10;
                } else if(facing == 90) {
                    semi_x = rng(0, 8);
                    semi_y = rng(4, 15);
                    trailer_x = semi_x + 12;
                    trailer_y = semi_y + 1;
                } else if(facing == 180) {
                    semi_x = rng(4, 16);
                    semi_y = rng(4, 6);
                    trailer_x = semi_x - 4;
                    trailer_y = semi_y + 10;
                } else {
                    semi_x = rng(12, 20);
                    semi_y = rng(5, 16);
                    trailer_x = semi_x - 12;
                    trailer_y = semi_y - 1;
                }
                add_vehicle("semi_truck", semi_x, semi_y, (facing + 135) % 360, -1, 1);
                add_vehicle("truck_trailer", trailer_x, trailer_y, (facing + 90) % 360, -1, 1);
            } else {
                //Huge pileup of random vehicles
                std::string next_vehicle;
                int num_cars = rng(18, 22);
                bool policecars = block_type >= 95; //Policecar pileup, Blues Brothers style
                vehicle *last_added_car = NULL;
                for(int i = 0; i < num_cars; i++) {
                    if(policecars) {
                        next_vehicle = "policecar";
                    } else {
                        //Random car
                        int car_type = rng(0, 100);
                        if(car_type <= 70) {
                            next_vehicle = "car";
                        } else if(car_type <= 90) {
                            next_vehicle = "pickup";
                        } else if(car_type <= 95) {
                            next_vehicle = "cube_van";
                        } else {
                            next_vehicle = "hippie_van";
                        }
                    }
                    last_added_car = add_vehicle(next_vehicle, rng(4, 16), rng(4, 16), rng(0, 3) * 90, -1, 1);
                }

                //Hopefully by the last one we've got a giant pileup, so name it
                if (last_added_car != NULL) {
                    if(policecars) {
                        last_added_car->name = _("policecar pile-up");
                    } else {
                        last_added_car->name = _("pile-up");
                    }
                }
            }
        }
    } else {
        // spawn regular road out of fuel vehicles
        if (one_in(40)) {
            int vx = rng(8, 16);
            int vy = rng(8, 16);
            int car_type = rng(1, 27);
            if (car_type <= 10) {
                add_vehicle("car", vx, vy, facing, 0, -1);
            } else if (car_type <= 14) {
                add_vehicle("car_sports", vx, vy, facing, 0, -1);
            } else if (car_type <= 16) {
                add_vehicle("pickup", vx, vy, facing, 0, -1);
            } else if (car_type <= 18) {
                add_vehicle("semi_truck", vx, vy, facing, 0, -1);
            } else if (car_type <= 20) {
                add_vehicle("humvee", vx, vy, facing, 0, -1);
            } else if (car_type <= 24) {
                add_vehicle("rara_x", vx, vy, facing, 0, -1);
            } else if (car_type <= 25) {
                add_vehicle("apc", vx, vy, facing, 0, -1);
            } else {
                add_vehicle("armored_car", vx, vy, facing, 0, -1);
            }
        }
    }
}
