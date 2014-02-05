#include "vehicle.h"
#include "map.h"
#include "output.h"
#include "game.h"
#include "item.h"
#include "item_factory.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"
#include "catacharset.h"

#include "debug.h"

const ammotype fuel_types[num_fuel_types] = { "gasoline", "battery", "plutonium", "plasma", "water" };
/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const std::string fuel_type_gasoline("gasoline");
static const std::string fuel_type_battery("battery");
static const std::string fuel_type_plutonium("plutonium");
static const std::string fuel_type_plasma("plasma");
static const std::string fuel_type_water("water");
static const std::string fuel_type_muscle("muscle");
static const std::string part_location_structure("structure");

enum vehicle_controls {
 toggle_cruise_control,
 toggle_lights,
 toggle_overhead_lights,
 toggle_turrets,
 toggle_tracker,
 activate_horn,
 release_control,
 control_cancel,
 convert_vehicle,
 toggle_reactor,
 toggle_engine,
 toggle_fridge,
 toggle_recharger
};

vehicle_part2::vehicle_part2()
: id("null")
, iid(0)
, hp(0)
, blood(0)
, bigness(0)
, flags(0)
, passenger_id(0)
, amount(0)
, vp(NULL)
{
}

vparzu::vparzu()
: mount_dx(0)
, mount_dy(0)
, inside(false)
, veh(NULL)
{
    precalc_dx[0] = precalc_dx[1] = -1;
    precalc_dy[0] = precalc_dy[1] = -1;
}

vehicle::vehicle(std::string type_id, int init_veh_fuel, int init_veh_status): type(type_id)
{
    posx = 0;
    posy = 0;
    velocity = 0;
    turn_dir = 0;
    last_turn = 0;
    of_turn_carry = 0;
    turret_mode = 0;
    lights_epower = 0;
    overhead_epower = 0;
    fridge_epower = 0;
    recharger_epower = 0;
    tracking_epower = 0;
    cruise_velocity = 0;
    skidding = false;
    cruise_on = true;
    lights_on = false;
    tracking_on = false;
    overhead_lights_on = false;
    fridge_on = false;
    recharger_on = false;
    reactor_on = false;
    engine_on = false;
    has_pedals = false;

    //type can be null if the type_id parameter is omitted
    if(type != "null") {
      if(g->vtypes.count(type) > 0) {
        //If this template already exists, copy it
        *this = *(g->vtypes[type]);
        init_state(init_veh_fuel, init_veh_status);
      }
    }
    precalc_mounts(0, face.dir());
}

vehicle::~vehicle()
{
}

bool vehicle::player_in_control (player *p) const
{
    vparzu *veh_part;
    vehicle *veh = g->m.veh_at (p->posx, p->posy, veh_part);
    if (veh == NULL || veh != this)
        return false;
    return veh_part->has_part_with_feature(VPFLAG_CONTROLS, false) && p->controlling_vehicle;
}

void vehicle::load (std::ifstream &stin)
{
    getline(stin, type);

    if ( type.size() > 1 && ( type[0] == '{' || type[1] == '{' ) ) {
        std::stringstream derp;
        derp << type;
        JsonIn jsin(derp);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad vehicle json\n%s", jsonerr.c_str() );
        }
    } else {
        load_legacy(stin);
    }
    refresh(); // part index lists are lost on save??
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        a->second.add_missing_frame()
    }
}

void vparzu::add_missing_frame()
{
    for(size_t i = 0; i < parts.size(); i++) {
        if(parts[i].location() == part_location_structure) {
            return;
        }
    }
    //No frame here! Install one.
    vehicle_part new_part;
    new_part.vp = this;
    new_part.setid("frame_vertical");
    new_part.hp = vehicle_part_types["frame_vertical"].durability;
    vp.parts.insert(parts.begin(), new_part);
}

void vehicle::save (std::ofstream &stout)
{
    serialize(stout);
    stout << std::endl;
    return;
}

void vehicle::init_state(int init_veh_fuel, int init_veh_status)
{
    bool destroyEngine = false;
    bool destroyTires = false;
    bool blood_covered = false;
    bool blood_inside = false;

    std::map<std::string, item> consistent_bignesses;

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 1% to 7%
    int veh_fuel_mult = init_veh_fuel;
    if (init_veh_fuel == - 1) {
        veh_fuel_mult = rng (1,7);
    }
    if (init_veh_fuel > 100) {
        veh_fuel_mult = 100;
    }

    // im assuming vehicles only spawn in active maps
    levx = g->levx;
    levy = g->levy;

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamaged
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if (init_veh_status == 0) {
        veh_status = 0;
    }
    if (init_veh_status == 1) {
     veh_status = 1;
     if (one_in(2)) {  // either engine or tires are destroyed
      destroyEngine = true;
     } else {
      destroyTires = true;
     }
    }

    //Provide some variety to non-mint vehicles
    if(veh_status != 0) {

        //Leave engine running in some vehicles
        if(veh_fuel_mult > 0
                && all_parts_with_feature("ENGINE", true).size() > 0
                && one_in(8)) {
            engine_on = true;
        }

        //Turn on lights on some vehicles
        if(one_in(20)) {
            lights_on = true;
        }

        //Turn flasher/overhead lights on separately (more likely since these are rarer)
        if(one_in(4)) {
            overhead_lights_on = true;
        }

        if(one_in(10)) {
            blood_covered = true;
        }

        if(one_in(8)) {
            blood_inside = true;
        }

        //Fridge should always start out activated if present
        if(all_parts_with_feature("FRIDGE", false).size() > 0) {
            fridge_on = true;
        }

    }

    // Reactor should always start out activated if present
    ppvposvec fuel_tanks = all_parts_with_feature(VPFLAG_FUEL_TANK, true);
    for(int p = 0; p < fuel_tanks.size(); p++) {
        const vehicle_part2 &vp2 = fuel_tanks[p].first->parts[fuel_tanks[p].second];
        if(vp2.fuel_type() == fuel_type_plutonium) {
            reactor_on = true;
            break;
        }
    }

    bool blood_inside_set = false;
    int blood_inside_x = 0;
    int blood_inside_y = 0;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        for(size_t i = 0; i < vp.parts.size(); i++) {
            vehicle_part2 &pp = vp.parts[i];
            const vpart_info &vpi = pp.part_info();
            if (pp.has_flag("VARIABLE_SIZE")) { // generate its bigness attribute.?
                // generate an item for this type, & cache its bigness
                if (consistent_bignesses.count(pp.id) < 1) {
                    consistent_bignesses[pp.id] = item(itypes[vpi.item], 0);
                }
                const item &it = consistent_bignesses[pp.id];
                pp.get_part_properties_from_item(it);
            }
            if (pp.has_flag("FUEL_TANK")) {   // set fuel status
                pp.amount = vpi.size * veh_fuel_mult / 100;
            }
            if (pp.has_flag("OPENABLE")) {    // doors are closed
                if(!pp.open && one_in(4)) {
                    open(&vp);
                }
            }
            if (pp.has_flag("BOARDABLE")) {      // no passengers
                pp.remove_flag(vehicle_part::passenger_flag);
            }
            // initial vehicle damage
            if (veh_status == 0) {
                // Completely mint condition vehicle
                pp.hp = vpi.durability;
            } else {
                //a bit of initial damage :)
                //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
                int broken = 8, unhurt = 20;
                int roll = dice(4,8);
                if (roll < unhurt) {
                    if (roll <= broken) {
                        pp.hp= 0;
                    } else {
                        pp.hp = ((float)(roll-broken) / (unhurt-broken)) * vpi.durability;
                    }
                } else { // new.
                    pp.hp = vpi.durability;
                }
            }
            if (destroyEngine && pp.has_flag("ENGINE")) { // vehicle is disabled because engine is dead
                pp.hp = 0;
            }
            if (destroyTires && pp.has_flag("WHEEL")) { // vehicle is disabled because flat tires
                pp.hp = 0;
            }
            /* Bloodsplatter the front-end parts. Assume anything with x > 0 is
             * the "front" of the vehicle (since the driver's seat is at (0, 0).
             * We'll be generous with the blood, since some may disappear before
             * the player gets a chance to see the vehicle. */
            if (blood_covered && vp.mount_dx > 0) {
                if(one_in(3)) {
                    //Loads of blood. (200 = completely red vehicle part)
                    pp.blood = rng(200, 600);
                } else {
                    //Some blood
                    pp.blood = rng(50, 200);
                }
            }
            if(blood_inside) {
                // blood is splattered around (blood_inside_x, blood_inside_y),
                // coords relative to mount point; the center is always a seat
                if (blood_inside_set) {
                    int distSq = pow((blood_inside_x - vp.mount_dx), 2) + \
                                 pow((blood_inside_y - vp.mount_dy), 2);
                    if (distSq <= 1) {
                        pp.blood = rng(200, 400) - distSq*100;
                    }
                } else if (pp.has_flag("SEAT")) {
                    // Set the center of the bloody mess inside
                    blood_inside_x = vp.mount_dx;
                    blood_inside_y = vp.mount_dy;
                    blood_inside_set = true;
                }
            }
        }
    }
}
/**
 * Smashes up a vehicle that has already been placed; used for generating
 * very damaged vehicles. Additionally, any spot where two vehicles overlapped
 * (ie, any spot with multiple frames) will be completely destroyed, as that
 * was the collision point.
 */
void vehicle::smash() {
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        int structures_found = 0;
        for(size_t i = 0; i < vp.parts.size(); i++) {
            if (vp.parts[i].location() == part_location_structure) {
                structures_found++;
            }
        }
        if(structures_found > 1) {
            // Destroy everything in the square
            for(size_t i = 0; i < vp.parts.size(); i++) {
                vp.parts[i].hp = 0;
            }
            continue;
        }
        for(size_t i = 0; i < vp.parts.size(); i++) {
            vehicle_part2 &pp = vp.parts[i];
            if (pp.hp == 0) {
                //Skip any parts already mashed up
                continue;
            }
            const vpart_info &vpi = pp.part_info();
            //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
            int damage = (int) (dice(1, 12) * 0.1 * vpi.durability);
            pp.hp = std::max(0, pp.hp - damage);
        }
    }
}

