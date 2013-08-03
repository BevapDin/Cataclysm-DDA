#include "veh_type.h"
#include "game.h"
#include "ui.h"
#include "item.h"

// following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly

typedef std::vector<vpart_info> vpart_info_vector;

static vpart_info_vector &get_vpart_list() {
	static vpart_info_vector vpart_list;
	if(!vpart_list.empty()) {
		return vpart_list;
	}
	
		// name        sym   color    sym_b color_b  dmg  dur  par1 par2
		// fuel        item           diff
		// flags
		// functions
    vpart_list.push_back(vpart_info(
		"null part",   '?',  c_red,    '?',  c_red,   100, 100, 0, 0,
		"NULL",        "null",        0,
		0,
		0
	));
    vpart_list.push_back(vpart_info(
		"seat",       '#', c_red,     '*', c_red,     60,  300, 0, 0,
		"NULL", "seat", 1,
        mfb(vpf_boardable) | mfb(vpf_no_reinforce) | mfb(vpf_anchor_point),
		mfb(vpc_seat) | mfb(vpc_over) | mfb(vpc_cargo)
    ));
    vpart_list.push_back(vpart_info(
		"saddle",     '#', c_red,     '*', c_red,     20,  200, 0, 0,
		"NULL", "saddle", 1,
        mfb(vpf_boardable) | mfb(vpf_no_reinforce),
		mfb(vpc_seat) | mfb(vpc_over)
    ));
    vpart_list.push_back(vpart_info(
		"bed",        '#', c_magenta, '*', c_magenta, 60,  300, 0, 0,
		"NULL", "seat", 1,
        mfb(vpf_boardable) | mfb(vpf_no_reinforce),
		mfb(vpc_seat) | mfb(vpc_over) | mfb(vpc_cargo)
    ));
    vpart_list.push_back(vpart_info(
		"frame",      'h', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'j', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'c', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'y', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'u', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'n', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'b', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      '=', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      'H', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "frame",      '^', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "handle",     '^', c_ltcyan,  '#', c_ltcyan,  100, 300, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'h', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'j', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'y', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'u', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'n', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "board",      'b', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle),
		0
    ));
    vpart_list.push_back(vpart_info(
        "aisle",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_no_reinforce) | mfb(vpf_boardable),
		mfb(vpc_over) | mfb(vpc_aisle)
    ));
    vpart_list.push_back(vpart_info(
        "aisle",       'H', c_white,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_no_reinforce) | mfb(vpf_boardable),
		mfb(vpc_over) | mfb(vpc_aisle)
    ));
    vpart_list.push_back(vpart_info(
        "floor trunk",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0,
		"NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_no_reinforce) | mfb(vpf_boardable),
		mfb(vpc_over) | mfb(vpc_aisle) | mfb(vpc_cargo)
    ));
    vpart_list.push_back(vpart_info(
        "roof",       '#', c_ltgray,  '#', c_dkgray,  100, 1000, 0, 0,
		"NULL", "steel_plate", 1,
        mfb(vpf_internal),
		mfb(vpc_roof)
    ));
    vpart_list.push_back(vpart_info(
        "door",       '+', c_cyan,    '&', c_cyan,    80,  200, 0, 0,
		"NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_openable) | mfb(vpf_boardable),
		0
    ));
    vpart_list.push_back(vpart_info(
        "opaque door",'+', c_cyan,    '&', c_cyan,    80,  200, 0, 0,
		"NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_boardable),
		0
    ));
    vpart_list.push_back(vpart_info(
        "internal door", '+', c_cyan, '&', c_cyan,    75,  75, 0, 0,
		"NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_no_reinforce) | mfb(vpf_boardable),
		mfb(vpc_roof)
    ));
    vpart_list.push_back(vpart_info(
        "windshield", '"', c_ltcyan,  '0', c_ltgray,  70,  50, 0, 0,
		"NULL", "glass_sheet", 1,
        mfb(vpf_obstacle) | mfb(vpf_no_reinforce),
		mfb(vpc_over)
    ));
    vpart_list.push_back(vpart_info(
        "blade",      '-', c_white,   'x', c_white,   250, 100, 0, 0,
		"NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce),
		0
    ));
    vpart_list.push_back(vpart_info(
        "blade",      '|', c_white,   'x', c_white,   350, 100, 0, 0,
		"NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce),
		0
    ));
    vpart_list.push_back(vpart_info(
        "spike",      '.', c_white,   'x', c_white,   300, 100, 0, 0,
		"NULL", "spike", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce),
		0
    ));

