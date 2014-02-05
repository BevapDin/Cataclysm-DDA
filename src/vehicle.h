#ifndef _VEHICLE_H_
#define _VEHICLE_H_

#include "tileray.h"
#include "color.h"
#include "item.h"
#include "line.h"
#include "veh_type.h"
#include <vector>
#include <string>
#include <fstream>

class map;
class player;

//collision factor for vehicle-vehicle collision; delta_v in mph
float get_collision_factor(float delta_v);

//How far to scatter parts from a vehicle when the part is destroyed (+/-)
#define SCATTER_DISTANCE 3

#define num_fuel_types 5
extern const ammotype fuel_types[num_fuel_types];
#define k_mvel 200 //adjust this to balance collision damage

// 0 - nothing, 1 - monster/player/npc, 2 - vehicle,
// 3 - thin_obstacle, 4 - bashable, 5 - destructible, 6 - other
enum veh_coll_type {
 veh_coll_nothing = 0,
 veh_coll_body,
 veh_coll_veh,
 veh_coll_thin_obstacle,
 veh_coll_bashable,
 veh_coll_destructable,
 veh_coll_other,

 num_veh_coll_types
};

struct vparzu;
struct veh_collision {
 //int veh?
 vparzu *part;
 veh_coll_type type;
 int imp; // impulse

 void* target;  //vehicle
 vparzu *target_part; //veh partnum
 std::string target_name;
 veh_collision() : part(NULL), type(veh_coll_nothing), imp(0), target(NULL), target_part(NULL), target_name("") {};
};

struct vehicle_item_spawn
{
    int x, y;
    int chance;
    std::vector<std::string> item_ids;
    std::vector<std::string> item_groups;
};

struct vehicle_prototype
{
    std::string id, name;
    std::vector<std::pair<point, std::string> > parts;
    std::vector<vehicle_item_spawn> item_spawns;
};

/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part2 : public JsonSerializer, public JsonDeserializer
{
    vehicle_part2();

    bool has_flag( int flag ) const { return flag & flags; }
    int set_flag( int flag ) { return flags |= flag; }
    int remove_flag( int flag ) { return flags &= ~flag; }

    static const int passenger_flag = 1;

    std::string id;         // id in map of parts (vehicle_part_types key)
    int iid;                // same as above, for lookup via int
    int hp;                 // current durability, if 0, then broken
    int blood;              // how much blood covers part (in turns).
    int bigness;            // size of engine, wheel radius, translates to item properties.
    int flags;
    int passenger_id;       // carrying passenger
    union
    {
        int amount;         // amount of fuel for tank/charge in battery
        int open;           // door is open
        int direction;      // direction the part is facing
    };
    std::vector<item> items;// inventory

    bool setid(const std::string str) {
        std::map<std::string, vpart_info>::const_iterator vpit = vehicle_part_types.find(str);
        if ( vpit == vehicle_part_types.end() ) {
            return false;
        }
        id = str;
        iid = vpit->second.loadid;
        return true;
    }

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

    // Ignores hp completely
    bool has_flag(const std::string &f) const;
    bool has_flag(const vpart_bitflags &f) const;

    // Check flag of vpart_info and (optionally) the hp
    bool has_feature(const std::string &f, bool unbroken) const;
    bool has_feature(const vpart_bitflags &f, bool unbroken) const;

    // get passenger on this part, or NULL if no passenger is here
    player* get_passenger() const;
    bool has_passenger() const;
    // get vpart type info for part
    const vpart_info& part_info() const;

    int max_volume() const;
    int free_volume() const;
    int stored_volume() const;
    bool is_full(const int addvolume = -1, const int addnumber = -1) const;

    // add item to part's cargo. if false, then there's no cargo at this part or cargo is full(*)
    bool add_item(const item &itm);
    // remove item from part's cargo
    void remove_item(int itemdex);

    // get vpart powerinfo for part number, accounting for variable-sized parts.
    int part_power() const;
    // get vpart epowerinfo for part number.
    int part_epower() const;

    // Includes passageners, items and the vehicle part item itself
    int total_mass() const;

    // Name from vpart_info
    const std::string &name() const;
    // location from vpart_info
    const std::string &location() const;
    // fuel_type from vpart_info
    const ammotype &fuel_type() const;

    // if this is a tank of the given fuel type, return the amount
    // of fuel in it, otherwise return 0
    int fuel_left(const ammotype &ftype) const;
    
    bool is_broken() const { return hp <= 0; }
    point get_global_pos() const;

    // Generate the corresponding item from a vehicle part.
    // Still needs to be removed.
    item item_from_part() const;
    // translate item health to part health
    void get_part_properties_from_item(const item& i);
    // translate part health to item health (very lossy.)
    void give_part_properties_to_item(item& i) const;

    void break_part_into_pieces(int x, int y, bool scatter/*=false*/) const;
protected:
    friend vparzu;
    vparzu *vp;
};