void vehicle::use_controls()
{
    std::vector<vehicle_controls> options_choice;
    std::vector<uimenu_entry> options_message;
    // Always have this option


    // Let go without turning the engine off.
    if (g->u.controlling_vehicle &&
        g->m.veh_at(g->u.posx, g->u.posy) == this) {
        options_choice.push_back(release_control);
        options_message.push_back(uimenu_entry(_("Let go of controls"), 'l'));
    }


    bool has_lights = false;
    bool has_overhead_lights = false;
    bool has_horn = false;
    bool has_turrets = false;
    bool has_tracker = false;
    bool has_reactor = false;
    bool has_engine = false;
    bool has_fridge = false;
    bool has_recharger = false;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        for(size_t i = 0; i < vp.parts.size(); i++) {
            vehicle_part2 &pp = vp.parts[i];
            const vpart_info &vpi = pp.part_info();
            if (vpi.has_flag("CONE_LIGHT")) {
                has_lights = true;
            }
            if (vpi.has_flag("CIRCLE_LIGHT")) {
                has_overhead_lights = true;
            }
            if (vpi.has_flag("LIGHT")) {
                has_lights = true;
            }
            if (vpi.has_flag("TURRET")) {
                has_turrets = true;
            }
            if (vpi.has_flag("HORN")) {
                has_horn = true;
            }
            if (vpi.has_flag("TRACK")) {
                has_tracker = true;
            }
            if (vpi.has_flag(VPFLAG_FUEL_TANK) &&
                    vpi.fuel_type == fuel_type_plutonium) {
                has_reactor = true;
            }
            if (vpi.has_flag("ENGINE")) {
                has_engine = true;
            }
            if (vpi.has_flag("FRIDGE")) {
                has_fridge = true;
            }
            if (vpi.has_flag("RECHARGE")) {
                has_recharger = true;
            }
        }
    }

    // Toggle engine on/off, stop driving if we are driving.
    if (!pedals() && has_engine) {
        options_choice.push_back(toggle_engine);
        if (g->u.controlling_vehicle) {
            options_message.push_back(uimenu_entry(_("Stop driving."), 's'));
        } else {
            options_message.push_back(uimenu_entry((engine_on) ? _("Turn off the engine") :
                                                   _("Turn on the engine"), 'e'));
        }
    }

    options_choice.push_back(toggle_cruise_control);
    options_message.push_back(uimenu_entry((cruise_on) ? _("Disable cruise control") :
                                           _("Enable cruise control"), 'c'));


    // Lights if they are there - Note you can turn them on even when damaged, they just don't work
    if (has_lights) {
        options_choice.push_back(toggle_lights);
        options_message.push_back(uimenu_entry((lights_on) ? _("Turn off headlights") :
                                               _("Turn on headlights"), 'h'));
    }

   if (has_overhead_lights) {
       options_choice.push_back(toggle_overhead_lights);
       options_message.push_back(uimenu_entry(overhead_lights_on ? _("Turn off overhead lights") :
                                              _("Turn on overhead lights"), 'v'));
   }

    //Honk the horn!
    if (has_horn) {
        options_choice.push_back(activate_horn);
        options_message.push_back(uimenu_entry(_("Honk horn"), 'o'));
    }

    // Turrets: off or burst mode
    if (has_turrets) {
        options_choice.push_back(toggle_turrets);
        options_message.push_back(uimenu_entry((0 == turret_mode) ? _("Switch turrets to burst mode") :
                                               _("Disable turrets"), 't'));
    }

    // Turn the fridge on/off
    if (has_fridge) {
        options_choice.push_back(toggle_fridge);
        options_message.push_back(uimenu_entry(fridge_on ? _("Turn off fridge") :
                                               _("Turn on fridge"), 'f'));
    }

    // Turn the recharging station on/off
    if (has_recharger) {
        options_choice.push_back(toggle_recharger);
        options_message.push_back(uimenu_entry(recharger_on ? _("Turn off recharger") :
                                               _("Turn on recharger"), 'r'));
    }

    // Tracking on the overmap
    if (has_tracker) {
        options_choice.push_back(toggle_tracker);
        options_message.push_back(uimenu_entry((tracking_on) ? _("Disable tracking device") :
                                                _("Enable tracking device"), 'g'));

    }

    if( tags.count("convertible") ) {
        options_choice.push_back(convert_vehicle);
        options_message.push_back(uimenu_entry(_("Fold bicycle"), 'f'));
    }

    // Turn the reactor on/off
    if (has_reactor) {
        options_choice.push_back(toggle_reactor);
        options_message.push_back(uimenu_entry(reactor_on ? _("Turn off reactor") :
                                               _("Turn on reactor"), 'm'));
    }

    options_choice.push_back(control_cancel);
    options_message.push_back(uimenu_entry(_("Do nothing"), ' '));

    uimenu selectmenu;
    selectmenu.return_invalid = true;
    selectmenu.text = _("Vehicle controls");
    selectmenu.entries = options_message;
    selectmenu.query();
    int select = selectmenu.ret;

    if (select < 0) {
        return;
    }

    switch(options_choice[select]) {
    case toggle_cruise_control:
        cruise_on = !cruise_on;
        g->add_msg((cruise_on) ? _("Cruise control turned on") : _("Cruise control turned off"));
        break;
    case toggle_lights:
        if(lights_on || fuel_left(fuel_type_battery) ) {
            lights_on = !lights_on;
            g->add_msg((lights_on) ? _("Headlights turned on") : _("Headlights turned off"));
        } else {
            g->add_msg(_("The headlights won't come on!"));
        }
        break;
    case toggle_overhead_lights:
        if( !overhead_lights_on || fuel_left(fuel_type_battery) ) {
            overhead_lights_on = !overhead_lights_on;
            g->add_msg((overhead_lights_on) ? _("Overhead lights turned on") :
                       _("Overhead lights turned off"));
        } else {
            g->add_msg(_("The lights won't come on!"));
        }
        break;
    case activate_horn:
        g->add_msg(_("You honk the horn!"));
        honk_horn();
        break;
    case toggle_turrets:
        if (++turret_mode > 1) {
            turret_mode = 0;
        }
        g->add_msg((0 == turret_mode) ? _("Turrets: Disabled") : _("Turrets: Burst mode"));
        break;
    case toggle_fridge:
        if( !fridge_on || fuel_left(fuel_type_battery) ) {
            fridge_on = !fridge_on;
            g->add_msg((fridge_on) ? _("Fridge turned on") :
                       _("Fridge turned off"));
        } else {
            g->add_msg(_("The fridge won't turn on!"));
        }
        break;
    case toggle_recharger:
        if( !recharger_on || fuel_left(fuel_type_battery) ) {
            recharger_on = !recharger_on;
            g->add_msg((recharger_on) ? _("Recharger turned on") :
                       _("Recharger turned off"));
        } else {
            g->add_msg(_("The recharger won't turn on!"));
        }
        break;
    case toggle_reactor:
        if(!reactor_on || fuel_left(fuel_type_plutonium)) {
            reactor_on = !reactor_on;
            g->add_msg((reactor_on) ? _("Reactor turned on") :
                       _("Reactor turned off"));
        }
        else {
            g->add_msg(_("The reactor won't turn on!"));
        }
        break;
    case toggle_engine:
        if (g->u.controlling_vehicle) {
            if (engine_on) {
                engine_on = false;
                g->add_msg(_("You turn the engine off and let go of the controls."));
            } else {
                g->add_msg(_("You let go of the controls."));
            }
            g->u.controlling_vehicle = false;
            break;
        } else if (engine_on) {
            engine_on = false;
            g->add_msg(_("You turn the engine off."));
        } else {
          if (total_power (true) < 1) {
              if (total_power (false) < 1) {
                  g->add_msg (_("The %s doesn't have an engine!"), name.c_str());
              } else if(pedals()) {
                  g->add_msg (_("The %s's pedals are out of reach!"), name.c_str());
              } else {
                  g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
              }
          }
          else {
              start_engine();
          }
        }
        break;
    case release_control:
        g->u.controlling_vehicle = false;
        g->add_msg(_("You let go of the controls."));
        break;
    case convert_vehicle:
    {
        if(g->u.controlling_vehicle) {
            g->add_msg(_("As the pitiless metal bars close on your nether regions, you reconsider trying to fold the bicycle while riding it."));
            break;
        }
        g->add_msg(_("You painstakingly pack the bicycle into a portable configuration."));
        // create a folding bicycle item
        item bicycle;
        bicycle.make( itypes["folding_bicycle"] );

        // Drop stuff in containers on ground
        for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
            vparzu &vp = a->second;
            for(size_t i = 0; i < vp.parts.size(); i++) {
                vehicle_part2 &pp = vp.parts[i];
                for(size_t j = 0; i < pp.items.size(); j++) {
                    g->m.add_item_or_charges(g->u.posx, g->u.posy, pp.items[j]);
                }
                pp.items.clear();
            }
        }

        // Store data of all parts, iuse::unfold_bicyle only loads
        // some of them (like bigness), some are expect to be
        // vehicle specific and therefor constant (like id, mount_dx).
        // Writing everything here is easier to manage, as only
        // iuse::unfold_bicyle has to adopt to changes.
        try {
            std::ostringstream veh_data;
            JsonOut json(veh_data);
            // TODO [vehicles]
//            json.write(pmap);
            bicycle.item_vars["folding_bicycle_parts"] = veh_data.str();
        } catch(std::string e) {
            debugmsg("Error storing vehicle: %s", e.c_str());
        }

        g->m.add_item_or_charges(g->u.posx, g->u.posy, bicycle);
        // Remove vehicle
        unboard_all();
        g->m.destroy_vehicle(this);

        g->u.moves -= 500;
        break;
    }
    case toggle_tracker:
        if (tracking_on)
        {
            g->cur_om->remove_vehicle(om_id);
            tracking_on = false;
            g->add_msg(_("tracking device disabled"));
        } else if (fuel_left(fuel_type_battery))
        {
            om_id = g->cur_om->add_vehicle(this);
            tracking_on = true;
            g->add_msg(_("tracking device enabled"));
        } else {
            g->add_msg(_("tracking device won't turn on"));
        }
        break;
    case control_cancel:
        break;
    }
}

void vehicle::start_engine()
{
    bool muscle_powered = false;
    // TODO: Make chance of success based on engine condition.
    for(int p = 0; p < engines.size(); p++) {
        const vehicle_part2 &engine = *engines[p];
        if(engine.is_broken()) {
            continue;
        }
        if(pp.fuel_type() == fuel_type_gasoline) {
            int engine_power = pp.part_power();
            if(engine_power < 50) {
                // Small engines can be pull-started
                engine_on = true;
            } else {
                // Starter motor battery draw proportional to engine power
                if(!discharge_battery(engine_power / 10)) {
                    engine_on = true;
                }
            }
        } else if (pp.fuel_type() == fuel_type_muscle) {
            muscle_powered = true;
        } else {
            // Electric & plasma engines
            engine_on = true;
        }
    }

    if(engine_on) {
        g->add_msg(_("The %s's engine starts up."), name.c_str());
    } else if (!muscle_powered) {
        g->add_msg (_("The %s's engine fails to start."), name.c_str());
    }
}

point vehicle_part2::get_global_pos() const
{
    return point(vp->veh->global_x() + vp->precalc_dx[0],
                 vp->veh->global_y() + vp->precalc_dy[0]);
}

void vehicle::honk_horn()
{
    for(int h = 0; h < horns.size(); h++) {
        const vehicle_part2 &horn = *horns[h];
        if (horn.is_broken()) {
            continue;
        }
        const point p = horn.get_global_pos();
        //Determine sound
        const int bonus = horn.part_info().bonus;
        if( bonus >= 40 ) {
            g->sound( p.x, p.y, bonus, _("HOOOOORNK!") );
        } else if( bonus >= 20 ){
            g->sound( p.x, p.y, bonus, _("BEEEP!") );
        } else{
            g->sound( p.x, p.y, bonus, _("honk.") );
        }
    }
}

const vpart_info& vehicle_part2::part_info() const
{
    return vehicle_part_int_types[iid];
}

const ammotype &vehicle_part2::fuel_type() const
{
    return part_info().fuel_type;
}

// engines & alternators all have power.
// engines provide, whilst alternators consume.
int vehicle_part2::part_power() const {
    if(is_broken()) {
        return 0; //broken.
    }
    if (!has_flag(VPFLAG_ENGINE) && !has_flag(VPFLAG_ALTERNATOR)) {
        return 0; //not an engine.
    }
    if(has_flag(VPFLAG_VARIABLE_SIZE)) { // example: 2.42-L V-twin engine
        return bigness;
    } else { // example: foot crank
        return part_info().power;
    }
}

// alternators, solar panels, reactors, and accessories all have epower.
// alternators, solar panels, and reactors provide, whilst accessories consume.
int vehicle_part2::part_epower() const {
    if(is_broken()) {
        return 0; //broken.
    } else {
        return part_info().epower;
    }
}

int vehicle::epower_to_power (int epower) {
    // Convert epower units (watts) to power units
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    const int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    int power = epower / conversion_factor;
    // epower remainder results in chance at additional charge/discharge
    if (x_in_y(abs(epower % conversion_factor), conversion_factor)) {
        power += epower >= 0 ? 1 : -1;
    }
    return power;
}

int vehicle::power_to_epower (int power) {
    // Convert power units to epower units (watts)
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    const int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    return power * conversion_factor;
}

bool vehicle::has_structural_part(int dx, int dy) const
{
    const vparzu *vp = vp_at(dx, dy);
    if(vp == NULL) {
        return false;
    }
    for(size_t i = 0; i < vp->parts.size(); i++) {
        if(vp->parts[i].location() == part_location_structure &&
            !vp->parts[i].has_flag("PROTRUSION")) {
            return true;
        }
    }
    return false;
}

/**
 * Returns whether or not the vehicle part with the given id can be mounted in
 * the specified square.
 * @param dx The local x-coordinate to mount in.
 * @param dy The local y-coordinate to mount in.
 * @param id The id of the part to install.
 * @return true if the part can be mounted, false if not.
 */
bool vehicle::can_mount (int dx, int dy, std::string id) const
{
    //The part has to actually exist.
    if(vehicle_part_types.count(id) == 0) {
        return false;
    }

    //It also has to be a real part, not the null part
    const vpart_info &part = vehicle_part_types[id];
    if(part.has_flag("NOINSTALL")) {
        return false;
    }

    const vparzu *vp = vp_at(dx, dy);

    //First part in an empty square MUST be a structural part
    if(vp == NULL) {
        if(part.location != part_location_structure) {
            return false;
        }
        //new parts on an empty square must be installed next to an existing part
        if( !has_structural_part(dx+1, dy) &&
            !has_structural_part(dx, dy+1) &&
            !has_structural_part(dx-1, dy) &&
            !has_structural_part(dx, dy-1)
        ) {
            return false;
        }
        // no other check applies, they check for existing parts
        // but in this case there is no existing part here.
        return true;
    }
    // vp != NULL -> there is already a part here

    //No other part can be placed on a protrusion
    if(vp->has_part_with_flag("PROTRUSION")) {
        return false;
    }
    //No part type can stack with itself
    if(vp->part_of_type(id, false) != NULL) {
        return false;
    }
    // Until we have an interface for handling multiple components with CARGO space,
    // exclude them from being mounted in the same tile.
    if(part.has_flag("CARGO") && vp->has_part_with_flag("CARGO")) {
        return false;
    }

    //No part type can stack with itself, or any other part in the same slot
    for(size_t i = 0; i < vp->parts.size(); i++) {
        //Parts with no location can stack with each other (but not themselves)
        if(!part.location.empty() && part.location == vp->parts[i].location()) {
            return false;
        }
    }

    // Alternators must be installed on a gas engine
    if(part.has_flag(VPFLAG_ALTERNATOR)) {
        bool anchor_found = false;
        for(size_t i = 0; i < vp->parts.size(); i++) {
            if(vp->parts[i].has_flag(VPFLAG_ENGINE) &&
               vp->parts[i].fuel_type() == fuel_type_gasoline) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Seatbelts must be installed on a seat
    if(part.has_flag("SEATBELT") && !vp->has_part_with_flag("BELTABLE")) {
        return false;
    }

    //Internal must be installed into a cargo area.
    if(part.has_flag("INTERNAL") && !vp->has_part_with_flag("CARGO")) {
        return false;
    }

    // curtains must be installed on (reinforced)windshields
    // TODO: do this automatically using "location":"on_mountpoint"
    if (part.has_flag("CURTAIN") && !vp->has_part_with_flag("WINDOW")) {
        return false;
    }

    //Anything not explicitly denied is permitted
    return true;
}

bool vehicle::can_unmount(const vehicle_part2 &pp) const
{
    const vparzu &vp = *pp.vp;
    int dx = vp.mount_dx;
    int dy = vp.mount_dy;

    // Can't remove an engine if there's still an alternator there
    if(pp.has_flag(VPFLAG_ENGINE) && vp.has_part_with_flag(VPFLAG_ALTERNATOR)) {
        return false;
    }

    //Can't remove a seat if there's still a seatbelt there
    if(pp.has_flag("BELTABLE") && vp.has_part_with_flag("SEATBELT")) {
        return false;
    }

    // Can't remove a window with curtains still on it
    if(pp.has_flag("WINDOW") && vp.has_part_with_flag("CURTAIN")) {
        return false;
    }

    //Structural parts have extra requirements
    if(pp.location() == part_location_structure) {

        /* To remove a structural part, there can be only structural parts left
         * in that square (might be more than one in the case of wreckage) */
        for(int part_index = 0; part_index < vp.parts.size(); part_index++) {
            if(vp.parts[part_index].location() != part_location_structure) {
                return false;
            }
        }

        //If it's the last part in the square...
        if(vp.parts.size() == 1) {

            /* This is the tricky part: We can't remove a part that would cause
             * the vehicle to 'break into two' (like removing the middle section
             * of a quad bike, for instance). This basically requires doing some
             * breadth-first searches to ensure previously connected parts are
             * still connected. */

            //First, find all the squares connected to the one we're removing
            std::vector<const vparzu*> connected_parts;

            for(int i = 0; i < 4; i++) {
                int next_x = i < 2 ? (i == 0 ? -1 : 1) : 0;
                int next_y = i < 2 ? 0 : (i == 2 ? -1 : 1);
                const vparzu *vp2 = vp_at(dx + next_x, dy + next_y);
                //Ignore empty squares
                if(vp2 != NULL) {
                    //Just need one part from the square to track the x/y
                    connected_parts.push_back(vp2);
                }
            }

            /* If size = 0, it's the last part of the whole vehicle, so we're OK
             * If size = 1, it's one protruding part (ie, bicycle wheel), so OK
             * Otherwise, it gets complicated... */
            if(connected_parts.size() > 1) {

                /* We'll take connected_parts[0] to be the target part.
                 * Every other part must have some path (that doesn't involve
                 * the part about to be removed) to the target part, in order
                 * for the part to be legally removable. */
                for(int next_part = 1; next_part < connected_parts.size(); next_part++) {
                    if(!is_connected(connected_parts[0], connected_parts[next_part], &vp)) {
                        //Removing that part would break the vehicle in two
                        return false;
                    }
                }

            }

        }

    }

    //Anything not explicitly denied is permitted
    return true;
}

/**
 * Performs a breadth-first search from one part to another, to see if a path
 * exists between the two without going through the excluded part. Used to see
 * if a part can be legally removed.
 * @param to The part to reach.
 * @param from The part to start the search from.
 * @param excluded The part that is being removed and, therefore, should not
 *        be included in the path.
 * @return true if a path exists without the excluded part, false otherwise.
 */
bool vehicle::is_connected(const vparzu *to, const vparzu *from, const vparzu *excluded) const
{
    //Breadth-first-search components
    std::list<const vparzu*> discovered;
    const vparzu *current_part;
    std::list<const vparzu*> searched;

    //We begin with just the start point
    discovered.push_back(from);

    while(!discovered.empty()) {
        current_part = discovered.front();
        discovered.pop_front();
        int current_x = current_part->mount_dx;
        int current_y = current_part->mount_dy;

        for(int i = 0; i < 4; i++) {
            int next_x = current_x + (i < 2 ? (i == 0 ? -1 : 1) : 0);
            int next_y = current_y + (i < 2 ? 0 : (i == 2 ? -1 : 1));
            const vparzu *next_part = vp_at(next_x, next_y);
            if(next_part == NULL) {
                continue;
            } else if(next_part == to) {
                //Success!
                return true;
            } else if(next_part == excluded) {
                //There might be a path, but we're not allowed to go that way
                continue;
            }
            //Only add the part if we haven't been here before
            if(std::find(discovered.begin(), discovered.end(), next_part) == discovered.end()) {
                if(std::find(searched.begin(), searched.end(), next_part) == searched.end()) {
                    discovered.push_back(next_part);
                }
            }
        }

        //Now that that's done, we've finished exploring here
        searched.push_back(current_part);

    }

    //If we completely exhaust the discovered list, there's no path
    return false;
}

/**
 * Installs a part into this vehicle.
 * @param dx The x coordinate of where to install the part.
 * @param dy The y coordinate of where to install the part.
 * @param id The string ID of the part to install. (see vehicle_parts.json)
 * @param hp The starting HP of the part. If negative, default to max HP.
 * @param force true if the part should be installed even if not legal,
 *              false if illegal part installation should fail.
 * @return false if the part could not be installed, true otherwise.
 */
bool vehicle::install_part (int dx, int dy, std::string id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id)) {
        return false;  // no money -- no ski!
    }
    vehicle_part2 new_part;
    new_part.setid(id);
    new_part.hp = hp < 0 ? vehicle_part_types[id].durability : hp;
    new_part.amount = 0;
    new_part.blood = 0;
    item tmp(itypes[vehicle_part_types[id].item], 0);
    new_part.bigness = tmp.bigness;

    vparzu &vp = vpat(point(dx, dy));
    new_part.vp = &vp;
    vp.parts.push_back(new_part);

    refresh();
    return true;
}