//                                                           wheel_width(inches)
    vpart_list.push_back(vpart_info(
        "wheel",      '0',    c_dkgray,  'x', c_ltgray,  50,  200, 9, 0,
		"NULL", "wheel", 4,
        mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_mount_point) | mfb(vpf_variable_size),
		mfb(vpc_wheel)
    ));
    vpart_list.push_back(vpart_info(
        "wide wheel", 'O',     c_dkgray,   'x', c_ltgray,  50,  400, 14, 0,
		"NULL", "wheel_wide", 5,
        mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_mount_point) | mfb(vpf_variable_size),
		mfb(vpc_wheel)
    ));
    vpart_list.push_back(vpart_info(
        "bicycle wheel",'|',  c_dkgray, 'x', c_ltgray,  50,  40, 2, 0,
		"NULL", "wheel_bicycle", 1,
        mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_mount_point) | mfb(vpf_variable_size),
		mfb(vpc_wheel)
    ));
    vpart_list.push_back(vpart_info(
        "motorbike wheel",'o',c_dkgray, 'x', c_ltgray,  50,  90, 4, 0,
		"NULL", "wheel_motorbike", 2,
        mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_mount_point) | mfb(vpf_variable_size),
		mfb(vpc_wheel)
    ));
    vpart_list.push_back(vpart_info(
        "small wheel",    'o',c_dkgray, 'x', c_ltgray,  50,  70, 6, 0,
		"NULL", "wheel_small", 2,
        mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_mount_point) | mfb(vpf_variable_size),
		mfb(vpc_wheel)
    ));
//
    vpart_list.push_back(vpart_info(
        "1-cylinder engine",    '*', c_ltred,  '#', c_red,     80, 150, 40, 0,
		"gasoline", "1cyl_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_variable_size),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "V-twin engine",       '*', c_ltred,  '#', c_red,     80, 200, 120, 0,
		"gasoline", "v2_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_variable_size),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "Inline-4 engine",     '*', c_ltred,  '#', c_red,     80, 300, 300, 0,
		"gasoline", "i4_combustion", 3,
        mfb(vpf_internal) | mfb(vpf_variable_size),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "V6 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0,
		"gasoline", "v6_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_variable_size),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "V8 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0,
		"gasoline", "v8_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_variable_size),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "electric motor",             '*', c_yellow,  '#', c_red,    80, 200, 70, 0,
		"battery", "motor", 3,
        mfb(vpf_internal),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "large electric motor",       '*', c_yellow,  '#', c_red,    80, 400, 350, 0,
		"battery", "motor_large", 4,
        mfb(vpf_internal),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "plasma engine",              '*', c_ltblue,  '#', c_red,    80, 250, 400, 0,
		"plasma", "plasma_engine", 6,
        mfb(vpf_internal),
		mfb(vpc_engine)
    ));
    vpart_list.push_back(vpart_info(
        "Foot pedals",                '*', c_ltgray,  '#', c_red,     50, 50, 70, 0,
		"muscle", "foot_crank", 1,
        mfb(vpf_internal),
		mfb(vpc_engine)
    ));