/**
 * A collection of all parts that are mounted on the same tile
 * on a vehicle.
 */
struct vparzu : public JsonSerializer, public JsonDeserializer
{
    vparzu();
protected:
    // not implemented!
    vparzu(const vparzu&);
    vparzu& operator=(const vparzu&);
public:
    int mount_dx;
    int mount_dy;
    int precalc_dx[2];      // mount_dx translated to face.dir [0] and turn_dir [1]
    int precalc_dy[2];      // mount_dy translated to face.dir [0] and turn_dir [1]

    // The parts mounted here.
    std::vector<vehicle_part2> parts;

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

    // returns the first vehicle part from @ref parts
    // that has this feature (and optional is not broken).
    // returns NULL if no part matches
    vehicle_part2 *part_with_feature(const std::string &f, bool unbroken);
    const vehicle_part2 *part_with_feature(const std::string &f, bool unbroken) const;
    vehicle_part2 *part_with_feature(const vpart_bitflags &f, bool unbroken);
    const vehicle_part2 *part_with_feature(const vpart_bitflags &f, bool unbroken) const;

    // shortcut for part_with_feature(...) != NULL
    bool has_part_with_feature(const std::string &f, bool unbroken) const;
    bool has_part_with_feature(const vpart_bitflags &f, bool unbroken) const;

    // ignores hp completely
    bool has_part_with_flag(const std::string &f) const;
    bool has_part_with_flag(const vpart_bitflags &f) const;

    // Returns the first part of the given type.
    // or NULL if there is no such part.
    vehicle_part2 *part_of_type(const std::string &type, bool unbroken);
    const vehicle_part2 *part_of_type(const std::string &type, bool unbroken) const;

    // get passenger on this part, or NULL if no passenger is here
    player *get_passenger() const;
    bool has_passenger() const;
    // get color for map
    nc_color part_color() const;
    // get symbol for map
    char part_sym() const;
    vehicle_part2 *part_displayed();
    const vehicle_part2 *part_displayed() const;
    
    bool is_transparent() const;
    bool is_obstacle() const;
    int move_cost() const;
    bool is_inside() const;
    // has roof or is obstacle, includes hp-checks
    bool has_roof() const;
    int total_mass() const;
    // only parts, no items in it, no passagener
    int part_mass() const;

    // TODO: document this
    std::string part_id_string(char &part_mod) const;

private:
    friend class vehicle;
    bool inside;
    vehicle *veh;

    // see vehicle::add_missing_frames
    void add_missing_frame();
};

typedef std::map<point, vparzu> vparmap;

struct vehicle_part : public vehicle_part2
{
    vehicle_part() : vehicle_part2(), mount_dx(0), mount_dy(0)
    {
        precalc_dx[0] = precalc_dx[1] = -1;
        precalc_dy[0] = precalc_dy[1] = -1;
    }

    static const int passenger_flag = 1;

    int mount_dx;           // mount point on the forward/backward axis
    int mount_dy;           // mount point on the left/right axis
    int precalc_dx[2];      // mount_dx translated to face.dir [0] and turn_dir [1]
    int precalc_dy[2];      // mount_dy translated to face.dir [0] and turn_dir [1]

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);
};