// share damage & bigness betwixt veh_parts & items.
void vehicle_part2::get_part_properties_from_item(const item& i)
{
    assert(i.type->id == part_info().item);
    //transfer bigness if relevant.
    if(const_cast<itype*>(i.type)->is_var_veh_part()) {
        bigness = i.bigness;
    }
    // item damage is 0,1,2,3, or 4. part hp is 1..durability.
    // assuming it rusts. other item materials disentigrate at different rates...
    int health = 5 - i.damage;
    health *= part_info().durability; //[0,dur]
    health /= 5;
    hp = health;
}

void vehicle_part2::give_part_properties_to_item(item& i) const
{
    assert(i.type->id == part_info().item);
    //transfer bigness if relevant.
    if(const_cast<itype*>(i.type)->is_var_veh_part()) {
        i.bigness = bigness;
    }
    // remove charges if part is made of a tool
    if(const_cast<itype*>(i.type)->is_tool()) {
        i.charges = 0;
    }
    // translate part damage to item damage.
    // max damage is 4, min damage 0.
    // this is very lossy.
    int dam;
    float hpofdur = (float)hp / part_info().durability;
    dam = (1 - hpofdur) * 5;
    if (dam > 4) dam = 4;
    if (dam < 0) dam = 0;
    i.damage = dam;
}

void vehicle::remove_part(const vehicle_part2 &pp)
{
    vparzu &vp = *pp.vp;
    const bool was_tracker = pp.has_flag("TRACK");
    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if(pp.has_flag("WINDOW")) {
        while((vehicle_part2 *po = vp.part_with_flag("CURTAIN")) != NULL) {
            remove_part(*po);
        }
    }
    //Ditto for seatbelts
    if(pp.has_flag("SEAT")) {
        while((vehicle_part2 *po = vp.part_with_flag("SEATBELT")) != NULL) {
            remove_part(*po);
        }
    }

    //If we remove the (0, 0) frame, we need to shift things around
    if(vp.parts.size() == 1 && vp.mount_dx == 0 && vp.mount_dy == 0) {
        //Find a frame, any frame, to shift to
        for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
            if(vp == &a->second) {
                continue;
            }
            shift_parts(a->second.mount_dx, a->second.mount_dy);
            break;
        }
    }

    const int index = &pp - &vp.parts.front();
    vp.parts.erase(vp.parts.begin() + index);
    if(vp.parts.empty()) {
        vparmap::iterator a = pmap.find(point(vp.mount_dx, vp.mount_dy));
        if(a != pmap.end()) {
            pmap.erase(a);
        }
    }
    refresh();

    if (was_tracker && tracking_on) {
        // disable tracking if there are no other trackers installed.
        bool has_tracker = false;
        for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
            if(vp.has_part_with_feature("TRACK", true)) {
                has_tracker = true;
                break;
            }
        }
        if (!has_tracker){ // disable tracking
            g->cur_om->remove_vehicle(om_id);
            tracking_on = false;
        }
    }

    if(pmap.empty()) {
        g->m.destroy_vehicle(this);
    } else {
        g->m.update_vehicle_cache(this, false);
    }
}

/**
 * Breaks the specified part into the pieces defined by its breaks_into entry.
 * @param x The map x-coordinate to place pieces at (give or take).
 * @param y The map y-coordinate to place pieces at (give or take).
 * @param scatter If true, pieces are scattered near the target square.
 */
void vehicle_part2::break_part_into_pieces(int x, int y, bool scatter) const
{
    std::vector<break_entry> break_info = part_info().breaks_into;
    for(int index = 0; index < break_info.size(); index++) {
        int quantity = rng(break_info[index].min, break_info[index].max);
        for(int num = 0; num < quantity; num++) {
            const int actual_x = scatter ? x + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : x;
            const int actual_y = scatter ? y + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : y;
            item piece(itypes[break_info[index].item_id], g->turn);
            g->m.add_item_or_charges(actual_x, actual_y, piece);
        }
    }
}

item vehicle_part2::item_from_part() const
{
    itype_id itm = part_info().item;
    itype* parttype = itypes[itm];
    item tmp(parttype, g->turn);
    //transfer damage, etc.
    give_part_properties_to_item(tmp);
    return tmp;
}

vehicle_part2 *vparzu::part_with_feature(const vpart_bitflags &flag, bool unbroken) {
    for(size_t i = 0; i < parts.size(); i++) {
        vehicle_part2 &pp = parts[i];
        if (pp.has_feature(flag, unbroken)) {
            return &pp;
        }
    }
    return NULL;
}

const vehicle_part2 *vparzu::part_with_feature(const vpart_bitflags &flag, bool unbroken) const {
    for(size_t i = 0; i < parts.size(); i++) {
        const vehicle_part2 &pp = parts[i];
        if (pp.has_feature(flag, unbroken)) {
            return &pp;
        }
    }
    return NULL;
}

bool vehicle_part2::has_feature(const vpart_bitflags &flag, bool unbroken) const {
    return (part_info().has_flag(flag) && (!unbroken || hp > 0));
}

/**
 * Returns all parts in the vehicle with the given flag, optionally checking
 * to only return unbroken parts.
 * If performance becomes an issue, certain lists (such as wheels) could be
 * cached and fast-returned here, but this is currently linear-time with
 * respect to the number of parts in the vehicle.
 * @param feature The flag (such as "WHEEL" or "CONE_LIGHT") to find.
 * @param unbroken true if only unbroken parts should be returned, false to
 *        return all matching parts.
 */
vehicle::ppvposvec vehicle::all_parts_with_feature(const std::string& feature, bool unbroken)
{
    ppvposvec parts_found;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        for(int i = 0; i < vp.parts.size(); i++) {
            if(vp.parts[i].has_feature(feature, unbroken)) {
                parts_found.push_back(std::pair<vparzu*, int>(&vp, i));
            }
        }
    }
    return parts_found;
}

vehicle::ppvposvec vehicle::all_parts_with_feature(const vpart_bitflags & feature, bool unbroken)
{
    ppvposvec parts_found;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        for(int i = 0; i < vp.parts.size(); i++) {
            if(vp.parts[i].has_feature(feature, unbroken)) {
                parts_found.push_back(std::pair<vparzu*, int>(&vp, i));
            }
        }
    }
    return parts_found;
}

int vehicle::global_part_at(int x, int y)
{
 int dx = x - global_x();
 int dy = y - global_y();
 return part_at(dx,dy);
}

#if 0
/**
 * Given a vehicle part which is inside of this vehicle, returns the index of
 * that part. This exists solely because activities relating to vehicle editing
 * require the index of the vehicle part to be passed around.
 * @param part The part to find.
 * @return The part index, -1 if it is not part of this vehicle.
 */
int vehicle::index_of_part(vehicle_part *part)
{
  if(part != NULL) {
    for(int index = 0; index < parts.size(); index++) {
      vehicle_part next_part = parts[index];
      if(part->id == next_part.id &&
              part->mount_dx == next_part.mount_dx &&
              part->mount_dy == next_part.mount_dy &&
              part->hp == next_part.hp) {
        return index;
      }
    }
  }
  return NULL;
}
#endif

/**
 * Returns which part (as an index into the parts list) is the one that will be
 * displayed for the given square. Returns -1 if there are no parts in that
 * square.
 * @param local_x The local x-coordinate.
 * @param local_y The local y-coordinate.
 * @return The index of the part that will be displayed.
 */
const vehicle_part2 *vehicle::part_displayed_at(int local_x, int local_y) const
{
    const vparzu *vp = vp_at(local_x, local_y);
    if (vp == NULL) {
        return NULL;
    }
    return vp->part_displayed();
}

const vehicle_part2 *vparzu::part_displayed() const
{
    assert(!parts.empty());
    int top_part = 0;
    for(int index = 1; index < parts.size(); index++) {
        if(parts[top_part].part_info().z_order <
                parts[index].part_info().z_order) {
            top_part = index;
        }
    }
    return &(parts[top_part]);
}

vehicle_part2 *vparzu::part_displayed()
{
    return const_cast<vehicle_part2*>(part_displayed());
}

char vparzu::part_sym() const
{
    const vehicle_part2 *displayed_part = part_displayed();
    if (displayed_part == NULL) {
        return ' ';
    }
    if (displayed_part->has_feature(VPFLAG_OPENABLE, true) && displayed_part->open) {
        return '\''; // open door
    } else {
        return displayed_part->is_broken() ?
            displayed_part->part_info().sym_broken : displayed_part->part_info().sym;
    }
}