//                                                                         capacity type
    vpart_list.push_back(vpart_info(
        "gasoline tank",              'O', c_ltred,  '#', c_red,     80, 150, 3000, 0,
		"gasoline", "metal_tank", 1,
        mfb(vpf_internal),
		mfb(vpc_fuel_tank)
    ));
    vpart_list.push_back(vpart_info(
        "storage battery",            'O', c_yellow,  '#', c_red,    80, 300, 100000, 0,
		"battery", "storage_battery", 2,
        mfb(vpf_internal),
		mfb(vpc_fuel_tank)
    ));
    vpart_list.push_back(vpart_info(
        "minireactor",                'O', c_ltgreen,  '#', c_red,    80, 700, 10000, 0,
		"plutonium", "minireactor", 7,
        mfb(vpf_internal),
		mfb(vpc_fuel_tank)
    ));
    vpart_list.push_back(vpart_info(
        "hydrogen tank",             'O', c_ltblue,  '#', c_red,     80, 150, 3000, 0,
		"plasma", "metal_tank", 1,
        mfb(vpf_internal),
		mfb(vpc_fuel_tank)
    ));
    vpart_list.push_back(vpart_info(
        "water tank",                 'O', c_ltcyan,  '#', c_red,     80, 150, 400, 0,
		"water", "metal_tank", 1,
        mfb(vpf_internal),
		mfb(vpc_fuel_tank)
    ));
    vpart_list.push_back(vpart_info(
        "trunk",                      'H', c_brown,  '#', c_brown,    80, 300, 400, 0,
		"NULL", "frame", 1,
        0,
		mfb(vpc_over) | mfb(vpc_cargo)
    ));
    vpart_list.push_back(vpart_info(
        "box",                        'o', c_brown,  '#', c_brown,    60, 100, 400, 0,
		"NULL", "frame", 1,
        mfb(vpf_boardable),
		mfb(vpc_over) | mfb(vpc_cargo)
    ));

    vpart_list.push_back(vpart_info(
        "controls",   '$', c_ltgray,  '$', c_red,     10, 250, 0, 0, "NULL",
		"vehicle_controls", 3,
        mfb(vpf_internal),
		mfb(vpc_controls)
    ));
//                                                          bonus
    vpart_list.push_back(vpart_info(
        "muffler",    '/', c_ltgray,  '/', c_ltgray,  10, 150, 40, 0, "NULL",
		"muffler", 2,
        mfb(vpf_internal),
		mfb(vpc_muffler)
    ));
    vpart_list.push_back(vpart_info(
        "seatbelt",   ',', c_ltgray,  ',', c_red,     10, 200, 25, 0, "NULL",
		"rope_6", 1,
        mfb(vpf_internal),
		mfb(vpc_seatbelt)
    ));
    vpart_list.push_back(vpart_info(
        "solar panel", '#', c_yellow,  'x', c_yellow, 10, 20, 30, 0, "NULL",
		"solar_panel", 6,
        0,
		mfb(vpc_over) | mfb(vpc_solar_panel)
    ));
    vpart_list.push_back(vpart_info(
        "kitchen unit", '&', c_ltcyan, 'x', c_ltcyan, 10, 20, 0, 0, "NULL",
		"kitchen_unit", 4,
        mfb(vpf_no_reinforce) | mfb(vpf_obstacle),
		mfb(vpc_over) | mfb(vpc_cargo) | mfb(vpc_roof) | mfb(vpc_kitchen) | mfb(vpc_examine)
    ));
    vpart_list.push_back(vpart_info(
        "mounted M249",         't', c_cyan,    '#', c_cyan,    80, 400, 0, 0,
		"223", "m249", 6,
		0,
        mfb(vpc_over) | mfb(vpc_turret) | mfb(vpc_cargo)
    ));
    vpart_list.push_back(vpart_info(
        "mounted flamethrower", 't', c_dkgray,  '#', c_dkgray,  80, 400, 0, 0,
		"gasoline", "flamethrower", 7,
		0,
        mfb(vpc_over) | mfb(vpc_turret)
    ));
    vpart_list.push_back(vpart_info(
        "mounted plasma gun", 't', c_ltblue,    '#', c_ltblue,    80, 400, 0, 0,
		"plasma", "plasma_rifle", 7,
		0,
        mfb(vpc_over) | mfb(vpc_turret)
    ));

    vpart_list.push_back(vpart_info(
        "steel plating",     ')', c_ltcyan, ')', c_ltcyan, 100, 1000, 0, 0,
		"NULL", "steel_plate", 3,
        mfb(vpf_internal),
		mfb(vpc_armor)
    ));
    vpart_list.push_back(vpart_info(
        "superalloy plating",')', c_dkgray, ')', c_dkgray, 100, 900, 0, 0,
		"NULL", "alloy_plate", 4,
        mfb(vpf_internal),
		mfb(vpc_armor)
    ));
    vpart_list.push_back(vpart_info(
        "spiked plating",    ')', c_red,    ')', c_red,    150, 900, 0, 0,
		"NULL", "spiked_plate", 3,
        mfb(vpf_internal) | mfb(vpf_sharp),
		mfb(vpc_armor)
    ));
    vpart_list.push_back(vpart_info(
        "hard plating",      ')', c_cyan,   ')', c_cyan,   100, 2300, 0, 0,
		"NULL", "hard_plate", 4,
        mfb(vpf_internal),
		mfb(vpc_armor)
    ));
    vpart_list.push_back(vpart_info(
        "head light",        '*', c_white,  '*', c_white,  10, 20, 480, 0,
		"NULL", "flashlight", 1,
		mfb(vpf_internal),
		mfb(vpc_light)
    ));
    vpart_list.push_back(vpart_info(
        "mounted longbow", 't', c_brown,    '#', c_brown,    80, 400, 0, 0,
		"arrow", "longbow", 7,
		0,
        mfb(vpc_over) | mfb(vpc_turret) | mfb(vpc_cargo)
    ));
	
	// car horn, makes a loud noise
    vpart_list.push_back(vpart_info(
        "horn",        '*', c_blue,  '*', c_white,  10, 20, 480, 0,
		"NULL", "vehicle_horn", 1,
		mfb(vpf_internal),
		mfb(vpc_horn)
    ));
	
    vpart_list.push_back(vpart_info(
        "water collector",        'V', c_blue,  '*', c_white,  10, 20, 480, 0,
		"NULL", "water_collector", 1,
		mfb(vpf_internal),
		mfb(vpc_examine)
    ));
	
    vpart_list.push_back(vpart_info(
        "remote controll",        '*', c_green,  '*', c_white,  10, 20, 480, 0,
		"NULL", "vehicle_remote_controll", 1,
		mfb(vpf_internal),
		mfb(vpc_remote_controll)
    ));
	
	// remote control of car horn, allows you to remotly active the car horn
	
	return vpart_list;
}