/**
 * A vehicle as a whole with all its components.
 *
 * This object can occupy multiple tiles, the objects actually visible
 * on the map are of type `vehicle_part`.
 *
 * Facts you need to know about implementation:
 * - Vehicles belong to map. There's `std::vector<vehicle>`
 *   for each submap in grid. When requesting a reference
 *   to vehicle, keep in mind it can be invalidated
 *   by functions such as `map::displace_vehicle()`.
 * - To check if there's any vehicle at given map tile,
 *   call `map::veh_at()`, and check vehicle type (`veh_null`
 *   means there's no vehicle there).
 * - Vehicle consists of parts (represented by vector). Parts have some
 *   constant info: see veh_type.h, `vpart_info` structure and
 *   vpart_list array -- that is accessible through `part_info()` method.
 *   The second part is variable info, see `vehicle_part` structure.
 * - Parts are mounted at some point relative to vehicle position (or starting part)
 *   (`0, 0` in mount coords). There can be more than one part at
 *   given mount coords, and they are mounted in different slots.
 *   Check tileray.h file to see a picture of coordinate axes.
 * - Vehicle can be rotated to arbitrary degree. This means that
 *   mount coords are rotated to match vehicle's face direction before
 *   their actual positions are known. For optimization purposes
 *   mount coords are precalculated for current vehicle face direction
 *   and stored in `precalc_*[0]`. `precalc_*[1]` stores mount coords for
 *   next move (vehicle can move and turn). Method `map::displace_vehicle()`
 *   assigns `precalc[1]` to `precalc[0]`. At any time (except
 *   `map::vehmove()` innermost cycle) you can get actual part coords
 *   relative to vehicle's position by reading `precalc_*[0]`.
 * - Vehicle keeps track of 3 directions:
 *     Direction | Meaning
 *     --------- | -------
 *     face      | where it's facing currently
 *     move      | where it's moving, it's different from face if it's skidding
 *     turn_dir  | where it will turn at next move, if it won't stop due to collision
 * - Some methods take `part` or `p` parameter. This is the index of a part in
 *   the parts list.
 * - Driver doesn't know what vehicle he drives.
 *   There's only player::in_vehicle flag which
 *   indicates that he is inside vehicle. To figure
 *   out what, you need to ask a map if there's a vehicle
 *   at driver/passenger position.
 * - To keep info consistent, always use
 *   `map::board_vehicle()` and `map::unboard_vehicle()` for
 *   boarding/unboarding player.
 * - To add new predesigned vehicle, add an entry to data/raw/vehicles.json
 *   similar to the existing ones. Keep in mind, that positive x coordinate points
 *   forwards, negative x is back, positive y is to the right, and
 *   negative y to the left:
 *
 *       orthogonal dir left (-Y)
 *            ^
 *       -X ------->  +X (forward)
 *            v
 *       orthogonal dir right (+Y)
 *
 *   When adding parts, function checks possibility to install part at given
 *   coords. If it shows debug messages that it can't add parts, when you start
 *   the game, you did something wrong.
 *   There are a few rules:
 *   1. Every mount point (tile) must begin with a part in the 'structure'
 *      location, usually a frame.
 *   2. No part can stack with itself.
 *   3. No part can stack with another part in the same location, unless that
 *      part is so small as to have no particular location (such as headlights).
 *   If you can't understand why installation fails, try to assemble your
 *   vehicle in game first.
 */
class vehicle : public JsonSerializer, public JsonDeserializer
{
public:
    typedef std::vector<vparzu*> vzpvec;
private:
    bool has_structural_part(int dx, int dy) const;
    void open_or_close(vparzu *part, bool opening);
    bool is_connected(const vparzu *to, const vparzu *from, const vparzu *excluded) const;
    void add_missing_frames();

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct (vehicle_part2 &p, int dmg, int type = 1);
    vparzu *find_parzu(vehicle_part2 *p);

    // convert epower (watts) to power.
    static int epower_to_power(int epower);
    // convert power to epower (watts).
    static int power_to_epower(int power);

    //Refresh all caches and re-locate all parts
    void refresh();

public:
    vehicle (std::string type_id = "null", int veh_init_fuel = -1, int veh_init_status = -1);
    ~vehicle ();

// check if given player controls this vehicle
    bool player_in_control (player *p) const;

// init parts state for randomly generated vehicle
    void init_state(int veh_init_fuel, int veh_init_status);

// damages all parts of a vehicle by a random amount
    void smash();

// load and init vehicle data from stream. This implies valid save data!
    void load_legacy(std::ifstream &stin);
    void load (std::ifstream &stin);

// Save vehicle data to stream
    void save (std::ofstream &stout);

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

// Operate vehicle
    void use_controls();

// Start the vehicle's engine, if there are any
    void start_engine();

// Honk the vehicle's horn, if there are any
    void honk_horn();

// check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount (int dx, int dy, std::string id) const;

// check if certain part can be unmounted
    bool can_unmount(const vehicle_part2 &p) const;

// install a new part to vehicle (force to skip possibility check)
    bool install_part (int dx, int dy, std::string id, int hp = -1, bool force = false);