const std::string vehicle_part2::name() const
{
    return part_info().name;
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
std::string vparzu::part_id_string(char &part_mod) const
{
    part_mod = 0;

    const vehicle_part2 *displayed_part = part_displayed();
    const std::string &idinfo = displayed_part->id;

    if (displayed_part->has_flag(VPFLAG_OPENABLE) && displayed_part->open) {
        part_mod = 1; // open
    } else if (displayed_part->is_broken()){
        part_mod = 2; // broken
    }

    return idinfo;
}

nc_color vparzu::part_color() const
{
    nc_color col;

    const vehicle_part2 *parm = NULL;

    //If armoring is present and the option is set, it colors the visible part
    if (OPTIONS["VEHICLE_ARMOR_COLOR"] == true) {
        parm = part_with_flag(VPFLAG_ARMOR);
    }

    if (parm != NULL) {
        col = parm->part_info().color;
    } else {
        const vehicle_part2 *displayed_part = part_displayed();
        if (displayed_part == NULL) {
            return c_black;
        }
        if (displayed_part->blood > 200) {
            col = c_red;
        } else if (displayed_part->blood > 0) {
            col = c_ltred;
        } else if (displayed_part->is_broken()) {
            col = displayed_part->part_info().color_broken;
        } else {
            col = displayed_part->part_info().color;
        }
    }

    // curtains turn windshields gray
    const vehicle_part2 *curtains = part_with_feature(VPFLAG_CURTAIN, true);
    if (curtains != NULL) {
        if (part_with_feature(VPFLAG_WINDOW, true) != NULL && !curtains->open) {
            col = curtains->part_info().color;
        }
    }

    //Invert colors for cargo parts with stuff in them
    const vehicle_part2 *cargo_part = part_with_flag(VPFLAG_CARGO);
    if(cargo_part != NULL && !cargo_part->items.empty()) {
        return invert_color(col);
    } else {
        return col;
    }
}

/**
 * Prints a list of all parts to the screen inside of a boxed window, possibly
 * highlighting a selected one.
 * @param w The window to draw in.
 * @param y1 The y-coordinate to start drawing at.
 * @param width The width of the window.
 * @param p The index of the part being examined.
 * @param hl The index of the part to highlight (if any).
 */
int vehicle::print_part_desc(WINDOW *win, int y1, int width, const vparzu *p, int hl /*= -1*/)
{
    if (p == NULL) {
        return y1;
    }
    int y = y1;
    for(int i = 0; i < p->parts.size(); i++)
    {
        const vehicle_part2 &vp = p->parts[i];
        int dur = vp.part_info().durability;
        int per_cond = vp.hp * 100 / (dur < 1? 1 : dur);
        nc_color col_cond;
        if (vp.hp >= dur) {
            col_cond = c_green;
        } else if (per_cond >= 80) {
            col_cond = c_ltgreen;
        } else if (per_cond >= 50) {
            col_cond = c_yellow;
        } else if (per_cond >= 20) {
            col_cond = c_ltred;
        } else if (vp.hp > 0) {
            col_cond = c_red;
        } else { //Broken
            col_cond = c_dkgray;
        }

        std::string partname;
        // part bigness, if that's relevant.
        if (vp.has_flag("VARIABLE_SIZE") && vp.has_flag("ENGINE")) {
            //~ 2.8-Liter engine
            partname = string_format(_("%4.2f-Liter %s"),
                                     (float)(vp.bigness) / 100,
                                     vp.name.c_str());
        } else if (vp.has_flag("VARIABLE_SIZE") && vp.has_flag("WHEEL")) {
            //~ 14" wheel
            partname = string_format(_("%d\" %s"),
                                     vp.bigness,
                                     vp.name.c_str());
        } else {
            partname = vp.name;
        }

        bool armor = vp.has_flag("ARMOR");
        std::string left_sym, right_sym;
        if(armor) {
            left_sym = "("; right_sym = ")";
        } else if(vp.location() == part_location_structure) {
            left_sym = "["; right_sym = "]";
        } else {
            left_sym = "-"; right_sym = "-";
        }

        mvwprintz(win, y, 1, i == hl? hilite(c_ltgray) : c_ltgray, left_sym.c_str());
        mvwprintz(win, y, 2, i == hl? hilite(col_cond) : col_cond, partname.c_str());
        mvwprintz(win, y, 2 + utf8_width(partname.c_str()), i == hl? hilite(c_ltgray) : c_ltgray, right_sym.c_str());

        if (i == 0 && p->is_inside()) {
            //~ indicates that a vehicle part is inside
            mvwprintz(win, y, width-2-utf8_width(_("In")), c_ltgray, _("In"));
        } else if (i == 0) {
            //~ indicates that a vehicle part is outside
            mvwprintz(win, y, width-2-utf8_width(_("Out")), c_ltgray, _("Out"));
        }
        y++;
    }

    return y;
}

void vehicle::print_fuel_indicator (void *w, int y, int x, bool fullsize, bool verbose, bool desc)
{
    WINDOW *win = (WINDOW *) w;
    const nc_color fcs[num_fuel_types] = { c_ltred, c_yellow, c_ltgreen, c_ltblue, c_ltcyan };
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_ltgray;
    int yofs = 0;
    for(int i = 0; i < num_fuel_types; i++) {
        int cap = fuel_capacity(fuel_types[i]);
        if (cap > 0 && ( basic_consumption(fuel_types[i]) > 0 || fullsize ) ) {
            mvwprintz(win, y + yofs, x, col_indf1, "E...F");
            int amnt = cap > 0? fuel_left(fuel_types[i]) * 99 / cap : 0;
            int indf = (amnt / 20) % 5;
            mvwprintz(win, y + yofs, x + indf, fcs[i], "%c", fsyms[indf]);
            if (verbose) {
                if (g->debugmon) {
                    mvwprintz(win, y + yofs, x + 6, fcs[i], "%d/%d", fuel_left(fuel_types[i]), cap);
                } else {
                    mvwprintz(win, y + yofs, x + 6, fcs[i], "%d", (fuel_left(fuel_types[i]) * 100) / cap);
                    wprintz(win, c_ltgray, "%c", 045);
                }
            }
            if (desc) {
                wprintz(win, c_ltgray, " - %s", ammo_name(fuel_types[i]).c_str() );
            }
            if (fullsize) {
                yofs++;
            }
        }
    }
}

void vehicle::coord_translate (int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (face.dir());
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::coord_translate (int dir, int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (dir);
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::precalc_mounts (int idir, int dir)
{
    if (idir < 0 || idir > 1) {
        debugmsg("Invalid input to vehicle::precalc_mounts: idir = %d", idir);
        idir = 0;
    }
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        coord_translate(dir,
            vp.mount_dx, vp.mount_dy,
            vp.precalc_dx[idir],
            vp.precalc_dy[idir]);
    }
}

std::vector<vparzu*> vehicle::boarded_parts()
{
    std::vector<vparzu*> res;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        if (vp.has_passenger()) {
            res.push_back(&vp);
        }
    }
    return res;
}

const vparzu *vehicle::free_seat() const
{
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        const vehicle_part2 *pp = vp.part_with_feature(VPFLAG_BOARDABLE, true);
        if (pp != NULL && !pp->has_passenger()) {
            return &vp;
        }
    }
    return NULL;
}

player *vehicle_part2::get_passenger() const
{
    if (!has_flag(VPFLAG_BOARDABLE)) {
        return NULL;
    }
    if (!has_flag(vehicle_part::passenger_flag)) {
        return NULL;
    }
    if (passenger_id == g->u.getID()) {
        return &g->u;
    }
    int npcdex = g->npc_by_id(passenger_id);
    if (npcdex >= 0) {
        return g->active_npc[npcdex];
    }
    return NULL;
}

player *vparzu::get_passenger() const
{
    const vehicle_part2 *seat = part_with_flag(VPFLAG_BOARDABLE);
    if (seat == NULL) {
        return NULL;
    }
    return seat->get_passenger();
}

bool vparzu::has_passenger() const
{
    return get_passenger() != NULL;
}

int vehicle::global_x () const
{
    return smx * SEEX + posx;
}

int vehicle::global_y () const
{
    return smy * SEEY + posy;
}

int vehicle::omap_x() {
    return levx + (global_x() / SEEX);
}

int vehicle::omap_y() {
    return levy + (global_y() / SEEY);
}

void vehicle::update_map_x(int x) {
    levx = x;
    if (tracking_on)
        g->cur_om->vehicles[om_id].x = omap_x()/2;
}

void vehicle::update_map_y(int y) {
    levy = y;
    if (tracking_on)
        g->cur_om->vehicles[om_id].y = omap_y()/2;
}

int vehicle::total_mass() const
{
    int m = 0;
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        m += a->second.total_mass();
    }
    return m / 1000;
}

int vparzu::total_mass() const
{
    int m = 0;
    for(size_t i = 0; i < parts.size(); i++) {
        m += parts[i].total_mass();
    }
    return m;
}

int vehicle_part2::total_mass() const
{
    int m = itypes[part_info().item]->weight;
    for(int j = 0; j < items.size(); j++) {
        m += items[j].weight();
    }
    if (has_passenger()) {
        m += 81500; // TODO: get real weight
    }
    return m;
}

void vehicle::center_of_mass(int &x, int &y) const
{
    float xf = 0, yf = 0;
    const int m_total = total_mass();
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        const int m_part = vp.total_mass();
        xf += vp.precalc_dx[0] * m_part / 1000;
        yf += vp.precalc_dy[0] * m_part / 1000;
    }
    xf /= m_total;
    yf /= m_total;
    x = int(xf + 0.5); //round to nearest
    y = int(yf + 0.5);
}

int vehicle::fuel_left (const ammotype & ftype) const
{
    int fl = 0;
    for(size_t p = 0; p < fuel.size(); p++) {
        const vehicle_part2 &tank = *fuel[p];
        if(ftype == tank.fuel_type()) {
            fl += tank.amount;
        }
    }
    return fl;
}

int vehicle::fuel_capacity (const ammotype & ftype) const
{
    int cap = 0;
    for(size_t p = 0; p < fuel.size(); p++) {
        const vehicle_part2 &tank = *fuel[p];
        if(ftype == tank.fuel_type()) {
            cap += tank.part_info().size;
        }
    }
    return cap;
}

int vehicle::refill (const ammotype & ftype, int amount)
{
    for(size_t p = 0; p < fuel.size() && amount > 0; p++) {
        vehicle_part2 &tank = *fuel[p];
        const int need = tank.part_info().size - tank.amount;
        if(ftype != tank.fuel_type() || tank.is_broken() || need > 0) {
            continue;
        }
        const int t = std::min(amount, need);
        amount -= t;
        tank.amount += t;
    }
    return amount;
}

int vehicle::drain (const ammotype & ftype, int amount) {
    int drained = 0;
    for(size_t p = 0; p < fuel.size() && amount > 0; p++) {
        vehicle_part2 &tank = *fuel[p];
        if(ftype != tank.fuel_type() || tank.amount <= 0) {
            continue;
        }
        const int t = std::min(amount, tank.amount);
        drained += t;
        tank.amount -= t;
    }
    return drained;
}

int vehicle::basic_consumption (const ammotype & ftype) const
{
    int fcon = 0;
    for(int p = 0; p < engines.size(); p++) {
        vehicle_part2 &pp = *engines[p];
        if(ftype == pp.fuel_type() && pp.hp > 0) {
            if(pp.fuel_type() == fuel_type_battery) {
                // electric engine - use epower instead
                fcon += abs(epower_to_power(pp.part_epower()));
            }
            else {
                fcon += pp.part_power();
            }
        }
    }
    return fcon;
}

int vehicle::total_power (bool fueled) const
{
    int pwr = 0;
    int cnt = 0;
    bool player_controlling = player_in_control(&(g->u));
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        // player is here
        const bool pl_here = g->u.posx == vp.precalc_dx[0] && g->u.posy == vp.precalc_dx[1];
        for(size_t i = 0; i < vp.parts.size(); i++) {
            const vehicle_part2 &pp = vp.parts[i];
            const vpart_info &vpi = pp.part_info();
            if (pp.is_broken()) {
                continue;
            }
            if (vpi.has_flag(VPFLAG_ENGINE)) {
                if (!fueled ||
                    fuel_left(vpi.fuel_type) > 0 ||
                    (vpi.fuel_type == fuel_type_muscle && player_controlling && pl_here))
                {
                    pwr += pp.part_power();
                    cnt++;
                }
            }
            if (vpi.has_flag(VPFLAG_ALTERNATOR)) {
                pwr += pp.part_power(); // alternators have negative power
            }
        }
    }
    if (cnt > 1) {
        pwr = pwr * 4 / (4 + cnt -1);
    }
    return pwr;
}

int vehicle::solar_epower () const
{
    int epower = 0;
    const int nll = g->natural_light_level();
    for(int p = 0; p < solar_panels.size(); p++) {
        const vparzu &vp = *solar_panels[p].first;
        const vehicle_part2 pp = vp.parts[solar_panels[p].second];
        if (pp.is_broken()) {
            continue;
        }
        const int part_x = global_x() + vp.precalc_dx[0];
        const int part_y = global_y() + vp.precalc_dy[0];
        // Can't use g->in_sunlight() because it factors in vehicle roofs.
        if (!g->m.has_flag_ter_or_furn(TFLAG_INDOORS, part_x, part_y)) {
            epower += (pp.part_epower() * nll) / DAYLIGHT_LEVEL;
        }
    }
    return epower;
}

int vehicle::acceleration (bool fueled) const
{
    return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
}

int vehicle::max_velocity (bool fueled) const
{
    return total_power (fueled) * 80;
}

int vehicle::safe_velocity (bool fueled) const
{
    int pwrs = 0;
    int cnt = 0;
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        for(size_t i = 0; i < vp.parts.size(); i++) {
            const vehicle_part2 &pp = vp.parts[i];
            const vpart_info &vpi = pp.part_info();
            if (pp.is_broken()) {
                continue;
            }
            if (vpi.has_flag(VPFLAG_ENGINE)) {
                if (!fueled ||
                    fuel_left(vpi.fuel_type) > 0 ||
                    vpi.fuel_type == fuel_type_muscle)
                {
                    int m2c = 100;

                    if( vpi.fuel_type == fuel_type_gasoline )    m2c = 60;
                    else if( vpi.fuel_type == fuel_type_plasma ) m2c = 75;
                    else if( vpi.fuel_type == fuel_type_battery )   m2c = 90;
                    else if( vpi.fuel_type == fuel_type_muscle ) m2c = 45;

                    pwrs += pp.part_power() * m2c / 100;
                    cnt++;
                }
            }
            if (vpi.has_flag(VPFLAG_ALTERNATOR)) {
                pwrs += pp.part_power(); // alternators have negative power
            }
        }
    }
    if (cnt > 0) {
        pwrs = pwrs * 4 / (4 + cnt -1);
    }
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}

int vehicle::noise (bool fueled, bool gas_only) const
{
    int pwrs = 0;

    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        for(size_t i = 0; i < vp.parts.size(); i++) {
            const vehicle_part2 &pp = vp.parts[i];
            if (pp.has_feature("ENGINE", true) &&
                (!fueled || fuel_left(pp.fuel_type()) ||
                pp.fuel_type() == fuel_type_muscle))  {
            int nc = 10;

            if( pp.fuel_type() == fuel_type_gasoline ) {
                nc = 25;
            } else if( pp.fuel_type() == fuel_type_plasma ) {
                nc = 10;
            } else if( pp.fuel_type() == fuel_type_battery ) {
                nc = 3;
            } else if( pp.fuel_type() == fuel_type_muscle ) {
                nc = 5;
            }

            if (!gas_only || pp.fuel_type() == fuel_type_gasoline) {
                int pwr = pp.part_power() * nc / 100;
                if (muffle < 100 && (pp.fuel_type() == fuel_type_gasoline ||
                                     pp.fuel_type() == fuel_type_plasma)) {
                    pwr = pwr * muffle / 100;
                }
                // Only the loudest engine counts.
                pwrs = std::max(pwrs, pwr);
            }
            }
        }
    }
    return pwrs;
}

float vehicle::wheels_area (int *cnt) const
{
    int count = 0;
    int total_area = 0;
    const ppvposvec &wheel_indices = wheelcache;
    for(int i = 0; i < wheel_indices.size(); i++)
    {
        const vehicle_part2 &vp2 = wheel_indices[i].first->parts[wheel_indices[i].second];
        if (vp2.is_broken()) {
            continue;
        }
        int width = vp2.part_info().wheel_width;
        int bigness = vp2.bigness;
        // 9 inches, for reference, is about normal for cars.
        total_area += ((float)width/9) * bigness;
        count++;
    }
    if (cnt) {
        *cnt = count;
    }
    return total_area;
}

float vehicle::k_dynamics () const
{
    const int max_obst = 13;
    int obst[max_obst];
    for(int o = 0; o < max_obst; o++) {
        obst[o] = 0;
    }
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        int frame_size = vp.part_with_feature(VPFLAG_OBSTACLE, true) != NULL ? 30 : 10;
        int pos = vp.mount_dy + max_obst / 2;
        if (pos < 0) {
            pos = 0;
        }
        if (pos >= max_obst) {
            pos = max_obst -1;
        }
        if (obst[pos] < frame_size) {
            obst[pos] = frame_size;
        }
    }
    int frame_obst = 0;
    for(int o = 0; o < max_obst; o++) {
        frame_obst += obst[o];
    }
    float ae0 = 200.0;
    float fr0 = 1000.0;
    float wa = wheels_area();

    // calculate aerodynamic coefficient
    float ka = ae0 / (ae0 + frame_obst);

    // calculate safe speed reduction due to wheel friction
    float kf = fr0 / (fr0 + wa);

    return ka * kf;
}

float vehicle::k_mass () const
{
    float wa = wheels_area();
    if (wa <= 0)
       return 0;

    float ma0 = 50.0;

    // calculate safe speed reduction due to mass
    float km = ma0 / (ma0 + (total_mass()/8) / (8 * (float) wa));

    return km;
}