int vpart_info::getNumberOfParts() {
	return get_vpart_list().size();
}

const vpart_info &vpart_info::getVehiclePartInfo(vpart_id index) {
	const vpart_info_vector &vparts = get_vpart_list();
	if(index <= 0 || index >= vparts.size()) {
		return vparts[0];
	}
	return vparts[index];
}

bool vpart_info::has_function(vpart_function function) const {
	return functions & mfb(function);
}

bool vpart_info::has_flag(vpart_flags flag) const {
	return flags & mfb(flag);
}

void add_item(const itype *type, const char *mat, const char *rep_item, double amount, vpart_info::type_count_pair_vector &result) {
	// If the item is made if mat, add rep_item to the list
	// of repair-items.
	// Add more if the m1 is the material, add less if m2 is the material
	const itype *rep_it = g->itypes[rep_item];
	if(rep_it == 0 || rep_it->name == "null") {
		return;
	}
	// Example: item of vehicle part is a steel frame,
	// damage is 20 % -> amount = <weight-of-steel-frame> * 0.2
	// Repair-item is steel_chunk, therefor
	// item_count = <weight-of-steel-frame> * 0.2 / <weight-of-chunk>
	// = 240 * 0.2 / 6 = 8
	int item_count = static_cast<int>(amount / rep_it->weight);
	if(type->m1 == mat && item_count >= 1) {
		result.push_back(vpart_info::type_count_pair(rep_item, item_count));
	} else if(type->m2 == mat && item_count >= 2) {
		result.push_back(vpart_info::type_count_pair(rep_item, item_count / 2));
	}
}

vpart_info::type_count_pair_vector vpart_info::get_repair_materials(int hp) const {
	type_count_pair_vector result;
	const int hp_to_repair = durability - hp;
	if(hp_to_repair < durability / 20) {
		// Less than 5% damage -> no further items needed
		return result;
	}
	if(hp <= 0) {
		result.push_back(vpart_info::type_count_pair(item, 1));
		return result;
	}
	const itype *type = g->itypes[item];
	if(type == 0 || type->id == "null") {
		return result;
	}
	// relative amount of damage
	const double rel_damage = static_cast<double>(hp_to_repair) / durability;
	// amount (as weight) of damage that must be repaired
	const double amount = rel_damage * type->weight;
	// material     item-type for repair   relative-damage
	::add_item(type, "steel", "steel_chunk", amount, result);
	if(result.empty()) {
		// Try again, this time use the smaller scraps, this is for
		// when the damage is to small to require a full chunk
		::add_item(type, "steel", "scrap", amount, result);
	}
	::add_item(type, "leather", "leather", amount, result);
	::add_item(type, "fur", "fur", amount, result);
	::add_item(type, "cotton", "rag", amount, result);
	::add_item(type, "iron", "scrap", amount, result);
	::add_item(type, "plastic", "plastic_chunk", amount, result);
	// for now:
	::add_item(type, "glass", "plastic_chunk", amount, result);
	return result;
}

