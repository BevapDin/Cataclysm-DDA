#include "veh_type.h"

// following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
const vpart_info vpart_list[] =
{   // name        sym   color    sym_b   color_b  dmg  dur  par1 par2  item
    { "null part",  '?', c_red,     '?', c_red,     100, 100, 0, 0, "NULL", "null", 0,
        0 },
    { "seat",       '#', c_red,     '*', c_red,     60,  300, 0, 0, "NULL", "seat", 1,
        mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_boardable) | mfb(vpf_cargo) |
        mfb(vpf_no_reinforce) | mfb(vpf_anchor_point) },
    { "saddle",     '#', c_red,     '*', c_red,     20,  200, 0, 0, "NULL", "saddle", 1,
        mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_boardable) |
        mfb(vpf_no_reinforce) },
    { "bed",        '#', c_magenta, '*', c_magenta, 60,  300, 0, 0, "NULL", "seat", 1,
        mfb(vpf_over) | mfb(vpf_bed) | mfb(vpf_boardable) |
        mfb(vpf_cargo) | mfb(vpf_no_reinforce) },
    { "frame",      'h', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'j', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'c', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'y', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'u', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'n', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'b', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '=', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'H', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '^', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "handle",     '^', c_ltcyan,  '#', c_ltcyan,  100, 300, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "board",      'h', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'j', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'y', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'u', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'n', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'b', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "aisle",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb(vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) },
    { "aisle",       'H', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb (vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) },
    { "floor trunk",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb (vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) | mfb(vpf_cargo) },
    { "roof",       '#', c_ltgray,  '#', c_dkgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_internal) | mfb(vpf_roof) },
    { "door",       '+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_openable) | mfb(vpf_boardable) },
    { "opaque door",'+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_boardable) },
    { "internal door", '+', c_cyan, '&', c_cyan,    75,  75, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_roof) | mfb(vpf_no_reinforce) | mfb(vpf_boardable) },
    { "windshield", '"', c_ltcyan,  '0', c_ltgray,  70,  50, 0, 0, "NULL", "glass_sheet", 1,
        mfb(vpf_over) | mfb(vpf_obstacle) | mfb(vpf_no_reinforce) },
    { "blade",      '-', c_white,   'x', c_white,   250, 100, 0, 0, "NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "blade",      '|', c_white,   'x', c_white,   350, 100, 0, 0, "NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "spike",      '.', c_white,   'x', c_white,   300, 100, 0, 0, "NULL", "spike", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },

//                                                           wheel_width(inches)
    { "wheel",      '0',    c_dkgray,  'x', c_ltgray,  50,  200, 9, 0, "NULL", "wheel", 4,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "wide wheel", 'O',     c_dkgray,   'x', c_ltgray,  50,  400, 14, 0, "NULL", "wheel_wide", 5,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "bicycle wheel",'|',  c_dkgray, 'x', c_ltgray,  50,  40, 2, 0, "NULL", "wheel_bicycle", 1,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "motorbike wheel",'o',c_dkgray, 'x', c_ltgray,  50,  90, 4, 0, "NULL", "wheel_motorbike", 2,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "small wheel",    'o',c_dkgray, 'x', c_ltgray,  50,  70, 6, 0, "NULL", "wheel_small", 2,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
//
    { "1-cylinder engine",    '*', c_ltred,  '#', c_red,     80, 150, 40, 0, "gasoline", "1cyl_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V-twin engine",       '*', c_ltred,  '#', c_red,     80, 200, 120, 0, "gasoline", "v2_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "Inline-4 engine",     '*', c_ltred,  '#', c_red,     80, 300, 300, 0, "gasoline", "i4_combustion", 3,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V6 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0, "gasoline", "v6_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V8 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0, "gasoline", "v8_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "electric motor",             '*', c_yellow,  '#', c_red,    80, 200, 70, 0, "battery", "motor", 3,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "large electric motor",       '*', c_yellow,  '#', c_red,    80, 400, 350, 0, "battery", "motor_large", 4,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "plasma engine",              '*', c_ltblue,  '#', c_red,    80, 250, 400, 0, "plasma", "plasma_engine", 6,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "Foot pedals",                '*', c_ltgray,  '#', c_red,     50, 50, 70, 0, "muscle", "foot_crank", 1,
        mfb(vpf_internal) | mfb(vpf_engine) },
//                                                                         capacity type
    { "gasoline tank",              'O', c_ltred,  '#', c_red,     80, 150, 3000, 0, "gasoline", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "storage battery",            'O', c_yellow,  '#', c_red,    80, 300, 100000, 0, "battery", "storage_battery", 2,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "minireactor",                'O', c_ltgreen,  '#', c_red,    80, 700, 10000, 0, "plutonium", "minireactor", 7,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "hydrogen tank",             'O', c_ltblue,  '#', c_red,     80, 150, 3000, 0, "plasma", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "water tank",                 'O', c_ltcyan,  '#', c_red,     80, 150, 400, 0, "water", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "trunk",                      'H', c_brown,  '#', c_brown,    80, 300, 400, 0, "NULL", "frame", 1,
        mfb(vpf_over) | mfb(vpf_cargo) },
    { "box",                        'o', c_brown,  '#', c_brown,    60, 100, 400, 0, "NULL", "frame", 1,
        mfb(vpf_over) | mfb(vpf_cargo) | mfb(vpf_boardable) },

    { "controls",   '$', c_ltgray,  '$', c_red,     10, 250, 0, 0, "NULL", "vehicle_controls", 3,
        mfb(vpf_internal)  | mfb(vpf_controls) },
//                                                          bonus
    { "muffler",    '/', c_ltgray,  '/', c_ltgray,  10, 150, 40, 0, "NULL", "muffler", 2,
        mfb(vpf_internal)  | mfb(vpf_muffler) },
    { "seatbelt",   ',', c_ltgray,  ',', c_red,     10, 200, 25, 0, "NULL", "rope_6", 1,
        mfb(vpf_internal)  | mfb(vpf_seatbelt) },
    { "solar panel", '#', c_yellow,  'x', c_yellow, 10, 20, 30, 0, "NULL", "solar_panel", 6,
        mfb(vpf_over)  | mfb(vpf_solar_panel) },
    { "kitchen unit", '&', c_ltcyan, 'x', c_ltcyan, 10, 20, 0, 0, "NULL", "kitchen_unit", 4,
        mfb(vpf_over) | mfb(vpf_cargo) | mfb(vpf_roof) | mfb(vpf_no_reinforce) | mfb(vpf_obstacle) | mfb(vpf_kitchen) },
    { "mounted M249",         't', c_cyan,    '#', c_cyan,    80, 400, 0, 0, "223", "m249", 6,
        mfb(vpf_over)  | mfb(vpf_turret) | mfb(vpf_cargo) },
    { "mounted flamethrower", 't', c_dkgray,  '#', c_dkgray,  80, 400, 0, 0, "gasoline", "flamethrower", 7,
        mfb(vpf_over)  | mfb(vpf_turret) },
    { "mounted plasma gun", 't', c_ltblue,    '#', c_ltblue,    80, 400, 0, 0, "plasma", "plasma_rifle", 7,
        mfb(vpf_over)  | mfb(vpf_turret) },

    { "steel plating",     ')', c_ltcyan, ')', c_ltcyan, 100, 1000, 0, 0, "NULL", "steel_plate", 3,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "superalloy plating",')', c_dkgray, ')', c_dkgray, 100, 900, 0, 0, "NULL", "alloy_plate", 4,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "spiked plating",    ')', c_red,    ')', c_red,    150, 900, 0, 0, "NULL", "spiked_plate", 3,
        mfb(vpf_internal) | mfb(vpf_armor) | mfb(vpf_sharp) },
    { "hard plating",      ')', c_cyan,   ')', c_cyan,   100, 2300, 0, 0, "NULL", "hard_plate", 4,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "head light",        '*', c_white,  '*', c_white,  10, 20, 480, 0, "NULL", "flashlight", 1,
       mfb(vpf_internal) | mfb(vpf_light) },
    { "mounted longbow", 't', c_brown,    '#', c_brown,    80, 400, 0, 0, "arrow", "longbow", 7,
        mfb(vpf_over)  | mfb(vpf_turret) | mfb(vpf_cargo) }
};

int vpart_info::getNumberOfParts() {
	return sizeof(vpart_list) / sizeof(*vpart_list);
}

const vpart_info &vpart_info::getVehiclePartInfo(vpart_id index) {
	if(index <= 0 || index >= getNumberOfParts()) {
		return vpart_list[0];
	}
	return vpart_list[index];
}

bool vpart_info::isKitchen() const {
	return flags & mfb(vpf_kitchen);
}

bool vpart_info::isFireplace() const {
	return false;
}

bool vpart_info::isWheel() const {
	return flags & mfb(vpf_wheel);
}