float vehicle::strain () const
{
    int mv = max_velocity(true);
    int sv = safe_velocity(true);
    if (mv <= sv)
        mv = sv + 1;
    if (velocity < safe_velocity(true))
        return 0;
    else
        return (float) (velocity - sv) / (float) (mv - sv);
}

int vparzu::part_mass() const {
    int m = 0;
    for(size_t i = 0; i < parts.size(); i++) {
        m += itypes[parts[i].part_info().item]->weight;
    }
    return m;
}

bool vehicle::valid_wheel_config () const
{
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int count = 0;
    const ppvposvec &wheel_indices = wheelcache;
    if(wheel_indices.size() == 0) {
        //No wheels!
        return false;
    } else if(wheel_indices.size() == 1) {
        //Has to be a stable wheel
        const vehicle_part2 &vp2 = wheel_indices[0].first->parts[wheel_indices[0].second];
        if(vp2.has_flag("STABLE") && vp2.hp > 0) {
            //Valid only if the vehicle is 1 square in size (1 structural part)
            return pmap.size() == 1;
        } else {
            return false;
        }
    }
    for(int i = 0; i < wheel_indices.size(); i++)
    {
        const vparzu &vp = *wheel_indices[i].first;
        const vehicle_part2 &vp2 = vp.parts[wheel_indices[i].second];
        if (vp2.is_broken()) {
            continue;
        }
        if (!count) {
            x1 = x2 = vp.mount_dx;
            y1 = y2 = vp.mount_dy;
        }
        if (vp.mount_dx < x1) {
            x1 = vp.mount_dx;
        }
        if (vp.mount_dx > x2) {
            x2 = vp.mount_dx;
        }
        if (vp.mount_dy < y1) {
            y1 = vp.mount_dy;
        }
        if (vp.mount_dy > y2) {
            y2 = vp.mount_dy;
        }
        count++;
    }
    float xo = 0, yo = 0;
    float wo = 0, w2;
    // lets find vehicle's center of masses
    for(vparmap::const_iterator a = pmap.begin(); a != pmap.end(); ++a) {
        const vparzu &vp = a->second;
        w2 = vp.part_mass();
        if (w2 < 1) {
            continue;
        }
        xo = xo * wo / (wo + w2) + vp.mount_dx * w2 / (wo + w2);
        yo = yo * wo / (wo + w2) + vp.mount_dy * w2 / (wo + w2);
        wo += w2;
    }
//    g->add_msg("cm x=%.3f y=%.3f m=%d  x1=%d y1=%d x2=%d y2=%d", xo, yo, (int) wo, x1, y1, x2, y2);
    if ((int)xo < x1 || (int)xo > x2 || (int)yo < y1 || (int)yo > y2) {
        return false; // center of masses not inside support of wheels (roughly)
    }
    return true;
}

void vehicle::consume_fuel (float rate = 1.0)
{
    ammotype ftypes[3] = { fuel_type_gasoline, fuel_type_battery, fuel_type_plasma };
    for(int ft = 0; ft < 3; ft++)
    {
        int base_amnt = basic_consumption(ftypes[ft]);
        if (!base_amnt)
            continue; // no consumption for engines of that type
        float st = strain() * 10;
        int amnt_precise = (int)(base_amnt * (1.0 + st * st) * rate);
        if (amnt_precise < 1) amnt_precise = 1;
        int amnt;
        if (ftypes[ft] == fuel_type_battery) {
            amnt = amnt_precise;
        }
        else {
            // engine not electric - divide consumption by 100
            amnt = amnt_precise / 100;
            // consumption remainder results in chance at additional fuel consumption
            if (x_in_y(amnt_precise % 100, 100)) {
                amnt += 1;
            }
        }
        if (amnt > 0) {
            drain(ftypes[ft], amnt);
        }
    }
}

//TODO: more categories of powered part!
void vehicle::power_parts ()
{
    int epower = 0;

    // Consumers of epower
    int gas_epower = 0;
    if(engine_on) {
        // Gas engines require epower to run for ignition system, ECU, etc.
        for(int p = 0; p < engines.size(); p++) {
            const vehicle_part2 &pp = *engines[p];
            if(pp.hp > 0 && pp.fuel_type() == fuel_type_gasoline) {
                gas_epower += pp.part_info().epower;
            }
        }
        epower += gas_epower;
    }

    if(lights_on) epower += lights_epower;
    if(overhead_lights_on) epower += overhead_epower;
    if(tracking_on) epower += tracking_epower;
    if(fridge_on) epower += fridge_epower;
    if(recharger_on) epower += recharger_epower;

    // Producers of epower
    epower += solar_epower();

    if(engine_on) {
        // Plasma engines generate epower if turned on
        int plasma_epower = 0;
        for(int p = 0; p < engines.size(); p++) {
            const vehicle_part2 &pp = *engines[p];
            if(pp.hp > 0 && pp.fuel_type() == fuel_type_plasma) {
                plasma_epower += pp.part_info().epower;
            }
        }
        epower += plasma_epower;
    }

    int battery_discharge = power_to_epower(fuel_capacity(fuel_type_battery) - fuel_left(fuel_type_battery));
    if(engine_on && (battery_discharge - epower > 0)) {
        // Not enough surplus epower to fully charge battery
        // Produce additional epower from any alternators
        int alternators_epower = 0;
        int alternators_power = 0;
        for(int p = 0; p < alternators.size(); p++) {
            const vehicle_part2 &pp = *alternators[p];
            if(pp.hp > 0) {
                alternators_epower += pp.part_info().epower;
                alternators_power += pp.part_power();
            }
        }
        if(alternators_epower > 0) {
            int alternator_output;
            if (battery_discharge - epower > alternators_epower) {
                alternator_output = alternators_epower;
            }
            else {
                alternator_output = battery_discharge - epower;
            }
            alternator_load = (float)alternator_output / (float)alternators_epower *
                (float)abs(alternators_power);
            epower += alternator_output;
        }
    }

    if(reactor_on && battery_discharge - epower > 0) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        int reactors_epower = 0;
        int reactors_fuel_epower = 0;
        for(int p = 0; p < reactors.size(); p++) {
            const vehicle_part2 &pp = *reactors[p];
            if(pp.hp > 0) {
                reactors_epower += pp.part_info().epower;
                reactors_fuel_epower += power_to_epower(pp.amount);
            }
        }
        // check if enough fuel for full reactor output
        if(reactors_fuel_epower < reactors_epower) {
            // partial reactor output
            reactors_epower = reactors_fuel_epower;
        }
        if(reactors_epower > 0) {
            int reactors_output;
            if (battery_discharge - epower > reactors_epower) {
                reactors_output = reactors_epower;
            }
            else {
                reactors_output = battery_discharge - epower;
            }
            // calculate battery-equivalent fuel consumption
            int battery_consumed = epower_to_power(reactors_output);
            // 1 plutonium == 100 battery - divide consumption by 100
            int plutonium_consumed = battery_consumed / 100;
            // battery remainder results in chance at additional plutonium consumption
            if (x_in_y(battery_consumed % 100, 100)) {
                plutonium_consumed += 1;
            }
            for(int p = 0; p < reactors.size() && plutonium_consumed > 0; p++) {
                vehicle_part2 &pp = *reactors[p];
                int avail_plutonium = pp.amount;
                if(avail_plutonium < plutonium_consumed) {
                    plutonium_consumed -= avail_plutonium;
                    pp.amount = 0;
                }
                else {
                    pp.amount -= plutonium_consumed;
                    plutonium_consumed = 0;
                }
            }
            epower += reactors_output;
        }
        else {
            // all reactors out of fuel or destroyed
            reactor_on = false;
            if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
                g->add_msg(_("The %s's reactor dies!"), name.c_str());
            }
        }
    }

    int battery_deficit = 0;
    if(epower > 0) {
        // store epower surplus in battery
        charge_battery(epower_to_power(epower));
    }
    else {
        // draw epower deficit from battery
        battery_deficit = discharge_battery(abs(epower_to_power(epower)));
    }

    if(battery_deficit) {
        lights_on = false;
        tracking_on = false;
        overhead_lights_on = false;
        fridge_on = false;
        recharger_on = false;
        if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
            g->add_msg("The %s's battery dies!",name.c_str());
        }
        if(gas_epower < 0) {
            // Not enough epower to run gas engine ignition system
            engine_on = false;
            if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
                g->add_msg("The %s's engine dies!",name.c_str());
            }
        }
    }
}

void vehicle::charge_battery (int amount)
{
    refill(fuel_type_battery, amount);
}

int vehicle::discharge_battery (int amount)
{
    return drain(fuel_type_battery, amount);
}

void vehicle::idle() {
    int engines_power = 0;
    float idle_rate;

    if (engine_on && total_power(true) > 0 && !pedals()) {
        int strn = (int)(strain() * strain() * 100);
        for(int p = 0; p < engines.size(); p++) {
            const vehicle_part2 &pp = *engines[p];
            if (fuel_left(pp.fuel_type()) && pp.hp > 0) {
                engines_power += pp.part_power();
                if (one_in(6) && rng(1, 100) < strn) {
                    int dmg = rng(strn * 2, strn * 4);
                    damage_direct(pp, p, dmg, 0);
                        if(one_in(2))
                            g->add_msg(_("Your engine emits a high pitched whine."));
                        else
                            g->add_msg(_("Your engine emits a loud grinding sound."));
                    }
                }
            }
        }

        idle_rate = (float)alternator_load / (float)engines_power;
        if (idle_rate < 0.01) idle_rate = 0.01; // minimum idle is 1% of full throttle
        consume_fuel(idle_rate);

        if (one_in(6)) {
            int sound = noise() / 10 + 2;
            g->sound(global_x(), global_y(), sound, "hummm.");

            if (one_in(10)) {
                int smk = noise(true, true); // Only generate smoke for gas cars.
                if (smk > 0 && !pedals()) {
                    int rdx = rng(0, 2);
                    int rdy = rng(0, 2);
                    g->m.add_field(global_x() + rdx, global_y() + rdy, fd_smoke, (sound / 50) + 1);
                }
            }
        }
    }
    else {
        if (g->u_see(global_x(), global_y()) && engine_on) {
            g->add_msg(_("The %s's engine dies!"), name.c_str());
        }
        engine_on = false;
    }
}

void vehicle::thrust (int thd) {
    if (velocity == 0)
    {
        turn_dir = face.dir();
        move = face;
        of_turn_carry = 0;
        last_turn = 0;
        skidding = false;
    }

    if (!thd)
        return;

    bool pl_ctrl = player_in_control(&g->u);

    if (!valid_wheel_config() && velocity == 0)
    {
        if (pl_ctrl)
            g->add_msg (_("The %s doesn't have enough wheels to move!"), name.c_str());
        return;
    }

    bool thrusting = true;
    if(velocity){ //brake?
       int sgn = velocity < 0? -1 : 1;
       thrusting = sgn == thd;
    }

    if (thrusting)
    {
        if (total_power () < 1)
        {
            if (pl_ctrl)
            {
              if (total_power (false) < 1) {
                  g->add_msg (_("The %s doesn't have an engine!"), name.c_str());
              } else if(pedals()) {
                  g->add_msg (_("The %s's pedals are out of reach!"), name.c_str());
              } else {
                  g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
              }
            }
            cruise_velocity = 0;
            return;
        }
        else if (!engine_on && !pedals()) {
          g->add_msg (_("The %s's engine isn't on!"), name.c_str());
          cruise_velocity = 0;
          return;
        } else if (pedals()) {
            if (g->u.has_bionic("bio_torsionratchet")
                && g->turn.get_turn() % 60 == 0) {
                g->u.charge_power(1);
            }
        }

        consume_fuel ();

        int strn = (int) (strain () * strain() * 100);

        for(int p = 0; p < parts.size(); p++)
        {
            if (part_flag(p, VPFLAG_ENGINE))
            {
                if(fuel_left(pp.fuel_type()) && parts[p].hp > 0 && rng (1, 100) < strn)
                {
                    int dmg = rng (strn * 2, strn * 4);
                    damage_direct (p, dmg, 0);
                    if(one_in(2))
                     g->add_msg(_("Your engine emits a high pitched whine."));
                    else
                     g->add_msg(_("Your engine emits a loud grinding sound."));
                }
            }
        }
        // add sound and smoke
        int smk = noise (true, true);
        if (smk > 0 && !pedals())
        {
            int rdx, rdy;
            coord_translate (exhaust_dx, exhaust_dy, rdx, rdy);
            g->m.add_field(global_x() + rdx, global_y() + rdy, fd_smoke, (smk / 50) + 1);
        }
        std::string soundmessage;
        if (!pedals()) {
          if (smk > 80)
            soundmessage = "ROARRR!";
          else if (smk > 60)
            soundmessage = "roarrr!";
          else if (smk > 30)
            soundmessage = "vroom!";
          else
            soundmessage = "whirrr!";
          g->sound(global_x(), global_y(), noise(), soundmessage.c_str());
        }
        else {
          g->sound(global_x(), global_y(), noise(), "");
        }
    }

    if (skidding)
        return;

    int accel = acceleration();
    int max_vel = max_velocity();
    int brake = 30 * k_mass();
    int brk = abs(velocity) * brake / 100;
    if (brk < accel)
        brk = accel;
    if (brk < 10 * 100)
        brk = 10 * 100;
    int vel_inc = (thrusting? accel : brk) * thd;
    if(thd == -1 && thrusting) // reverse accel.
       vel_inc = .6 * vel_inc;
    if ((velocity > 0 && velocity + vel_inc < 0) ||
        (velocity < 0 && velocity + vel_inc > 0))
        stop ();
    else
    {
        velocity += vel_inc;
        if (velocity > max_vel)
            velocity = max_vel;
        else
        if (velocity < -max_vel / 4)
            velocity = -max_vel / 4;
    }
}

void vehicle::cruise_thrust (int amount)
{
    if (!amount)
        return;
    int max_vel = (safe_velocity() * 11 / 10000 + 1) * 1000;
    cruise_velocity += amount;
    cruise_velocity = cruise_velocity / abs(amount) * abs(amount);
    if (cruise_velocity > max_vel)
        cruise_velocity = max_vel;
    else
    if (-cruise_velocity > max_vel / 4)
        cruise_velocity = -max_vel / 4;
}

void vehicle::turn (int deg)
{
    if (deg == 0)
        return;
    if (velocity < 0)
        deg = -deg;
    last_turn = deg;
    turn_dir += deg;
    if (turn_dir < 0)
        turn_dir += 360;
    if (turn_dir >= 360)
        turn_dir -= 360;
}

void vehicle::stop ()
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0;
    of_turn_carry = 0;
}