bool vpart_info::examine(vehicle *veh, int part) const {
	vehicle_part &vp = veh->parts[part];
	if(has_function(vpc_kitchen)) {
		int wamount = veh->fuel_left("water");
		if(wamount == 0 && vp.items.empty()) {
			return false;
		}
		uimenu task_menu;
		task_menu.entries.push_back(uimenu_entry(1, (!vp.items.empty()) ? 1 : 0, -1, std::string("Get items from ") + name));
		task_menu.entries.push_back(uimenu_entry(2, wamount > 0 ? 1 : 0, -1, std::string("Have a drink")));
		task_menu.entries.push_back(uimenu_entry(3, wamount > 0 ? 1 : 0, -1, std::string("Fill water into a container")));
		task_menu.entries.push_back(uimenu_entry(4, 1, -1, "cancel"));
		task_menu.selected = 3;
		task_menu.query();
		if(task_menu.ret == 1)
		{
			// fall through as with normal cargo units
			return false;
		}
		else if(task_menu.ret == 2)
		{
			veh->drain("water", 1);
			typename ::item water(g->itypes["water_clean"], 0);
			g->u.eat(g, g->u.inv.add_item(water).invlet);
			g->u.moves -= 250;
			return true;
		}
		else if(task_menu.ret == 3)
		{
			typename ::item water(g->itypes["water_clean"], g->turn);
			water.charges = wamount;
			g->handle_liquid(water, false, false);
			if(water.charges != wamount)
			{
				veh->drain("water", wamount - water.charges);
				g->u.moves -= 100;
			}
			return true;
		}
		else if(task_menu.ret == 4)
		{
			return true;
		}
	}
	if(item == "water_collector") {
		vehicle_part &vp = veh->parts[part];
		if(vp.items.empty() || vp.items[0].charges == 0) {
			return false;
		}
		std::ostringstream buffer; buffer << "there are " << vp.items[0].charges << " units of " << vp.items[0].tname() << " here";
		uimenu menu;
		menu.text = buffer.str();
		menu.entries.push_back(uimenu_entry("empty tank"));
		menu.entries.push_back(uimenu_entry("fill into container"));
		menu.entries.push_back(uimenu_entry("cancel"));
		menu.query();
		if(menu.selected == 0) {
			vp.items.clear();
			return false;
		} else if(menu.selected == 1) {
			g->handle_liquid(vp.items[0], false, false);
			if(vp.items[0].charges == 0) {
				vp.items.clear();
			}
			return false;
		} else {
			return false;
		}
		return false;
	}
	return false;
}

void vpart_info::on_turn(vehicle *veh, int part) const {
	if(item == "water_collector") {
		vehicle_part &vp = veh->parts[part];
		int part_x = veh->global_x() + vp.precalc_dx[0];
		int part_y = veh->global_y() + vp.precalc_dy[0];
		if(g->m.has_flag_ter_or_furn(indoors, part_x, part_y)) {
			return;
		}
		itype_id type;
		int amount = 0;
		switch(g->weather) {
			case WEATHER_DRIZZLE:
				type = "water";
				amount = 1;
				break;
			case WEATHER_RAINY:
				type = "water";
				amount = 2;
				break;
			case WEATHER_THUNDER:
				type = "water";
				amount = 3;
				break;
			case WEATHER_LIGHTNING:
				type = "water";
				amount = 3;
				break;
			case WEATHER_ACID_DRIZZLE:
				type = "water_acid";
				amount = 1;
				break;
			case WEATHER_ACID_RAIN:
				type = "water_acid";
				amount = 2;
				break;
		}
		if(amount == 0) {
			return;
		}
		if(vp.items.empty()) {
			vp.items.push_back(typename ::item(g->itypes[type], 0));
		}
		if(vp.items[0].type->id != type) {
			if(type == "water_acid") {
				vp.items[0].make(g->itypes[type]);
			}
		}
		vp.items[0].charges = std::min(vp.items[0].charges + amount, 200);
	}
}