    bool remove_part(vparzu *vp, vehicle_part2 *p);

// returns all parts in the vehicle with the given flag
    vzpvec all_parts_with_feature(const std::string &feature, bool unbroken);
    vzpvec all_parts_with_feature(const vpart_bitflags &f, bool unbroken);

// Translate seat-relative mount coords into tile coords
    void coord_translate (int reldx, int reldy, int &dx, int &dy);

// Translate seat-relative mount coords into tile coords using given face direction
    void coord_translate (int dir, int reldx, int reldy, int &dx, int &dy);

// Seek a vehicle part which obstructs tile with given coords relative to vehicle position
protected:
    int part_at (int dx, int dy);
    int global_part_at (int x, int y);
public:
    const vehicle_part2 *part_displayed_at(int local_x, int local_y) const;

// Given a part, finds its index in the vehicle
protected:
    int index_of_part(vehicle_part *part);
public:

// Vehicle parts description
    int print_part_desc (WINDOW *win, int y1, int width, const vparzu *p, int hl = -1);

// Vehicle fuel indicator. Should probably rename to print_fuel_indicators and make a print_fuel_indicator(..., FUEL_TYPE);
    void print_fuel_indicator (void *w, int y, int x, bool fullsize = false,
                               bool verbose = false, bool desc = false);

// Precalculate mount points for (idir=0) - current direction or (idir=1) - next turn direction
    void precalc_mounts (int idir, int dir);

// get a list of part indeces where is a passenger inside
    std::vector<vparzu*> boarded_parts();
    const vparzu *free_seat() const;

// get global coords for vehicle
    int global_x () const;
    int global_y () const;

// get omap coordinate for vehicle
    int omap_x ();
    int omap_y ();

// update map coordinates of the vehicle
    void update_map_x(int x);
    void update_map_y(int y);

// Checks how much certain fuel left in tanks.
    int fuel_left (const ammotype & ftype) const;
    int fuel_capacity (const ammotype & ftype) const;

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (const ammotype & ftype, int amount);

    // drains a fuel type (e.g. for the kitchen unit)
    // returns amount actually drained, does not engage reactor
    int drain (const ammotype & ftype, int amount);

// fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption (const ammotype & ftype) const;

    void consume_fuel (float rate);

    void power_parts ();

    void charge_battery (int amount);

    int discharge_battery (int amount);

// get the total mass of vehicle, including cargo and passengers
    int total_mass () const;

// get center of mass of vehicle; coordinates are precalc_dx[0] and precalc_dy[0]
    void center_of_mass(int &x, int &y) const;

// Get combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int total_power (bool fueled) const;

// Get combined epower of solar panels
    int solar_epower () const;

// Get acceleration gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int acceleration (bool fueled) const;

// Get maximum velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int max_velocity (bool fueled) const;

// Get safe velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int safe_velocity (bool fueled) const;

    int noise (bool fueled, bool gas_only = false) const;

// Calculate area covered by wheels and, optionally count number of wheels
    float wheels_area (int *cnt = 0) const;

// Combined coefficient of aerodynamic and wheel friction resistance of vehicle, 0-1.0.
// 1.0 means it's ideal form and have no resistance at all. 0 -- it won't move
    float k_dynamics () const;

// Coefficient of mass, 0-1.0.
// 1.0 means mass won't slow vehicle at all, 0 - it won't move
    float k_mass () const;

// strain of engine(s) if it works higher that safe speed (0-1.0)
    float strain () const;

// calculate if it can move using its wheels configuration
    bool valid_wheel_config () const;

// idle fuel consumption
    void idle ();

// thrust (1) or brake (-1) vehicle
    void thrust (int thd);

// depending on skid vectors, chance to recover.
    void possibly_recover_from_skid();

//forward component of velocity.
    float forward_velocity();

// cruise control
    void cruise_thrust (int amount);

// turn vehicle left (negative) or right (positive), degrees
    void turn (int deg);

    bool collision( std::vector<veh_collision> &veh_veh_colls,
                    std::vector<veh_collision> &veh_misc_colls, int dx, int dy,
                    bool &can_move, int &imp, bool just_detect = false );

// handle given part collision with vehicle, monster/NPC/player or terrain obstacle
// return collision, which has type, impulse, part, & target.
    veh_collision part_collision (vparzu &part, int x, int y, bool just_detect);

// Process the trap beneath
    void handle_trap (int x, int y);

// Generates starting items in the car, should only be called when placed on the map
    void place_spawn_items();

    void gain_moves();

// reduces velocity to 0
    void stop ();

    void find_special_parts();

    void find_power ();

    void find_exhaust ();

    void refresh_insides ();

    bool pedals();

    void unboard_all ();