bool vehicle::collision( std::vector<veh_collision> &veh_veh_colls,
                         std::vector<veh_collision> &veh_misc_colls, int dx, int dy,
                         bool &can_move, int &imp, bool just_detect )
{
    const int gx = global_x();
    const int gy = global_y();
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        // coords of where part will go due to movement (dx/dy)
        // and turning (precalc_dx/dy [1])
        const int dsx = gx + dx + vp.precalc_dx[1];
        const int dsy = gy + dy + vp.precalc_dy[1];
        veh_collision coll = part_collision(vp, dsx, dsy, just_detect);
        if( coll.type != veh_coll_nothing && just_detect ) {
            return true;
        } else if( coll.type == veh_coll_veh ) {
            veh_veh_colls.push_back( coll );
        } else if( coll.type != veh_coll_nothing ) { //run over someone?
            veh_misc_colls.push_back(coll);
            if( can_move ) {
                imp += coll.imp;
            }
            if( velocity == 0 ) {
                can_move = false;
            }
        }
    }
    return false;
}

veh_collision vehicle::part_collision (vparzu &part, int x, int y, bool just_detect)
{
    bool pl_ctrl = player_in_control (&g->u);
    int mondex = g->mon_at(x, y);
    int npcind = g->npc_at(x, y);
    bool u_here = x == g->u.posx && y == g->u.posy && !g->u.in_vehicle;
    monster *z = mondex >= 0? &g->zombie(mondex) : NULL;
    player *ph = (npcind >= 0? g->active_npc[npcind] : (u_here? &g->u : 0));

    // if in a vehicle assume it's this one
    if (ph && ph->in_vehicle) {
        ph = 0;
    }

    vaprzu *target_part = NULL;
    vehicle *oveh = g->m.veh_at (x, y, target_part);
    bool is_veh_collision = oveh && (oveh->posx != posx || oveh->posy != posy);
    bool is_body_collision = ph || mondex >= 0;

    veh_coll_type collision_type = veh_coll_nothing;
    std::string obs_name = g->m.name(x, y).c_str();

    // vehicle collisions are a special case. just return the collision.
    // the map takes care of the dynamic stuff.
    if (is_veh_collision) {
       veh_collision ret;
       ret.type = veh_coll_veh;
       //"imp" is too simplistic for veh-veh collisions
       ret.part = &part;
       ret.target = oveh;
       ret.target_part = target_part;
       ret.target_name = oveh->name.c_str();
       return ret;
    }

    //Damage armor before damaging any other parts
    int parm = part_with_feature (part, VPFLAG_ARMOR);
    if (parm < 0) {
        parm = part;
    }
    int dmg_mod = part_info(parm).dmg_mod;
    // let's calculate type of collision & mass of object we hit
    float mass = total_mass();
    float mass2=0;
    float e= 0.3; // e = 0 -> plastic collision
    // e = 1 -> inelastic collision
    int part_dens = 0; //part density

    if (is_body_collision) {
        // then, check any monster/NPC/player on the way
        collision_type = veh_coll_body; // body
        e=0.30;
        part_dens = 15;
        if (z) {
            switch (z->type->size) {
            case MS_TINY:    // Rodent
                mass2 = 1;
                break;
            case MS_SMALL:   // Half human
                mass2 = 41;
                break;
            default:
            case MS_MEDIUM:  // Human
                mass2 = 82;
                break;
            case MS_LARGE:   // Cow
                mass2 = 120;
                break;
            case MS_HUGE:     // TAAAANK
                mass2 = 200;
                break;
            }
        } else {
            mass2 = 82;// player or NPC
        }
    } else if (g->m.has_flag_ter_or_furn ("THIN_OBSTACLE", x, y)) {
        // if all above fails, go for terrain which might obstruct moving
        collision_type = veh_coll_thin_obstacle; // some fence
        mass2 = 10;
        e=0.30;
        part_dens = 20;
    } else if (g->m.has_flag_ter_or_furn("BASHABLE", x, y)) {
        collision_type = veh_coll_bashable; // (door, window)
        mass2 = 50;
        e=0.30;
        part_dens = 20;
    } else if (g->m.move_cost_ter_furn(x, y) == 0) {
        if(g->m.is_destructable_ter_furn(x, y)) {
            collision_type = veh_coll_destructable; // destructible (wall)
            mass2 = 200;
            e=0.30;
            part_dens = 60;
        } else {
            collision_type = veh_coll_other; // not destructible
            mass2 = 1000;
            e=0.10;
            part_dens = 80;
        }
    }

    if (collision_type == veh_coll_nothing) {  // hit nothing
        veh_collision ret;
        ret.type = veh_coll_nothing;
        return ret;
    } else if( just_detect ) {
        veh_collision ret;
        ret.type = collision_type;
        return ret;
    }

    int degree = rng (70, 100);

    //Calculate damage resulting from d_E
    material_type* vpart_item_mat1 = material_type::find_material(itypes[part_info(parm).item]->m1);
    material_type* vpart_item_mat2 = material_type::find_material(itypes[part_info(parm).item]->m2);
    int vpart_dens;
    if(vpart_item_mat2->ident() == "null") {
        vpart_dens = vpart_item_mat1->density();
    } else {
        vpart_dens = (vpart_item_mat1->density() + vpart_item_mat2->density())/2; //average
    }

    //k=100 -> 100% damage on part
    //k=0 -> 100% damage on obj
    float material_factor = (part_dens - vpart_dens)*0.5;
    if ( material_factor >= 25) material_factor = 25; //saturation
    if ( material_factor < -25) material_factor = -25;
    float weight_factor;
    //factor = -25 if mass is much greater than mass2
    if ( mass >= mass2 ) weight_factor = -25 * ( log(mass) - log(mass2) ) / log(mass);
    //factor = +25 if mass2 is much greater than mass
    else weight_factor = 25 * ( log(mass2) - log(mass) ) / log(mass2) ;

    float k = 50 + material_factor + weight_factor;
    if(k > 90) k = 90;  //saturation
    if(k < 10) k = 10;

    bool smashed = true;
    std::string snd;
    float part_dmg = 0.0;
    float dmg = 0.0;
    //Calculate Impulse of car
    const float prev_velocity = velocity / 100;
    int turns_stunned = 0;

    do {
        //Impulse of object
        const float vel1 = velocity / 100;

        //Assumption: velocitiy of hit object = 0 mph
        const float vel2 = 0;
        //lost energy at collision -> deformation energy -> damage
        const float d_E = ((mass*mass2)*(1-e)*(1-e)*(vel1-vel2)*(vel1-vel2)) / (2*mass + 2*mass2);
        //velocity of car after collision
        const float vel1_a = (mass2*vel2*(1+e) + vel1*(mass - e*mass2)) / (mass + mass2);
        //velocity of object after collision
        const float vel2_a = (mass*vel1*(1+e) + vel2*(mass2 - e*mass)) / (mass + mass2);

        //Damage calculation
        //damage dealt overall
        dmg += abs(d_E / k_mvel);
        //damage for vehicle-part - only if not a hallucination
        if(z && !z->is_hallucination()) {
            part_dmg = dmg * k / 100;
        }
        //damage for object
        const float obj_dmg  = dmg * (100-k)/100;

        if (collision_type == veh_coll_bashable) {
            // something bashable -- use map::bash to determine outcome
            int absorb = -1;
            g->m.bash(x, y, obj_dmg, snd, &absorb);
            smashed = obj_dmg > absorb;
        } else if (collision_type >= veh_coll_thin_obstacle) {
            // some other terrain
            smashed = obj_dmg > mass2;
            if (smashed) {
                // destroy obstacle
                switch (collision_type) {
                case veh_coll_thin_obstacle:
                    if (g->m.has_furn(x, y)) {
                        g->m.furn_set(x, y, f_null);
                    } else {
                        g->m.ter_set(x, y, t_dirt);
                    }
                    break;
                case veh_coll_destructable:
                    g->m.destroy(x, y, false);
                    snd = _("crash!");
                    break;
                case veh_coll_other:
                    smashed = false;
                    break;
                default:;
                }
            }
        }
        if (collision_type == veh_coll_body) {
            int dam = obj_dmg*dmg_mod/100;
            if (z) {
                int z_armor = part_flag(part, "SHARP")? z->type->armor_cut : z->type->armor_bash;
                if (z_armor < 0) {
                    z_armor = 0;
                }
                dam -= z_armor;
            }
            if (dam < 0) { dam = 0; }

            //No blood from hallucinations
            if(z && !z->is_hallucination()) {
                if (part_flag(part, "SHARP")) {
                    parts[part].blood += (20 + dam) * 5;
                } else if (dam > rng (10, 30)) {
                    parts[part].blood += (10 + dam / 2) * 5;
                }
            }

            turns_stunned = rng (0, dam) > 10? rng (1, 2) + (dam > 40? rng (1, 2) : 0) : 0;
            if (part_flag(part, "SHARP")) {
                turns_stunned = 0;
            }
            if (turns_stunned > 6) {
                turns_stunned = 6;
            }
            if (turns_stunned > 0 && z) {
                z->add_effect("stunned", turns_stunned);
            }

            int angle = (100 - degree) * 2 * (one_in(2)? 1 : -1);
            if (z) {
                z->hurt(dam);

                if (vel2_a > rng (10, 20)) {
                    g->fling_player_or_monster (0, z, move.dir() + angle, vel2_a);
                }
                if (z->hp < 1 || z->is_hallucination()) {
                    g->kill_mon (mondex, pl_ctrl);
                }
            } else {
                ph->hitall (dam, 40);
                if (vel2_a > rng (10, 20)) {
                    g->fling_player_or_monster (ph, 0, move.dir() + angle, vel2_a);
                }
            }
        }

        velocity = vel1_a*100;

    } while( !smashed && velocity != 0 );

    // Apply special effects from collision.
    if (!is_body_collision) {
        if (pl_ctrl) {
            if (snd.length() > 0) {
                g->add_msg (_("Your %s's %s rams into a %s with a %s"), name.c_str(),
                            part_info(part).name.c_str(), obs_name.c_str(), snd.c_str());
            } else {
                g->add_msg (_("Your %s's %s rams into a %s."), name.c_str(),
                            part_info(part).name.c_str(), obs_name.c_str());
            }
        } else if (snd.length() > 0) {
            g->add_msg (_("You hear a %s"), snd.c_str());
        }
        g->sound(x, y, smashed? 80 : 50, "");
    } else {
        std::string dname;
        if (z) {
            dname = z->name().c_str();
        } else {
            dname = ph->name;
        }
        if (pl_ctrl) {
            g->add_msg (_("Your %s's %s rams into %s%s!"),
                        name.c_str(), part_info(part).name.c_str(), dname.c_str(),
                        turns_stunned > 0 && z? _(" and stuns it") : "");
        }

        if (part_flag(part, "SHARP")) {
            g->m.adjust_field_strength(point(x, y), fd_blood, 1 );
        } else {
            g->sound(x, y, 20, "");
        }
    }

    if( smashed ) {

        int turn_amount = rng (1, 3) * sqrt ((double)dmg);
        turn_amount /= 15;
        if (turn_amount < 1) {
            turn_amount = 1;
        }
        turn_amount *= 15;
        if (turn_amount > 120) {
            turn_amount = 120;
        }
        int turn_roll = rng (0, 100);
        //probability of skidding increases with higher delta_v
        if (turn_roll < abs(prev_velocity - (float)(velocity / 100)) * 2 ) {
            //delta_v = vel1 - vel1_a
            //delta_v = 50 mph -> 100% probability of skidding
            //delta_v = 25 mph -> 50% probability of skidding
            skidding = true;
            turn (one_in (2)? turn_amount : -turn_amount);
        }
    }
    damage (parm, part_dmg, 1);

    veh_collision ret;
    ret.part = &part;
    ret.type = collision_type;
    ret.imp = part_dmg;
    return ret;
}

void vehicle::handle_trap (int x, int y)
{
    int part = part_at(x,y);
    int pwh = part_with_feature (part, VPFLAG_WHEEL);
    if (pwh < 0) {
        return;
    }
    trap_id t = g->m.tr_at(x, y);
    if (t == tr_null) {
        return;
    }
    int noise = 0;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    bool wreckit = false;
    std::string msg (_("The %s's %s runs over %s."));
    std::string snd;
    // todo; make trapfuncv?

    if ( t == tr_bubblewrap ) {
        noise = 18;
        snd = _("Pop!");
    } else if ( t == tr_beartrap ||
                t == tr_beartrap_buried ) {
        noise = 8;
        snd = _("SNAP!");
        wreckit = true;
        g->m.remove_trap(x, y);
        g->m.spawn_item(x, y, "beartrap");
    } else if ( t == tr_nailboard ) {
        wreckit = true;
    } else if ( t == tr_blade ) {
        noise = 1;
        snd = _("Swinnng!");
        wreckit = true;
    } else if ( t == tr_crossbow ) {
        chance = 30;
        noise = 1;
        snd = _("Clank!");
        wreckit = true;
        g->m.remove_trap(x, y);
        g->m.spawn_item(x, y, "crossbow");
        g->m.spawn_item(x, y, "string_6");
        if (!one_in(10)) {
            g->m.spawn_item(x, y, "bolt_steel");
        }
    } else if ( t == tr_shotgun_2 ||
                t == tr_shotgun_1 ) {
        noise = 60;
        snd = _("Bang!");
        chance = 70;
        wreckit = true;
        if (t == tr_shotgun_2) {
            g->m.add_trap(x, y, tr_shotgun_1);
        } else {
            g->m.remove_trap(x, y);
            g->m.spawn_item(x, y, "shotgun_sawn");
            g->m.spawn_item(x, y, "string_6");
        }
    } else if ( t == tr_landmine_buried ||
                t == tr_landmine ) {
        expl = 10;
        shrap = 8;
        g->m.remove_trap(x, y);
    } else if ( t == tr_boobytrap ) {
        expl = 18;
        shrap = 12;
    } else if ( t == tr_dissector ) {
        noise = 10;
        snd = _("BRZZZAP!");
        wreckit = true;
    } else if ( t == tr_sinkhole ||
                t == tr_pit ||
                t == tr_spike_pit ||
                t == tr_ledge ) {
        wreckit = true;
    } else if ( t == tr_goo ||
                t == tr_portal ||
                t == tr_telepad ||
                t == tr_temple_flood ||
                t == tr_temple_toggle ) {
        msg.clear();
    }
    if (msg.size() > 0 && g->u_see(x, y)) {
        g->add_msg (msg.c_str(), name.c_str(), part_info(part).name.c_str(), g->traps[t]->name.c_str());
    }
    if (noise > 0) {
        g->sound(x, y, noise, snd);
    }
    if (wreckit && chance >= rng (1, 100)) {
        damage (part, 500);
    }
    if (expl > 0) {
        g->explosion(x, y, expl, shrap, false);
    }
}

