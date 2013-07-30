#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

class vehicle;

enum vpart_id
{
    vp_null = 0,

// external parts
	/*
    vp_seat,
    vp_saddle,
    vp_bed,
    vp_frame_h,
    vp_frame_v,
    vp_frame_c,
    vp_frame_y,
    vp_frame_u,
    vp_frame_n,
    vp_frame_b,
    vp_frame_h2,
    vp_frame_v2,
    vp_frame_cover,
    vp_frame_handle,
    vp_board_h,
    vp_board_v,
    vp_board_y,
    vp_board_u,
    vp_board_n,
    vp_board_b,
    vp_aisle_h2,
    vp_aisle_v2,
    vp_floor_trunk,
    vp_roof,
    vp_door,
    vp_door_o,
    vp_door_i,
    vp_window,
    vp_blade_h,
    vp_blade_v,
    vp_spike_h,
    vp_spike_v = vp_spike_h,

    vp_wheel,
    vp_wheel_wide,
    vp_wheel_bicycle,
    vp_wheel_motorbike,
    vp_wheel_small,

    vp_engine_gas_1cyl,
    vp_engine_gas_v2,
    vp_engine_gas_i4,
    vp_engine_gas_v6,
    vp_engine_gas_v8,
    vp_engine_motor,
    vp_engine_motor_large,
    vp_engine_plasma,
    vp_engine_foot_crank,
    vp_fuel_tank_gas,
    vp_fuel_tank_batt,
    vp_fuel_tank_plut,
    vp_fuel_tank_hydrogen,
    vp_fuel_tank_water,
    vp_cargo_trunk, // over
    vp_cargo_box,   // over

// pure internal parts
    vp_controls,
    vp_muffler,
    vp_seatbelt,
    vp_solar_panel,
    vp_kitchen_unit,
    vp_m249,
    vp_flamethrower,
    vp_plasmagun,

// plating -- special case. mounted as internal, work as first line
// of defence and gives color to external part
    vp_steel_plate,
    vp_superalloy_plate,
    vp_spiked_plate,
    vp_hard_plate,

    vp_head_light,
    vp_longbow,

    num_vparts
    */
};

enum vpart_flags
{
    vpf_external,           // can be mounted as external part
    vpf_internal,           // can be mounted inside other part
    vpf_mount_point,        // allows mounting other parts to it
    vpf_mount_inner,        // allows mounting internal parts inside it (or over it)
    vpf_mount_over,         // allows mounting parts like cargo trunk over it
    vpf_anchor_point,       // Allows secure attachment of a seatbelt
    vpf_opaque,             // can't see through it
    vpf_obstacle,           // can't pass through it
    vpf_openable,           // can open/close it
    vpf_no_reinforce,       // can't reinforce this part with armor plates
    vpf_sharp,              // cutting damage instead of bashing
    vpf_unmount_on_damage,  // when damaged, part is unmounted, rather than broken
    vpf_boardable,          // part can carry passengers
    vpf_variable_size,      // has 'bigness' for power, wheel radius, etc.
   
	num_vpflags
};

// functional flags (only one of each can be mounted per tile)
enum vpart_function {
    vpc_over,               // can be mounted over other part
    vpc_roof,               // is a roof (cover)
    vpc_wheel,              // this part touches ground (trigger traps)
    vpc_seat,               // is seat
    vpc_bed,                // is bed (like seat, but can't be boarded)
    vpc_aisle,              // is aisle (no extra movement cost)
    vpc_engine,             // is engine
    vpc_kitchen,            // is kitchen
    vpc_fuel_tank,          // is fuel tank
    vpc_cargo,              // is cargo
    vpc_controls,           // is controls
    vpc_muffler,            // is muffler
    vpc_seatbelt,           // is seatbelt
    vpc_solar_panel,        // is solar panel
    vpc_turret,             // is turret
    vpc_armor,              // is armor plating
    vpc_light,              // generates light arc
	vpc_horn,               // vehicle horn
	vpc_examine,            // can be examined, use vpart_info::examine to do so
	vpc_remote_controll,    // can be remotly controlled
    
	num_vpfunctions
};

struct vpart_info
{
    std::string name;       // part name
    long sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    union
    {
        int par1;
        int power;      // engine (top spd), solar panel (% of 1 fuel per turn, can be > 100)
        int size;       // fuel tank, trunk
        int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
        int bonus;      // seatbelt (str), muffler (%)
    };
    int par2;
    std::string fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    
private:
    unsigned long flags;    // flags (vpart_flags)
    unsigned long functions;    // functions (vpart_function)
public:
	
	vpart_info(
		const std::string &name, long sym, nc_color color, char sym_broken,
		nc_color color_broken, int dmg_mod, int durability, int par1, int par2,
		const std::string &fuel_type, const itype_id &item, int difficulty,
		int flags, int functions
  	) : name(name), sym(sym), color(color), sym_broken(sym_broken),
  	color_broken(color_broken), dmg_mod(dmg_mod), durability(durability),
  	par1(par1), par2(par2), fuel_type(fuel_type), item(item),
  	difficulty(difficulty), flags(flags), functions(functions) { }

    static int getNumberOfParts();
	static const vpart_info &getVehiclePartInfo(vpart_id index);

	bool has_function(vpart_function function) const;
	bool has_flag(vpart_flags flag) const;
	
	typedef std::pair<itype_id, int> type_count_pair;
	typedef std::vector<type_count_pair> type_count_pair_vector;
	
	/**
	 * Get the materials needed for repairing this part,
	 * @param hp the current hp of the part.
	 */
	type_count_pair_vector get_repair_materials(int hp) const;
	
	/**
	 * Do something, is called each turn from vehicle::gain_moves,
	 * for each part of the car.
	 */
	void on_turn(vehicle *veh, int part) const;
	
	/**
	 * Examine the part.
	 * @return false - if the normal examine function should continue.
	 * (that means this examine function did nothing).
	 * true - we had some interaction with the player, eaxamining
	 * has finished, return to main loop.
	 */
	bool examine(vehicle *veh, int part) const;
};



enum vhtype_id
{
    veh_null = 0,
    veh_custom,

// in-built vehicles
    veh_bicycle,
    veh_motorcycle_chassis,
    veh_motorcycle,
    veh_sandbike,
    veh_sandbike_chassis,
    veh_car,
    veh_car_chassis,
    veh_truck,
    veh_semi,  //6L Semitruck. 10 wheels. Sleeper cab.
    veh_trucktrailer,  //Just a trailer with 8 wheels.
    veh_wagon, // Dwarf Fortress Wagon
    veh_bug,  //Old VW Bug.
    veh_bubblecar,  //360 degree view glass circular vehicle. Underpowered plutonium.
    veh_golfcart,  //Yamaha golf cart.
    veh_scooter,  //Vespa S50 scooter.
    veh_armytruck,  //Army M35A2 6L gas and/or hydrogen engine if commented parts uncommented.
    veh_schoolbus,  //Standard schoolbus
    veh_car_electric, // electric version of standard car.

	veh_wheelbarrow,

    num_vehicles
};

#endif