    vparmap pmap;
    vparzu *vp_at(int x, int y) { return vp_at(point(x, y)); }
    vparzu *vp_at(const point &p);
    const vparzu *vp_at(int x, int y) const { return vp_at(point(x, y)); }
    const vparzu *vp_at(const point &p) const;
protected:
    vparzu &vpat(const point &p);
public:
    const vparmap &get_pmap() const { return pmap; }
    void merge(const vehicle *veh);


    // damage types:
    // 0 - piercing
    // 1 - bashing (damage applied if it passes certain treshold)
    // 2 - incendiary
    // damage individual part. bash means damage
    // must exceed certain threshold to be substracted from hp
    // (a lot light collisions will not destroy parts)
    // returns damage bypassed
    int damage (vparzu *vp, vehicle_part2 *pn, int dmg, int type = 1, bool aimed = true);
    int damage (vparzu *vp, int dmg, int type = 1, bool aimed = true);

    // damage all parts (like shake from strong collision), range from dmg1 to dmg2
    void damage_all (int dmg1, int dmg2, int type, const point &impact);

    //Shifts the coordinates of all parts and moves the vehicle in the opposite direction.
    void shift_parts(const int dx, const int dy);

protected:
    void leak_fuel (int p);
    void tear_off_part(vparzu *vp, vehicle_part2 *pp);
public:

    // fire the turret which is part p
    void fire_turret (vparzu *p, bool burst);

    // internal procedure of turret firing
    bool fire_turret_internal (vparzu *part, vehicle_part2 *p, it_gun &gun, it_ammo &ammo, int charges,
                               const std::string &firing_sound = "");

    // opens/closes doors or multipart doors
    void open(vparzu *part);
    void close(vparzu *part);

    // upgrades/refilling/etc. see veh_interact.cpp
    void interact ();

    // return a vector w/ 'direction' & 'magnitude', in its own sense of the words.
    rl_vec2d velo_vec();
    //normalized vectors, from tilerays face & move
    rl_vec2d face_vec();
    rl_vec2d move_vec();

    // config values
    std::string name;   // vehicle name
    std::string type;           // vehicle type
    void move_precalc();
protected:
    friend class veh_interact;
protected:
    // These are cached values, updated after dmage, install/removal
    // of parts. Done by refresh() function
    vzpvec horns;            // List of horn part indices
    vzpvec lights;           // List of light part indices
    vzpvec alternators;      // List of alternator indices
    vzpvec fuel;             // List of fuel tank indices
    vzpvec engines;          // List of engine indices
    vzpvec reactors;         // List of reactor indices
    vzpvec solar_panels;     // List of solar panel indices
    vzpvec wheelcache;
    bool has_pedals;
    int muffle;
    int exhaust_dx;
    int exhaust_dy;

public:
    std::vector<vehicle_item_spawn> item_spawns; //Possible starting items
    std::set<std::string> tags;        // Properties of the vehicle

    // temp values
    int smx, smy;   // submap coords. WARNING: must ALWAYS correspond to sumbap coords in grid, or i'm out
    int init_veh_fuel;
    int init_veh_status;
protected:
    float alternator_load;
public:

    // save values
    int posx, posy;
    int levx,levy;       // vehicle map coordinates.
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity;       // vehicle current velocity, mph * 100
    int cruise_velocity; // velocity vehicle's cruise control trying to acheive
    bool cruise_on;     // cruise control on/off
    bool reactor_on;    // reactor on/off
    bool engine_on;     // engine on/off
    bool lights_on;     // lights on/off
    bool tracking_on;        // vehicle tracking on/off
    int om_id;          // id of the om_vehicle struct corresponding to this vehicle
    bool overhead_lights_on; //circle lights on/off
    bool fridge_on;     //fridge on/off
    bool recharger_on;  //recharger on/off
    int turn_dir;       // direction, to wich vehicle is turning (player control). will rotate frame on next move
    bool skidding;      // skidding mode
    int last_turn;      // amount of last turning (for calculate skidding due to handbrake)
    //int moves;
    float of_turn;      // goes from ~1 to ~0 while proceeding every turn
    float of_turn_carry;// leftover from prev. turn
    int turret_mode;    // turret firing mode: 0 = off, 1 = burst fire
    int lights_epower;   // total power of components with LIGHT or CONE_LIGHT flag
    int overhead_epower;   // total power of components with CIRCLE_LIGHT flag
    int tracking_epower; // total power consumed by tracking devices (why would you use more than one?)
    int fridge_epower; // total power consumed by fridges
    int recharger_epower; // total power consumed by rechargers
};

#endif