// total volume of all the things
int vehicle_part2::stored_volume() const {
    if (!has_feature("CARGO")) {
        return 0;
    }
    int cur_volume = 0;
    for(int i = 0; i < items.size(); i++) {
        cur_volume += items[i].volume();
    }
    return cur_volume;
}

int vehicle_part2::max_volume() const {
    if (has_feature("CARGO")) {
        return part_info().size;
    }
    return 0;
}

// free space
int vehicle_part2::free_volume() const {
    return max_volume() - stored_volume();
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool vehicle_part2::is_full(const int addvolume, const int addnumber) const {
   const int maxitems = MAX_ITEM_IN_VEHICLE_STORAGE;
   const int maxvolume = max_volume();

   if ( addvolume == -1 ) {
       if ( items.size() < maxitems ) return true;
       int cur_volume=stored_volume(part);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( items.size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(part);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

bool vehicle_part2::add_item(const item &itm)
{
    const int max_storage = MAX_ITEM_IN_VEHICLE_STORAGE; // (game.h)
    const int maxvolume = max_volume();         // (game.h => vehicle::max_volume(part) ) in theory this could differ per vpart ( seat vs trunk )

    // const int max_weight = ?! // TODO: weight limit, calc per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certian point, or when hitting bumps etc

    if (!has_feature("CARGO")) {
        return false;
    }
    if (items.size() >= max_storage) {
        return false;
    }
    it_ammo *ammo = dynamic_cast<it_ammo*>(itm.type);
    if (has_feature("TURRET")) {
        if (ammo == NULL || (ammo->type != fuel_type() ||
                 ammo->type == fuel_type_gasoline ||
                 ammo->type == fuel_type_plasma)) {
            return false;
        }
    }
    int cur_volume = 0;
    int add_volume = itm.volume();
    bool tryaddcharges=(itm.charges  != -1 && (itm.is_food() || itm.is_ammo()));
    // iterate anyway since we need a volume total
      for(int i = 0; i < items.size(); i++) {
        cur_volume += items[i].volume();
        if( tryaddcharges && items[i].type->id == itm.type->id ) {
          items[i].charges+=itm.charges;
          return true;
        }
      }

    if ( cur_volume + add_volume > maxvolume ) {
        return false;
    }
    items.push_back(itm);
    return true;
}

void vehicle::remove_item(int itemdex)
{
    if (itemdex < 0 || itemdex >= items.size()) {
        return;
    }
    items.erase(items.begin() + itemdex);
}

void vehicle::place_spawn_items()
{
    for(std::vector<vehicle_item_spawn>::iterator next_spawn = item_spawns.begin();
            next_spawn != item_spawns.end(); next_spawn++) {
        if(rng(1, 100) <= next_spawn->chance) {
            //Find the cargo part in that square
            int part = part_at(next_spawn->x, next_spawn->y);
            part = part_with_feature(part, "CARGO", false);
            if(part < 0) {
                debugmsg("No CARGO parts at (%d, %d) of %s!",
                        next_spawn->x, next_spawn->y, name.c_str());
            } else {
                bool partbroken = ( parts[part].hp < 1 );
                int idmg = 0;
                for(std::vector<std::string>::iterator next_id = next_spawn->item_ids.begin();
                        next_id != next_spawn->item_ids.end(); next_id++) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    item new_item = item_controller->create(*next_id, g->turn);
                    new_item = new_item.in_its_container(&(itypes));
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
                }
                for(std::vector<std::string>::iterator next_group_id = next_spawn->item_groups.begin();
                        next_group_id != next_spawn->item_groups.end(); next_group_id++) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    Item_tag group_tag = item_controller->id_from(*next_group_id);
                    item new_item = item_controller->create(group_tag, g->turn);
                    new_item = new_item.in_its_container(&(itypes));
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
                }
            }
        }
    }
}

void vehicle::gain_moves()
{
    if (velocity) {
        of_turn = 1 + of_turn_carry;
    } else {
        of_turn = 0;
    }
    of_turn_carry = 0;

    // cruise control TODO: enable for NPC?
    if (player_in_control(&g->u) && cruise_on ) {
        if( velocity - cruise_velocity >= 10 * 100 ||
            cruise_velocity - velocity >= acceleration()/3 ||
            (cruise_velocity != 0 && velocity == 0) ||
            (cruise_velocity == 0 && velocity != 0)) {
            thrust (cruise_velocity > velocity? 1 : -1);
        }
    }

    // check for smoking parts
    for(int p = 0; p < parts.size(); p++)
    {
        int part_x = global_x() + parts[p].precalc_dx[0];
        int part_y = global_y() + parts[p].precalc_dy[0];

        /* Only lower blood level if:
         * - The part is outside.
         * - The weather is any effect that would cause the player to be wet. */
        if (parts[p].blood > 0 &&
                g->m.is_outside(part_x, part_y) && g->levz >= 0 &&
                g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN) {
            parts[p].blood--;
        }
        int p_eng = part_with_feature (p, VPFLAG_ENGINE, false);
        if (p_eng < 0 || parts[p_eng].hp > 0 || parts[p_eng].amount < 1) {
            continue;
        }
        parts[p_eng].amount--;
        for(int ix = -1; ix <= 1; ix++) {
            for(int iy = -1; iy <= 1; iy++) {
                if (!rng(0, 2)) {
                    g->m.add_field(part_x + ix, part_y + iy, fd_smoke, rng(2, 4));
                }
            }
        }
    }

    if (turret_mode) { // handle turrets
        for(int p = 0; p < parts.size(); p++) {
            fire_turret (p);
        }
    }
}

void vehicle::find_special_parts ()
{
    horns.clear();
    alternators.clear();
    fuel.clear();
    engines.clear();
    reactors.clear();
    solar_panels.clear();
    exhaust_dx = INT_MAX;
    has_pedals = false;
    muffle = 100;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        for(int i = 0; i < vp.parts.size(); i++) {
            vehicle_part2& vp2 = vp.parts[i];
            const vpart_info& vpi = vp2.part_info();
            if(vpi.has_flag("HORN")) {
                horns.push_back(&vp2);
            }
            if(vpi.has_flag(VPFLAG_ALTERNATOR)) {
                alternators.push_back(&vp2);
            }
            if(vpi.has_flag(VPFLAG_FUEL_TANK)) {
                fuel.push_back(&vp2);
            }
            if(vpi.has_flag(VPFLAG_ENGINE)) {
                engines.push_back(&vp2);
            }
            if(vpi.has_flag(VPFLAG_FUEL_TANK) &&
                vpi.fuel_type == fuel_type_plutonium) {
                reactors.push_back(&vp2);
            }
            if(v.has_flag(VPFLAG_SOLAR_PANEL)) {
                solar_panels.push_back(&vp2);
            }
            if(v.has_flag(VPFLAG_WHEEL)) {
                wheelcache.push_back(&vp2);
            }
            if(vpi.has_flag(VPFLAG_ENGINE)
                vpi.fuel_type == fuel_type_gasoline) {
                exhaust_dy = vp.mount_dy;
                exhaust_dx = vp.mount_dx;
            }
            if(vpi.has_feature("PEDALS")) {
                has_pedals = true;
            }
            if (vp.has_feature("MUFFLER", true)) {
                muffle = std::max(muffle, vpi.bonus);
            }
        }
    }
    if(exhaust_dy == INT_MAX) {
        exhaust_dx = 0;
        exhaust_dy = 0;
    } else {
        while(veh_at(exhaust_dx - 1, exhaust_dy) != NULL) {
            exhaust_dx--;
        }
        exhaust_dx--;
    }
}

void vehicle::find_power ()
{
    lights.clear();
    lights_epower = 0;
    overhead_epower = 0;
    tracking_epower = 0;
    fridge_epower = 0;
    recharger_epower = 0;
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        for(int i = 0; i < a->second.parts.size(); i++) {
            const vpart_info& vpi = a->second.parts[i].part_info();
            if (vpi.has_flag(VPFLAG_LIGHT) || vpi.has_flag(VPFLAG_CONE_LIGHT)) {
                lights.push_back(std::make_pair(&a->second, i));
                lights_epower += vpi.epower;
            }
            if (vpi.has_flag(VPFLAG_CIRCLE_LIGHT)) {
                overhead_epower += vpi.epower;
            }
            if (vpi.has_flag(VPFLAG_TRACK)) {
                tracking_epower += vpi.epower;
            }
            if (vpi.has_flag(VPFLAG_FRIDGE)) {
                fridge_epower += vpi.epower;
            }
            if (vpi.has_flag(VPFLAG_RECHARGE)) {
                recharger_epower += vpi.epower;
            }
        }
    }
}

bool vehicle::pedals() {
    return has_pedals;
}

/**
 * Refreshes all caches and refinds all parts, after the vehicle has had a part
 * added or removed.
 */
void vehicle::refresh()
{
    find_special_parts ();
    find_power ();
    precalc_mounts (0, face.dir());
    refresh_insides();
}

void vehicle::refresh_insides ()
{
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        /* If there's no roof, or there is a roof but it's broken, it's outside.
         * (Use short-circuiting && so broken frames don't screw this up) */
        if ( !vp.has_roof() ) {
            vp.inside = false;
            continue;
        }

        vp.inside = true; // inside if not otherwise
        for(int i = 0; i < 4; i++) { // let's check four neighbour parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            const vparzu *vpo = vp_at(vp.mount_dx + ndx, vp.mount_dy + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            if (!vpo->has_roof()) {
                vp.inside = false;
                break;
            }
        }
    }
}

bool vparzu::has_roof() const
{
    if (is_obstacle()) { // obstacle, like board or windshield or door
        return true;
    }
    if (has_part_with_feature("ROOF", true)) { // another roof -- cover
        return true;
    }
    return false;
}

bool vparzu::is_inside() const
{
    return inside;
}

void vehicle::unboard_all ()
{
    std::vector<vparzu*> bp = boarded_parts ();
    for(int i = 0; i < bp.size(); i++) {
        g->m.unboard_vehicle (global_x() + bp[i]->precalc_dx[0],
                              global_y() + bp[i]->precalc_dy[0]);
    }
}

int vehicle::damage (int p, int dmg, int type, bool aimed)
{
    if (dmg < 1) {
        return dmg;
    }

    std::vector<int> pl = parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
    if (!aimed)
    {
        bool found_obs = false;
        for(int i = 0; i < pl.size(); i++)
            if (part_flag (pl[i], "OBSTACLE") &&
                (!part_flag (pl[i], "OPENABLE") || !parts[pl[i]].open))
            {
                found_obs = true;
                break;
            }
        if (!found_obs) // not aimed at this tile and no obstacle here -- fly through
            return dmg;
    }
    int parm = part_with_feature (p, "ARMOR");
    int pdm = pl[rng (0, pl.size()-1)];
    int dres;
    if (parm < 0)
        // not covered by armor -- damage part
        dres = damage_direct (pdm, dmg, type);
    else
    {
        // covered by armor -- damage armor first
        // half damage for internal part(over parts not covered)
        bool overhead = part_flag(pdm, "ROOF") ||
                        part_info(pdm).location == "on_roof";
        // Calling damage_direct may remove the damaged part
        // completely, therefor the other indes (pdm) becames
        // wrong if pdm > parm.
        // Damaging the part with the higher index first is save,
        // as removing a part only changes indizes after the
        // removed part.
        if(parm < pdm) {
            damage_direct (pdm, overhead ? dmg : dmg / 2, type);
            dres = damage_direct (parm, dmg, type);
        } else {
            dres = damage_direct (parm, dmg, type);
            damage_direct (pdm, overhead ? dmg : dmg / 2, type);
        }
    }
    return dres;
}

void vehicle::damage_all (int dmg1, int dmg2, int type, const point &impact)
{
    if (dmg2 < dmg1) { std::swap(dmg1, dmg2); }
    if (dmg1 < 1) { return; }
    for(int p = 0; p < parts.size(); p++) {
        int distance = 1 + square_dist( parts[p].mount_dx, parts[p].mount_dy, impact.x, impact.y );
        if( distance > 1 && pp.location() == part_location_structure ) {
            damage_direct (p, rng( dmg1, dmg2 ) / (distance * distance), type);
        }
    }
}

/**
 * Shifts all parts of the vehicle by the given amounts, and then shifts the
 * vehicle itself in the opposite direction. The end result is that the vehicle
 * appears to have not moved. Useful for re-zeroing a vehicle to ensure that a
 * (0, 0) part is always present.
 * @param dx How much to shift on the x-axis.
 * @param dy How much to shift on the y-axis.
 */
void vehicle::shift_parts(const int dx, const int dy)
{
    const vparzu *origin = vp_at(0, 0);
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &vp = a->second;
        vp.mount_dx -= dx;
        vp.mount_dy -= dy;
    }

    posx += origin->precalc_dx[0];
    posy += origin->precalc_dy[0];

    refresh();

    //Need to also update the map after this
    g->m.reset_vehicle_cache();

}

void vehicle::tear_off_part(vparzu *vp, vehicle_part2 *pp) {
    assert(vp != NULL);
    assert(pp != NULL);
    assert(!vp->parts.empty());
    assert(&vp->parts.front() <= pp && pp <= &vp->parts.back());
    const int x_pos = global_x() + vp->precalc_dx[0];
    const int y_pos = global_y() + vp->precalc_dy[0];
    if(pp->location() == part_location_structure) {
        //Just break it off
        if(g->u_see(x_pos, y_pos)) {
            g->add_msg(_("The %s's %s is destroyed!"), name.c_str(), pp->name.c_str());
        }
        pp->break_part_into_pieces(x_pos, y_pos, true);
        remove_part(vp, pp);
        return;
    }
    //For structural parts, remove other parts first
    int pp_index = -1;
    for(int index = vp->parts.size() - 1; index >= 0; index--) {
        vehicle_part2 &op = vp->parts[index];
        //Ignore the frame being destroyed
        if(&op == pp) {
            pp_index = index;
        } else {
            if(g->u_see(x_pos, y_pos)) {
                g->add_msg(_("The %s's %s is torn off!"), name.c_str(), op.name.c_str());
            }
            g->m.add_item_or_charges(x_pos, y_pos, op.item_from_part(), true);
            remove_part(vp, &op);
            if(pp_index > index) {
                pp_index--; // index of pp has changed
            }
        }
    }
    // index of pp might have changed!
    pp = &vp->parts[pp_index];
    /* After clearing the frame, remove it if normally legal to do
     * so (it's not holding the vehicle together). At a later date,
     * some more complicated system (such as actually making two
     * vehicles from the split parts) would be ideal. */
    if(can_unmount(vp, pp)) {
        if(g->u_see(x_pos, y_pos)) {
            g->add_msg(_("The %s's %s is destroyed!"), name.c_str(), pp->name.c_str());
        }
        pp->break_part_into_pieces(x_pos, y_pos, true);
        remove_part(vp, pp);
    }
}

int vehicle::damage_direct (vehicle_part2 &pp, int dmg, int type)
{
    if (pp.is_broken()) {
        /* Already-destroyed part - chance it could be torn off into pieces.
         * Chance increases with damage, and decreases with part max durability
         * (so lights, etc are easily removed; frames and plating not so much) */
        if(rng(0, pp.part_info().durability / 10) < dmg) {
            tear_off_part(find_parzu(&pp), &pp);
        }
        return dmg;
    }

    int tsh = pp.part_info().durability / 10;
    if (tsh > 20) {
        tsh = 20;
    }
    int dres = dmg;
    if (dmg >= tsh || type != 1)
    {
        dres -= pp.hp;
        const int last_hp = pp.hp;
        pp.hp = std::max(0, pp.hp - dmg);
        if (!pp.hp && last_hp > 0) {
            refresh_insides();
        }
        if (pp.has_flag("FUEL_TANK"))
        {
            const ammotype &ft = pp.fuel_type();
            if (ft == fuel_type_gasoline || ft == fuel_type_plasma)
            {
                vparzu *vp = find_parzu(&pp);
                int pow = pp.amount / 40;
                if (pp.is_broken()) {
                    leak_fuel(vp, &pp);
                }
                if (type == 2 ||
                    (one_in (ft == fuel_type_gasoline ? 2 : 4) && pow > 5 && rng (75, 150) < dmg))
                {
                    g->u.add_memorial_log(pgettext("memorial_male","The fuel tank of the %s exploded!"),
                        pgettext("memorial_female", "The fuel tank of the %s exploded!"),
                        name.c_str());
                    g->explosion (global_x() + vp->precalc_dx[0], global_y() + vp->precalc_dy[0],
                                pow, 0, ft == fuel_type_gasoline);
                    pp.hp = 0;
                }
            }
        }
        else
        if (pp.is_broken() && part_flag(p, "UNMOUNT_ON_DAMAGE"))
        {
            g->m.spawn_item(global_x() + pp.precalc_dx[0],
                           global_y() + pp.precalc_dy[0],
                           pp.part_info().item, 1, 0, g->turn);
            remove_part (p);
        }
    }
    if (dres < 0)
        dres = 0;
    return dres;
}

void vehicle::leak_fuel (int p)
{
    if (!part_flag(p, "FUEL_TANK"))
        return;
    ammotype ft = pp.fuel_type();
    if (ft == fuel_type_gasoline)
    {
        int x = global_x();
        int y = global_y();
        for(int i = x - 2; i <= x + 2; i++)
            for(int j = y - 2; j <= y + 2; j++)
                if (g->m.move_cost(i, j) > 0 && one_in(2))
                {
                    if (parts[p].amount < 100)
                    {
                        parts[p].amount = 0;
                        return;
                    }
                    g->m.spawn_item(i, j, fuel_type_gasoline);
                    g->m.spawn_item(i, j, fuel_type_gasoline);
                    parts[p].amount -= 100;
                }
    }
    parts[p].amount = 0;
}

void vehicle::fire_turret (vparzu *p, bool burst)
{
    vehicle_part2 *p2 = p->part_with_feature("TURRET");
    if (p1 == NULL) {
        return;
    }
    it_gun *gun = dynamic_cast<it_gun*> (itypes[p2->part_info().item]);
    if (!gun) {
        return;
    }
    int charges = burst? gun->burst : 1;
    std::string whoosh = "";
    if (!charges)
        charges = 1;
    ammotype amt = p2->fuel_type();
    if (amt == fuel_type_gasoline || amt == fuel_type_plasma || amt == fuel_type_battery)
    {
        if (amt == fuel_type_gasoline) {
            charges = 20; // hacky
        } else if (amt == fuel_type_battery) {
            if (one_in(100)) {
                //~ the sound of a charge-rifle firing a massive ball of plasma
                whoosh = _("whoosh!");
                charges = rng(5,8); // kaboom
            } else {
                charges = rng(1,4);
            }
        }
        int fleft = fuel_left (amt);
        if (fleft < 1) {
            return;
        }
        it_ammo *ammo = dynamic_cast<it_ammo*>(itypes[amt]);
        if (!ammo) {
            return;
        }
        if (fire_turret_internal (p, p2, *gun, *ammo, charges, whoosh)) {
            // consume fuel
            if (amt == fuel_type_plasma) {
                charges *= 10; // hacky, too
            } else if (amt == fuel_type_battery) {
                charges *= charges * 5;
            }
            drain(amt, charges);
        }
    } else if (!p2->items.empty()) {
        std::vector<item> &items = p2->items;
        it_ammo *ammo = dynamic_cast<it_ammo*> (items[0].type);
        if (!ammo || ammo->type != amt || items[0].charges < 1) {
            return;
        }
        if (charges > items[0].charges) {
            charges = items[0].charges;
        }
        if (fire_turret_internal (p, p2, *gun, *ammo, charges)) {
            // consume ammo
            if (charges >= items[0].charges) {
                items.erase (items.begin());
            } else {
                items[0].charges -= charges;
            }
        }
    }
}

bool vehicle::fire_turret_internal (vparzu *part, vehicle_part2 *p, it_gun &gun, it_ammo &ammo, int charges, const std::string &extra_sound)
{
    const int x = global_x() + part->precalc_dx[0];
    const int y = global_y() + part->precalc_dy[0];
    // code copied form mattack::smg, mattack::flamethrower
    int range = ammo.type == fuel_type_gasoline ? 5 : 12;

    // Check for available power for turrets that use it.
    const int power = fuel_left(fuel_type_battery);
    if( gun.item_tags.count( "USE_UPS" ) ) {
        if( power < 5 ) { return false; }
    } else if( gun.item_tags.count( "USE_UPS_20" ) ) {
        if( power < 20 ) { return false; }
    } else if( gun.item_tags.count( "USE_UPS_40" ) ) {
        if( power < 40 ) { return false; }
    }
    npc tmp;
    tmp.set_fake( true );
    tmp.name = rmp_format(_("<veh_player>The %s"), p.name.c_str());
    tmp.skillLevel(gun.skill_used).level(8);
    tmp.skillLevel("gun").level(4);
    tmp.recoil = abs(velocity) / 100 / 4;
    tmp.posx = x;
    tmp.posy = y;
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    tmp.weapon = item(&gun, 0);
    it_ammo curam = ammo;
    tmp.weapon.curammo = &curam;
    tmp.weapon.charges = charges;

    const bool u_see = g->u_see(x, y);

    int fire_t, boo_hoo;
    Creature *target = tmp.auto_find_hostile_target(range, boo_hoo, fire_t);
    if (target == NULL) {
        if (u_see) {
            if (boo_hoo > 1) {
                g->add_msg(_("%s points in your direction and emits %d annoyed sounding beeps."),
                tmp.name.c_str(), boo_hoo);
            } else if (boo_hoo > 0) {
                g->add_msg(_("%s points in your direction and emits an IFF warning beep."),
                tmp.name.c_str());
            }
        }
        return false;
    }

    // make a noise, if extra noise is to be made
    if (extra_sound != "") {
        g->sound(x, y, 20, extra_sound);
    }
    // notify player if player can see the shot
    if( g->u_see(x, y) ) {
        g->add_msg(_("The %s fires its %s!"), name.c_str(), pp.name.c_str());
    }
    // Spawn a fake UPS to power any turreted weapons that need electricity.
    item tmp_ups( itypes["UPS_on"], 0 );
    // Drain a ton of power
    tmp_ups.charges = drain( fuel_type_battery, 1000 );
    item &ups_ref = tmp.i_add(tmp_ups);
    tmp.fire_gun(target->xpos(), target->ypos(), true);
    // Return whatever is left.
    refill( fuel_type_battery, ups_ref.charges );

    return true;
}

/**
 * Opens an openable part. If it's a multipart, opens
 * all attached parts as well.
 * @param part The part to open.
 */
void vehicle::open(vparzu *part)
{
    if(!part->has_part_with_feature("OPENABLE")) {
        debugmsg("Attempted to open non-openable part %d,%d on a %s!",
            part->mount_dx, part->mount_dy, name.c_str());
    } else {
        open_or_close(part, true);
        refresh_insides();
    }
}

/**
 * Closes an openable part. If it's a multipart, closes
 * all attached parts as well.
 * @param part The part to close.
 */
void vehicle::close(vparzu *part)
{
    if(!part->has_part_with_feature("OPENABLE")) {
        debugmsg("Attempted to close non-openable part %d,%d on a %s!",
            part->mount_dx, part->mount_dy, name.c_str());
    } else {
        open_or_close(part, false);
        refresh_insides();
    }
}

void vehicle::open_or_close(vparzu *part, bool opening)
{
    vehicle_part2 *p = part->part_with_feature("OPENABLE");
    if (p == NULL) {
        return;
    }
    const int ostate = opening ? 0 : 1;
    p->open = ostate;
    if(!p->has_feature("MULTISQUARE")) {
        return;
    }
    /* Find all other closed parts with the same ID in adjacent squares.
     * This is a tighter restriction than just looking for other Multisquare
     * Openable parts, and stops trunks from opening side doors and the like. */
    for(int dx = -1; dx <= +1; dx += 2) {
        for(int dy = -1; dy <= +1; dy += 2) {
            vparzu *other = vp_at(part->mount_dx + dx, part->mount_dy + dy);
            if (other == NULL) {
                continue;
            }
            vehicle_part2 *p = other->part_of_type(p->part_info().id);
            if (p != NULL && p->open != ostate) {
                open_or_close(other, opening);
            }
        }
    }
}

// a chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid(){
   if (last_turn > 13)
      //turning on the initial skid is delayed, so move==face, initially. This filters out that case.
      return;
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   //threshold of recovery is gaussianesque.

   if (fabs(dot) * 100 > dice(9,20)){
      g->add_msg(_("The %s recovers from its skid."), name.c_str());
      skidding = false; //face_vec takes over.
      velocity *= dot; //wheels absorb horizontal velocity.
      if(dot < -.8){
         //pointed backwards, velo-wise.
         velocity *= -1; //move backwards.
      }
      move = face;
   }
}

// if not skidding, move_vec == face_vec, mv <dot> fv == 1, velocity*1 is returned.
float vehicle::forward_velocity(){
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   return velocity * dot;
}

rl_vec2d vehicle::velo_vec(){
    rl_vec2d ret;
    if(skidding)
       ret = move_vec();
    else
       ret = face_vec();
    ret = ret.normalized();
    ret = ret * velocity;
    return ret;
}

// normalized.
rl_vec2d vehicle::move_vec(){
    float mx,my;
    mx = cos (move.dir() * M_PI/180);
    my = sin (move.dir() * M_PI/180);
    rl_vec2d ret(mx,my);
    return ret;
}

// normalized.
rl_vec2d vehicle::face_vec(){
    float fx,fy;
    fx = cos (face.dir() * M_PI/180);
    fy = sin (face.dir() * M_PI/180);
    rl_vec2d ret(fx,fy);
    return ret;
}

float get_collision_factor(float delta_v)
{
    if (abs(delta_v) <= 31) {
        return ( 1 - ( 0.9 * abs(delta_v) ) / 31 );
    } else {
        return 0.1;
    }
}
#if 0
vehicle::part_map_1 vehicle::all_part_positions(bool global) const
{
    part_map_1 result;
    const int gx = const_cast<vehicle*>(this)->global_x();
    const int gy = const_cast<vehicle*>(this)->global_y();
    for(int i = 0; i < parts.size(); i++) {
        const vehicle_part &pp = parts[i];
        if (global) {
            const int px = gx + pp.precalc_dx[0];
            const int py = gy + pp.precalc_dy[0];
            point p(px, py);
            result.insert(std::make_pair(p, i));
        } else {
            const int px = pp.precalc_dx[0];
            const int py = pp.precalc_dy[0];
            point p(px, py);
            result.insert(std::make_pair(p, i));
        }
    }
    return result;
}
#endif
void vehicle::move_precalc()
{
    for(vparmap::iterator a = pmap.begin(); a != pmap.end(); ++a) {
        vparzu &pp = a->second;
        pp.precalc_dx[0] = pp.precalc_dx[1];
        pp.precalc_dy[0] = pp.precalc_dy[1];
    }
}

bool vparzu::is_transparent() const
{
    if (!has_part_with_feature(VPFLAG_OPAQUE, true)) {
        return true;
    }
    const vehicle_part2 *vp = part_with_feature(VPFLAG_OPENABLE, true);
    // open opaque door
    return (vp != NULL && vp->open);
}

bool vparzu::is_obstacle() const
{
    const vehicle_part2 *dpart = part_with_feature(VPFLAG_OBSTACLE, true);
    if (dpart == NULL) {
        return false;
    }
    // Obstacle here, maybe it's an open door?
    return (!dpart->has_feature(VPFLAG_OPENABLE, true) || !dpart->open);
}

int vparzu::move_cost() const
{
    const vehicle_part2 *dpart = part_with_feature(VPFLAG_OBSTACLE, true);
    if (dpart != NULL) {
        // Obstacle here, maybe it's an open door?
        if (!dpart->has_feature(VPFLAG_OPENABLE, true) || !dpart->open)) {
            // Nope, not openable, or not open
            return 0;
        }
    }
    if (has_part_with_feature(VPFLAG_AISLE, true)) {
        return 2;
    }
    return 8;
}

vparzu *vp_at(const point &p)
{
    vparmap::iterator a = pmap.find(p);
    if (a == pmap.end()) {
        return NULL;
    }
    return &a->second;
}

const vparzu *vp_at(const point &p) const
{
    vparmap::const_iterator a = pmap.find(p);
    if (a == pmap.end()) {
        return NULL;
    }
    return &a->second;
}

vparzu &vehicle::vpat(const point &p)
{
    vparmap::iterator a = pmap.find(p);
    if (a == pmap.end()) {
        vparzu &vp = pmap[p];
        vp->mount_dx = p.x;
        vp->mount_dy = p.y;
        return vp;
    } else {
        return a->second;
    }
}

void vehicle::merge(const vehicle *veh) {
    const int dx = global_x() - veh->global_x();
    const int dy = global_y() - veh->global_y();
    for(vparmap::const_iterator a = veh->pmap.begin(); a != veh->pmap.end(); ++a) {
        const point p(a->first.x + dx, a->first.y + dy);
        vparzu &pp = vpat(p);
        pp.parts.insert(vp.parts.end(), a->second.parts.begin(), a->second.parts.end());
    }
}
