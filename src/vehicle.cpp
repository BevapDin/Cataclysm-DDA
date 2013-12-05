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
#include <cassert>

#include "debug.h"

const ammotype fuel_types[num_fuel_types] = { "gasoline", "battery", "plutonium", "plasma", "water" };

// Flag for vehicle_part::flags
#define INACTIVE 8

// Sort by part type,
struct part_type_comparator {
    vehicle *veh;
    part_type_comparator(vehicle *v) : veh(v) { }
    bool operator()(int a, int b) const {
        if(&(veh->part_info(a)) != &(veh->part_info(b))) {
            return &(veh->part_info(a)) < &(veh->part_info(b));
        }
        if(veh->parts[a].mount_dx != veh->parts[b].mount_dx) {
            return veh->parts[a].mount_dx < veh->parts[b].mount_dx;
        }
        if(veh->parts[a].mount_dy != veh->parts[b].mount_dy) {
            return veh->parts[a].mount_dy < veh->parts[b].mount_dy;
        }
        return a < b;
    }
};

#include <algorithm>

enum vehicle_controls {
 toggle_cruise_control,
 toggle_lights,
 toggle_overhead_lights,
 toggle_turrets,
 toggle_tracker,
 activate_horn,
 release_control,
 control_cancel,
 control_engines,
 control_lights,
 control_turrets,
 convert_vehicle,
 toggle_engine,
 toggle_fridge
};

vehicle::vehicle(game *ag, std::string type_id, int init_veh_fuel, int init_veh_status): g(ag), type(type_id)
{
    posx = 0;
    posy = 0;
    velocity = 0;
    turn_dir = 0;
    last_turn = 0;
    of_turn_carry = 0;
    turret_mode = 0;
    lights_power = 0;
    tracking_power = 0;
    cruise_velocity = 0;
    skidding = false;
    cruise_on = true;
    lights_on = false;
    tracking_on = false;
    overhead_lights_on = false;
    insides_dirty = true;
    cached_gen_turn = 0;

    //type can be null if the type_id parameter is omitted
    if(type != "null") {
      if(ag->vtypes.count(type) > 0) {
        //If this template already exists, copy it
        *this = *(ag->vtypes[type]);
        init_state(ag, init_veh_fuel, init_veh_status);
      }
    }
    precalc_mounts(0, face.dir());
}

vehicle::~vehicle()
{
}

bool vehicle::player_in_control (player *p)
{
    int veh_part;
    vehicle *veh = g->m.veh_at (p->posx, p->posy, veh_part);
    if (veh == NULL || veh != this)
        return false;
    return part_with_feature(veh_part, "CONTROLS", false) >= 0 && p->controlling_vehicle;
}

void vehicle::load (std::ifstream &stin)
{
    getline(stin, type);

    if ( type.size() > 1 && ( type[0] == '{' || type[1] == '{' ) ) {
        std::stringstream derp;
        derp << type;
        JsonIn jsin(&derp);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad vehicle json\n%s", jsonerr.c_str() );
        }
    } else {
        load_legacy(stin);
    }
    sort_parts();
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    //No need to check the same (x, y) spot more than once
    std::set< std::pair<int, int> > locations_checked;
    for (int i = 0; i < parts.size(); i++) {
        int next_x = parts[i].mount_dx;
        int next_y = parts[i].mount_dy;
        std::pair<int, int> mount_location = std::make_pair(next_x, next_y);

        if(locations_checked.count(mount_location) == 0) {
            std::vector<int> parts_here = parts_at_relative(next_x, next_y);
            bool found = false;
            for(std::vector<int>::iterator here = parts_here.begin();
                    here != parts_here.end(); here++) {
                if(part_info(*here).location == "structure") {
                    found = true;
                    break;
                }
            }
            if(!found) {
                //No frame here! Install one.
                vehicle_part new_part;
                new_part.id = "frame_vertical";
                new_part.mount_dx = next_x;
                new_part.mount_dy = next_y;
                new_part.hp = vehicle_part_types["frame_vertical"].durability;
                new_part.amount = 0;
                new_part.blood = 0;
                new_part.bigness = 0;
                parts.push_back (new_part);
            }
        }

        locations_checked.insert(mount_location);
    }
}

void vehicle::save (std::ofstream &stout)
{
    serialize(stout);
    stout << std::endl;
    return;
}

void vehicle::init_state(game* g, int init_veh_fuel, int init_veh_status)
{
    bool destroyEngine = false;
    bool destroyTires = false;
    bool blood_covered = false;

    std::map<std::string, int> consistent_bignesses;

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 1% to 7%
    int veh_fuel_mult = init_veh_fuel;
    if (init_veh_fuel == - 1)
     veh_fuel_mult = rng (1,7);
    if (init_veh_fuel > 100)
     veh_fuel_mult = 100;

    // im assuming vehicles only spawn in active maps
    levx = g->levx;
    levy = g->levy;

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamgaed
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if (init_veh_status == 0)
     veh_status = 0;
    if (init_veh_status == 1) {
     veh_status = 1;
     if (one_in(2)) {  // either engine or tires are destroyed
      destroyEngine = true;
     } else {
      destroyTires = true;
     }
    }

    //Turn on lights on some non-mint vehicles
    if(veh_status != 0 && one_in(20)) {
        lights_on = true;
    }

    //Turn flasher/overhead lights on separately (more likely since these are rarer)
    if(veh_status != 0 && one_in(4)) {
        overhead_lights_on = true;
    }

    //Don't bloodsplatter mint condition vehicles
    if(veh_status != 0 && one_in(10)) {
      blood_covered = true;
    }

    for (int p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, "VARIABLE_SIZE")){ // generate its bigness attribute.?
            if(consistent_bignesses.count(parts[p].id) < 1){
                //generate an item for this type, & cache its bigness
                item tmp (itypes[part_info(p).item], 0);
                consistent_bignesses[parts[p].id] = tmp.bigness;
            }
            parts[p].bigness = consistent_bignesses[parts[p].id];
        }
        if (part_flag(p, "FUEL_TANK")) {   // set fuel status
          parts[p].amount = part_info(p).size * veh_fuel_mult / 100;
        }

        if (part_flag(p, "OPENABLE")) {    // doors are closed
            if(!parts[p].open && one_in(4)) {
              open(p);
            }
        }
        if (part_flag(p, "BOARDABLE")) {      // no passengers
            parts[p].remove_flag(vehicle_part::passenger_flag);
        }

        // initial vehicle damage
        if (veh_status == 0) {
            // Completely mint condition vehicle
            parts[p].hp= part_info(p).durability;
        } else {
         //a bit of initial damage :)
         //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
         int broken = 8, unhurt = 20;
         int roll = dice(4,8);
         if(roll < unhurt){
            if (roll <= broken)
               parts[p].hp= 0;
            else
               parts[p].hp= ((float)(roll-broken) / (unhurt-broken)) * part_info(p).durability;
            }
         else // new.
         {
            parts[p].hp= part_info(p).durability;
         }

         if (destroyEngine) { // vehicle is disabled because engine is dead
          if (part_flag(p, "ENGINE")) {
           parts[p].hp= 0;
          }
         }
         if (destroyTires) { // vehicle is disabled because flat tires
          if (part_flag(p, "WHEEL")) {
             parts[p].hp= 0;
          }
         }

         /* Bloodsplatter the front-end parts. Assume anything with x > 0 is
          * the "front" of the vehicle (since the driver's seat is at (0, 0).
          * We'll be generous with the blood, since some may disappear before
          * the player gets a chance to see the vehicle. */
         if(blood_covered && parts[p].mount_dx > 0) {
           if(one_in(3)) {
             //Loads of blood. (200 = completely red vehicle part)
             parts[p].blood = rng(200, 600);
           } else {
             //Some blood
             parts[p].blood = rng(50, 200);
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
    for (int part_index = 0; part_index < parts.size(); part_index++) {
        //Skip any parts already mashed up
        if(parts[part_index].hp == 0) {
            continue;
        }

        vehicle_part next_part = parts[part_index];
        std::vector<int> parts_in_square = parts_at_relative(next_part.mount_dx, next_part.mount_dy);
        int structures_found = 0;
        for (int square_part_index = 0; square_part_index < parts_in_square.size(); square_part_index++) {
            if (part_info(parts_in_square[square_part_index]).location == "structure") {
                structures_found++;
            }
        }

        if(structures_found > 1) {
            //Destroy everything in the square
            for (int square_part_index = 0; square_part_index < parts_in_square.size(); square_part_index++) {
                parts[parts_in_square[square_part_index]].hp = 0;
            }
            continue;
        }

        //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
        int damage = (int) (dice(1, 12) * 0.1 * part_info(part_index).durability);
        parts[part_index].hp -= damage;
        if (parts[part_index].hp < 0) {
            parts[part_index].hp = 0;
        }
    }
}

void vehicle::use_controls()
{
    std::vector<vehicle_controls> options_choice;
    std::vector<uimenu_entry> options_message;
    // Always have this option
    int current = 0;
    int letgoent = 0;

    options_choice.push_back(toggle_cruise_control);
    options_message.push_back(uimenu_entry((cruise_on) ? _("Disable cruise control") :
                                           _("Enable cruise control"), 'c'));

    std::vector<int> engines;
    std::vector<int> lights;
    std::vector<int> turrets;
    current++;

    bool has_lights = false;
    bool has_overhead_lights = false;
    bool has_horn = false;
    bool has_turrets = false;
    bool has_tracker = false;
    bool has_engine = false;
    bool has_fridge = false;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "CONE_LIGHT")) {
            has_lights = true;
            lights.push_back(p);
        }
        if (part_flag(p, "CIRCLE_LIGHT")) {
            has_overhead_lights = true;
            lights.push_back(p);
        }
        if (part_flag(p, "LIGHT")) {
            has_lights = true;
        }
        if (part_flag(p, "TURRET")) {
            has_turrets = true;
            turrets.push_back(p);
        }
        if (part_flag(p, "HORN")) {
            has_horn = true;
        }
        if (part_flag(p, "TRACK")) {
            has_tracker = true;
        }
        if (part_flag(p, "ENGINE")) {
            has_engine = true;
            engines.push_back(p);
        }
        if (part_flag(p, "FRIDGE")) {
            has_fridge = true;
        }
    }
    std::sort(lights.begin(), lights.end(), part_type_comparator(this));
    std::sort(engines.begin(), engines.end(), part_type_comparator(this));

    // Lights if they are there - Note you can turn them on even when damaged, they just don't work
    if (has_lights) {
        options_choice.push_back(toggle_lights);
        options_message.push_back(uimenu_entry((lights_on) ? _("Turn off headlights") :
                                               _("Turn on headlights"), 'h'));
        current++;
    }

   if (has_overhead_lights) {
       options_choice.push_back(toggle_overhead_lights);
       options_message.push_back(uimenu_entry(overhead_lights_on ? _("Turn off overhead lights") :
                                              _("Turn on overhead lights"), 'v'));
       current++;
   }

    //Honk the horn!
    if (has_horn) {
        options_choice.push_back(activate_horn);
        options_message.push_back(uimenu_entry(_("Honk horn"), 'o'));
        current++;
    }

    // Turrets: off or burst mode
    if (has_turrets) {
        options_choice.push_back(toggle_turrets);
        options_message.push_back(uimenu_entry((0 == turret_mode) ? _("Switch turrets to burst mode") :
                                               _("Disable turrets"), 't'));
        current++;
    }

    // Turn the fridge on/off
    if (has_fridge) {
        options_choice.push_back(toggle_fridge);
        options_message.push_back(uimenu_entry(fridge_on ? _("Turn off fridge") :
                                               _("Turn on fridge"), 'f'));
        current++;
    }

    // Tracking on the overmap
    if (has_tracker) {
        options_choice.push_back(toggle_tracker);
        options_message.push_back(uimenu_entry((tracking_on) ? _("Disable tracking device") :
                                                _("Enable tracking device"), 'g'));

        current++;
    }

    if( !g->u.controlling_vehicle && tags.count("convertible") ) {
        options_choice.push_back(convert_vehicle);
        options_message.push_back(uimenu_entry(_("Fold bicycle"), 'f'));
        current++;
    }

    if (engines.size() > 1) {
        options_choice.push_back(control_engines);
        options_message.push_back(uimenu_entry(_("Control engines"), 'E'));
        current++;
    }

    if (lights.size() > 1) {
        options_choice.push_back(control_lights);
        options_message.push_back(uimenu_entry(_("Control lights"), 'L'));
        current++;
    }

    if (turrets.size() > 1) {
        options_choice.push_back(control_turrets);
        options_message.push_back(uimenu_entry(_("Control turrets"), 'T'));
        current++;
    }

    if (has_engine) {
        options_choice.push_back(toggle_engine);
        options_message.push_back(uimenu_entry((engine_on) ? _("Turn off the engine") :
                                               _("Turn on the engine"), 'e'));
        current++;
    }

    // Exit vehicle, if we are in it.
    if (g->u.controlling_vehicle &&
        g->m.veh_at(g->u.posx, g->u.posy) == this) {
        options_choice.push_back(release_control);
        options_message.push_back(uimenu_entry(_("Let go of controls"), 'l'));
        letgoent = current;
    }

    options_choice.push_back(control_cancel);
    options_message.push_back(uimenu_entry(_("Do nothing"), ' '));

    uimenu selectmenu;
    selectmenu.text = _("Vehicle controls");
    selectmenu.entries = options_message;
    selectmenu.selected = letgoent;
    selectmenu.query();
    int select = selectmenu.ret;

    if (select == UIMENU_INVALID) {
        return;
    }

    switch(options_choice[select]) {
    case toggle_cruise_control:
        cruise_on = !cruise_on;
        g->add_msg((cruise_on) ? _("Cruise control turned on") : _("Cruise control turned off"));
        break;
    case toggle_lights:
        if(lights_on || fuel_left("battery") ) {
            lights_on = !lights_on;
            g->add_msg((lights_on) ? _("Headlights turned on") : _("Headlights turned off"));
        } else {
            lights_on = false;
            g->add_msg(_("The headlights won't come on!"));
        }
        break;
    case toggle_overhead_lights:
        if( !overhead_lights_on || fuel_left("battery") ) {
            overhead_lights_on = !overhead_lights_on;
            g->add_msg((overhead_lights_on) ? _("Overhead lights turned on") :
                       _("Overhead lights turned off"));
        } else {
            overhead_lights_on = false;
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
        if( !fridge_on || fuel_left("battery") ) {
            fridge_on = !fridge_on;
            g->add_msg((fridge_on) ? _("Fridge turned on") :
                       _("Fridge turned off"));
        } else {
            g->add_msg(_("The fridge won't turn on!"));
        }
        break;
    case toggle_engine:
        if (engine_on) {
          engine_on = false;
          g->add_msg(_("You turn the engine off."));
        }
        else {
          if (total_power () < 1) {
            if (total_power (false) < 1)
                g->add_msg (_("The %s doesn't have an engine!"), name.c_str());
            else
                g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
          }
          else {
            engine_on = true;
            // TODO: Make chance of success based on engine condition.
            g->add_msg(_("You turn the engine on."));
          }
        }
        break;
    case release_control:
        g->u.controlling_vehicle = false;
        g->add_msg(_("You let go of the controls."));
        break;
    case control_engines:
        while(toogle_active_menu(engines, "activate/deactive engines")) { ; }
        break;
    case control_lights:
        while(toogle_active_menu(lights, "activate/deactive lights")) { ; }
        break;
    case control_turrets:
        while(toogle_active_menu(turrets, "activate/deactive turrets")) { ; }
        break;
    case convert_vehicle:
    {
        g->add_msg(_("You painstakingly pack the bicycle into a portable configuration."));
        // create a folding bicycle item
        item bicycle;
        bicycle.make( itypes["folding_bicycle"] );

        std::ostringstream part_hps;
        // Stash part HP in item
        for (int p = 0; p < parts.size(); p++)
        {
            part_hps << parts[p].hp << " ";
            if( part_flag( p, "CARGO" ) ) {
                for( std::vector<item>::iterator it = parts[p].items.begin();
                     it != parts[p].items.end(); ++it ) {
                    g->m.add_item_or_charges( g->u.posx, g->u.posy, *it );
                }
            }
        }
        bicycle.item_vars["folding_bicycle_parts"] = part_hps.str();

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
        } else if (fuel_left("battery"))
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

bool vehicle::toogle_active_menu(const std::vector<int> &parts_to_toggle, const std::string &title) {
    uimenu selectmenu;
    selectmenu.text = title;
    for(int i = 0; i < parts_to_toggle.size(); i++) {
        const int p = parts_to_toggle[i];
        const vehicle_part &part = parts[p];
        std::ostringstream buffer;
        buffer << (part.active() ? "deactivate" : "activate");
        buffer << " " << part_info(p).name << " at " << part.mount_dx << "," << part.mount_dy;
        selectmenu.entries.push_back(uimenu_entry(buffer.str(), i < 10 ? '0' + i : 'a' + i - 10));
    }
    selectmenu.entries.push_back(uimenu_entry("activate all", 'A'));
    selectmenu.entries.push_back(uimenu_entry("deactivate all", 'D'));
    selectmenu.entries.push_back(uimenu_entry("Do nothing", ' '));
    selectmenu.query();
    int select = selectmenu.ret;
    if(select >= 0 && select < parts_to_toggle.size()) {
        const int p = parts_to_toggle[select];
        vehicle_part &part = parts[p];
        if(part.active()) {
            part.flags |= mfb(INACTIVE);
        } else {
            part.flags &= ~mfb(INACTIVE);
        }
        return true;
    } else if(select == parts_to_toggle.size()) {
        for(int i = 0; i < parts_to_toggle.size(); i++) {
            const int p = parts_to_toggle[i];
            parts[p].flags &= ~mfb(INACTIVE);
        }
        return true;
    } else if(select == parts_to_toggle.size() + 1) {
        for(int i = 0; i < parts_to_toggle.size(); i++) {
            const int p = parts_to_toggle[i];
            parts[p].flags |= mfb(INACTIVE);
        }
        return true;
    } else {
        return false;
    }
}

void vehicle::honk_horn()
{
    for(int h = 0; h < horns.size(); h++) {
        //Get global position of horn
        int horn_x = parts[horns[h]].mount_dx;
        int horn_y = parts[horns[h]].mount_dy;
        coord_translate( horn_x, horn_y, horn_x, horn_y );
        horn_x += global_x();
        horn_y += global_y();
        //Determine sound
        vpart_info &horn_type=part_info(horns[h]);
        if( horn_type.bonus >= 40 ){
            g->sound( horn_x, horn_y, horn_type.bonus, _("HOOOOORNK!") );
        } else if( horn_type.bonus >= 20 ){
            g->sound( horn_x, horn_y, horn_type.bonus, _("BEEEP!") );
        } else{
            g->sound( horn_x, horn_y, horn_type.bonus, _("honk.") );
        }
        drain("battery", 10);
    }
}

vpart_info& vehicle::part_info (int index)
{
    if (index >= 0 && index < parts.size()) {
        if(parts[index].vpart == 0) {
            parts[index].vpart = &(vehicle_part_types[parts[index].id]);
        }
        return *parts[index].vpart;
    } else {
        return vehicle_part_types["null"];
    }
}

// engines & solar panels all have power.
// engines consume, whilst panels provide.
int vehicle::part_power (int index){
   if (!part_flag(index, "ENGINE") &&
       !part_flag(index, "SOLAR_PANEL")) {
      return 0; //not an engine.
   }
   if(parts[index].hp <= 0) {
      return 0; //broken.
   }
   if(parts[index].inactive()) {
      return 0; //not active.
   }
   if(part_flag (index, "VARIABLE_SIZE")){ // example: 2.42-L V-twin engine
      return parts[index].bigness;
   }
   else // example: foot crank
   {
      return part_info(index).power;
   }
}

bool vehicle::has_structural_part(int dx, int dy)
{
    std::vector<int> parts_here = parts_at_relative(dx, dy);

    for( std::vector<int>::iterator it = parts_here.begin(); it != parts_here.end(); ++it ) {
        if(part_info(*it).location == "structure" && !part_info(*it).has_flag("PROTRUSION")) {
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
bool vehicle::can_mount (int dx, int dy, std::string id)
{
    //The part has to actually exist.
    if(vehicle_part_types.count(id) == 0) {
        return false;
    }

    //It also has to be a real part, not the null part
    const vpart_info part = vehicle_part_types[id];
    if(part.has_flag("NOINSTALL")) {
        return false;
    }

    const std::vector<int> parts_in_square = parts_at_relative(dx, dy);

    //First part in an empty square MUST be a structural part
    if(parts_in_square.empty() && part.location != "structure") {
        return false;
    }

    //No other part can be placed on a protrusion
    if(!parts_in_square.empty() && part_info(parts_in_square[0]).has_flag("PROTRUSION")) {
        return false;
    }

    //No part type can stack with itself, or any other part in the same slot
    for( std::vector<int>::const_iterator part_it = parts_in_square.begin();
         part_it != parts_in_square.end(); ++part_it ) {
        const vpart_info other_part = vehicle_part_types[parts[*part_it].id];

        //Parts with no location can stack with each other (but not themselves)
        if(part.id == other_part.id ||
                (!part.location.empty() && part.location == other_part.location)) {
            return false;
        }
        // Until we have an interface for handling multiple components with CARGO space,
        // exclude them from being mounted in the same tile.
        if( part.has_flag( "CARGO" ) && other_part.has_flag( "CARGO" ) ) {
            return false;
        }

    }

    //All parts after the first must be installed on or next to an existing part
    if(parts.size() > 0) {
        if(!has_structural_part(dx, dy) &&
                !has_structural_part(dx+1, dy) &&
                !has_structural_part(dx, dy+1) &&
                !has_structural_part(dx-1, dy) &&
                !has_structural_part(dx, dy-1)) {
            return false;
        }
    }

    //Seatbelts must be installed on a seat
    if(vehicle_part_types[id].has_flag("SEATBELT")) {
        bool anchor_found = false;
        for( std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it ) {
            if(part_info(*it).has_flag("BELTABLE")) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    // curtains must be installed on (reinforced)windshields
    // TODO: do this automatically using "location":"on_mountpoint"
    if (vehicle_part_types[id].has_flag("CURTAIN")) {
        bool anchor_found = false;
        for ( std::vector<int>::const_iterator it = parts_in_square.begin();
                it != parts_in_square.end(); it++) {
            if (part_info(*it).has_flag("WINDOW")) {
                anchor_found = true;
            }
        }
        if (!anchor_found) {
            return false;
        }
    }

    //Anything not explicitly denied is permitted
    return true;
}

bool vehicle::can_unmount (int p)
{
    if(p < 0 || p > parts.size()) {
        return false;
    }

    int dx = parts[p].mount_dx;
    int dy = parts[p].mount_dy;

    std::vector<int> parts_in_square = parts_at_relative(dx, dy);

    //Can't remove a seat if there's still a seatbelt there
    if(part_flag(p, "BELTABLE") && part_with_feature(p, "SEATBELT") >= 0) {
        return false;
    }

    // Can't remove a window with curtains still on it
    if(part_flag(p, "WINDOW") && part_with_feature(p, "CURTAIN") >=0) {
        return false;
    }

    //Structural parts have extra requirements
    if(part_info(p).location == "structure") {

        /* Can't unmount the last structural part at (0, 0) unless it is the
         * last part of the whole vehicle, as the map relies on vehicles having
         * a part at (0, 0) to locate them with. */
        if(dx == 0 && dy == 0 && parts_in_square.size() == 1 && parts.size() > 1) {
            return false;
        }

        /* To remove a structural part, there can be only structural parts left
         * in that square (might be more than one in the case of wreckage) */
        for(int part_index = 0; part_index < parts_in_square.size(); part_index++) {
            if(part_info(parts_in_square[part_index]).location != "structure") {
                return false;
            }
        }

        //If it's the last part in the square...
        if(parts_in_square.size() == 1) {

            /* This is the tricky part: We can't remove a part that would cause
             * the vehicle to 'break into two' (like removing the middle section
             * of a quad bike, for instance). This basically requires doing some
             * breadth-first searches to ensure previously connected parts are
             * still connected. */

            //First, find all the squares connected to the one we're removing
            std::vector<vehicle_part> connected_parts;

            for(int i = 0; i < 4; i++) {
                int next_x = i < 2 ? (i == 0 ? -1 : 1) : 0;
                int next_y = i < 2 ? 0 : (i == 2 ? -1 : 1);
                std::vector<int> parts_over_there = parts_at_relative(dx + next_x, dy + next_y);
                //Ignore empty squares
                if(parts_over_there.size() > 0) {
                    //Just need one part from the square to track the x/y
                    connected_parts.push_back(parts[parts_over_there[0]]);
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
                    if(!is_connected(connected_parts[0], connected_parts[next_part], parts[p])) {
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
bool vehicle::is_connected(vehicle_part &to, vehicle_part &from, vehicle_part &excluded)
{
    int target_x = to.mount_dx;
    int target_y = to.mount_dy;

    int excluded_x = excluded.mount_dx;
    int excluded_y = excluded.mount_dy;

    //Breadth-first-search components
    std::list<vehicle_part> discovered;
    vehicle_part current_part;
    std::list<vehicle_part> searched;

    //We begin with just the start point
    discovered.push_back(from);

    while(!discovered.empty()) {
        current_part = discovered.front();
        discovered.pop_front();
        int current_x = current_part.mount_dx;
        int current_y = current_part.mount_dy;

        for(int i = 0; i < 4; i++) {
            int next_x = current_x + (i < 2 ? (i == 0 ? -1 : 1) : 0);
            int next_y = current_y + (i < 2 ? 0 : (i == 2 ? -1 : 1));

            if(next_x == target_x && next_y == target_y) {
                //Success!
                return true;
            } else if(next_x == excluded_x && next_y == excluded_y) {
                //There might be a path, but we're not allowed to go that way
                continue;
            }

            std::vector<int> parts_there = parts_at_relative(next_x, next_y);

            if(!parts_there.empty()) {
                vehicle_part next_part = parts[parts_there[0]];
                //Only add the part if we haven't been here before
                bool found = false;
                for(std::list<vehicle_part>::iterator it = discovered.begin();
                        it != discovered.end(); it++) {
                    if(it->mount_dx == next_x && it->mount_dy == next_y) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    for(std::list<vehicle_part>::iterator it = searched.begin();
                        it != searched.end(); it++) {
                        if(it->mount_dx == next_x && it->mount_dy == next_y) {
                            found = true;
                            break;
                        }
                    }
                }
                if(!found) {
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
int vehicle::install_part (int dx, int dy, std::string id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id)) {
        return -1;  // no money -- no ski!
    }
    vehicle_part new_part;
    new_part.id = id;
    new_part.mount_dx = dx;
    new_part.mount_dy = dy;
    new_part.hp = hp < 0 ? vehicle_part_types[id].durability : hp;
    new_part.amount = 0;
    new_part.blood = 0;
    item tmp(itypes[vehicle_part_types[id].item], 0);
    new_part.bigness = tmp.bigness;
    parts.push_back (new_part);

    if(part_flag(parts.size()-1,"HORN")) {
        horns.push_back(parts.size() - 1);
    }
    if(part_flag(parts.size()-1,"FUEL_TANK"))
        fuel.push_back(parts.size()-1);

    find_power ();
    find_exhaust ();
    precalc_mounts (0, face.dir());
    insides_dirty = true;
    sort_parts();
    std::vector<int> parts_here = parts_at_relative(dx, dy);
    for(size_t i = 0; i < parts_here.size(); i++) {
        if(parts[parts_here[i]].id == id) {
            return parts_here[i];
        }
    }
    debugmsg("Did not found new installed vpart %s at %d,%d", id.c_str(), dx, dy);
    return parts.size() - 1;
}

// share damage & bigness betwixt veh_parts & items.
void vehicle::get_part_properties_from_item(game* g, int partnum, item& i){
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = itypes[pitmid];
    if(itemtype->is_var_veh_part())
       parts[partnum].bigness = i.bigness;

    // item damage is 0,1,2,3, or 4. part hp is 1..durability.
    // assuming it rusts. other item materials disentigrate at different rates...
    int health = 5 - i.damage;
    health *= part_info(partnum).durability; //[0,dur]
    health /= 5;
    parts[partnum].hp = health;
}
void vehicle::give_part_properties_to_item(game* g, int partnum, item& i){
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = itypes[pitmid];
    if(itemtype->is_var_veh_part())
       i.bigness = parts[partnum].bigness;

    // remove charges if part is made of a tool
    if(itemtype->is_tool())
        i.charges = 0;

    // translate part damage to item damage.
    // max damage is 4, min damage 0.
    // this is very lossy.
    int dam;
    float hpofdur = (float)parts[partnum].hp / part_info(partnum).durability;
    dam = (1 - hpofdur) * 5;
    if (dam > 4) dam = 4;
    if (dam < 0) dam = 0;
    i.damage = dam;
}

void vehicle::remove_part (int p)
{
    if (part_flag(p, "TRACK")) {
        // disable tracking if there are no other trackers installed.
        if (tracking_on)
        {
            bool has_tracker = false;
            for (int i = 0; i != parts.size(); i++){
                if (i != p && part_flag(i, "TRACK")){
                    has_tracker = true;
                    break;
                }
            }
            if (!has_tracker){ // disable tracking
                g->cur_om->remove_vehicle(om_id);
                tracking_on = false;
            }
        }
    }

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if(part_flag(p, "WINDOW")) {
        int curtain = part_with_feature(p, "CURTAIN", false);
        if (curtain >= 0) {
            int x = parts[curtain].precalc_dx[0], y = parts[curtain].precalc_dy[0];
            item it = item_from_part(curtain);
            g->m.add_item_or_charges(global_x() + x, global_y() + y, it, 2);
            remove_part(curtain);
        }
    }

    parts.erase(parts.begin() + p);
    find_horns ();
    find_power ();
    find_fuel_tanks ();
    find_exhaust ();
    precalc_mounts (0, face.dir());
    insides_dirty = true;

    if(parts.size() == 0) {
        g->m.destroy_vehicle(this);
    } else {
        g->m.update_vehicle_cache(this, false);
    }
}

/**
 * Breaks the specified part into the pieces defined by its breaks_into entry.
 * @param p The index of the part to break.
 * @param x The map x-coordinate to place pieces at (give or take).
 * @param y The map y-coordinate to place pieces at (give or take).
 * @param scatter If true, pieces are scattered near the target square.
 */
void vehicle::break_part_into_pieces(int p, int x, int y, bool scatter) {
    std::vector<break_entry> break_info = part_info(p).breaks_into;
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

item vehicle::item_from_part( int part )
{
    itype_id itm = part_info(part).item;
    int bigness = parts[part].bigness;
    itype* parttype = itypes[itm];
    item tmp(parttype, g->turn);

    //transfer damage, etc.
    give_part_properties_to_item(g, part, tmp);
    if( parttype->is_var_veh_part() ) {
        tmp.bigness = bigness;
    }
    return tmp;
}

std::vector<int> vehicle::parts_at_relative (int dx, int dy)
{
    std::vector<int> res;
    if(parts.empty()) {
        return res;
    }
    int l = 0;
    int u = parts.size();
    while(u != l) {
        const int pivo = (u + l) / 2;
        if(parts[pivo].mount_dx < dx) {
            l = pivo + 1;
        } else if(parts[pivo].mount_dx > dx) {
            u = pivo;
        } else if(parts[pivo].mount_dy < dy) {
            l = pivo + 1;
        } else if(parts[pivo].mount_dy > dy) {
            u = pivo;
        } else {
            // Found matching part, stop here
            assert(parts[pivo].mount_dx == dx && parts[pivo].mount_dy == dy);
            u = pivo;
            break;
        }
    }
    if(u >= parts.size()) {
        return res;
    }
    while(u > 0) {
        // move back to the first matching part
        if(parts[u - 1].mount_dx == dx && parts[u - 1].mount_dy == dy) {
            u--;
        } else {
            // part[u-1] does not match
            break;
        }
    }
    for( ; u < parts.size() && parts[u].mount_dx == dx && parts[u].mount_dy == dy; u++) {
        res.push_back(u);
    }
    return res;
}

void vpart_range(vehicle *veh, int &part_begin, int &part_end, int dx, int dy) {
    part_end = part_begin + 1;
    while(part_begin > 0) {
        const vehicle_part &vp = veh->parts[part_begin - 1];
        if(vp.mount_dx == dx && vp.mount_dy == dy) {
            part_begin--;
        } else {
            break;
        }
    }
    while(part_end < veh->parts.size()) {
        const vehicle_part &vp = veh->parts[part_end];
        if(vp.mount_dx == dx && vp.mount_dy == dy) {
            part_end++;
        } else {
            break;
        }
    }
    assert(part_begin < part_end);
}

int vehicle::part_with_feature (int part, const std::string &flag, bool unbroken)
{
    const int dx = parts[part].mount_dx;
    const int dy = parts[part].mount_dy;
    while(part > 0) {
        if(parts[part - 1].mount_dx == dx && parts[part - 1].mount_dy == dy) {
            part--;
        } else {
            break;
        }
    }
    for (int i = part; i < parts.size(); i++) {
        if (parts[i].mount_dx == dx && parts[i].mount_dy == dy) {
            if (part_flag(i, flag) && (!unbroken || parts[i].hp > 0)) {
                return i;
            }
        } else {
            break;
        }
    }
    return -1;
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
 * @return A list of indices to all the parts with the specified feature.
 */
std::vector<int> vehicle::all_parts_with_feature(const std::string& feature, bool unbroken)
{
    std::vector<int> parts_found;
    for(int part_index = 0; part_index < parts.size(); part_index++) {
        if(part_info(part_index).has_flag(feature) &&
                (!unbroken || parts[part_index].hp > 0)) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

/**
 * Returns all parts in the vehicle that exist in the given location slot. If
 * the empty string is passed in, returns all parts with no slot.
 * @param location The location slot to get parts for.
 * @return A list of indices to all parts with the specified location.
 */
std::vector<int> vehicle::all_parts_at_location(const std::string& location)
{
    std::vector<int> parts_found;
    for(int part_index = 0; part_index < parts.size(); part_index++) {
        if(part_info(part_index).location == location) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

bool vehicle::part_flag (int part, const std::string &flag)
{
    if (part < 0 || part >= parts.size()) {
        return false;
    } else {
        return part_info(part).has_flag(flag);
    }
}

int vehicle::part_at(int dx, int dy)
{
    for (int p = 0; p < parts.size(); p++) {
        if (parts[p].precalc_dx[0] == dx && parts[p].precalc_dy[0] == dy) {
            return p;
        }
    }
    return -1;
}

int vehicle::global_part_at(int x, int y)
{
 int dx = x - global_x();
 int dy = y - global_y();
 return part_at(dx,dy);
}

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
  return -1;
}

/**
 * Returns which part (as an index into the parts list) is the one that will be
 * displayed for the given square. Returns -1 if there are no parts in that
 * square.
 * @param local_x The local x-coordinate.
 * @param local_y The local y-coordinate.
 * @return The index of the part that will be displayed.
 */
int vehicle::part_displayed_at(int local_x, int local_y)
{
    std::vector<int> parts_in_square = parts_at_relative(local_x, local_y);

    if(parts_in_square.empty()) {
        return -1;
    }

    int top_part = 0;
    for(int index = 1; index < parts_in_square.size(); index++) {
        if(part_info(parts_in_square[top_part]).z_order <
                part_info(parts_in_square[index]).z_order) {
            top_part = index;
        }
    }

    return parts_in_square[top_part];
}

char vehicle::part_sym (int p)
{
    if (p < 0 || p >= parts.size()) {
        return ' ';
    }

    int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);

    if (part_flag (displayed_part, "OPENABLE") && parts[displayed_part].open) {
        return '\''; // open door
    } else {
        return parts[displayed_part].hp <= 0 ?
            part_info(displayed_part).sym_broken : part_info(displayed_part).sym;
    }
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
std::string vehicle::part_id_string(int p, char &part_mod)
{
    part_mod = 0;
    std::string idinfo;
    if (p < 0 || p >= parts.size()){
        return "";
    }

    int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);
    idinfo = parts[displayed_part].id;

    if (part_flag (displayed_part, "OPENABLE") && parts[displayed_part].open) {
        part_mod = 1; // open
    } else if (parts[displayed_part].hp <= 0){
        part_mod = 2; // broken
    }

    return idinfo;
}

bool invert_cargo_color = true;
bool set_armor_color = true;

nc_color vehicle::part_color (int p)
{
    if (p < 0 || p >= parts.size()) {
        return c_black;
    }

    nc_color col;

    //If armoring is present, it colors the visible part
    int parm = part_with_feature(p, "ARMOR", false);
    if (parm >= 0 && set_armor_color) {
        col = part_info(parm).color;
    } else {

        int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);

        if (parts[displayed_part].blood > 200) {
            col = c_red;
        } else if (parts[displayed_part].blood > 0) {
            col = c_ltred;
        } else if (parts[displayed_part].hp <= 0) {
            col = part_info(displayed_part).color_broken;
        } else {
            col = part_info(displayed_part).color;
        }

    }

    // curtains turn windshields gray
    int curtains = part_with_feature(p, "CURTAIN", false);
    if (curtains >= 0) {
        if (part_with_feature(p, "WINDOW", true) >= 0 && !parts[curtains].open)
            col = part_info(curtains).color;
    }

    //Invert colors for cargo parts with stuff in them
    int cargo_part = part_with_feature(p, "CARGO");
    if(cargo_part > 0 && !parts[cargo_part].items.empty()) {
        if(!invert_cargo_color && this == g->m.veh_at(g->u.posx, g->u.posy))
        return col;
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
int vehicle::print_part_desc(WINDOW *win, int y1, int width, int p, int hl /*= -1*/)
{
    if (p < 0 || p >= parts.size()) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
    int y = y1;
    for (int i = 0; i < pl.size(); i++)
    {
        int dur = part_info (pl[i]).durability;
        int per_cond = parts[pl[i]].hp * 100 / (dur < 1? 1 : dur);
        nc_color col_cond;
        if (parts[pl[i]].hp >= dur) {
            col_cond = c_green;
        } else if (per_cond >= 80) {
            col_cond = c_ltgreen;
        } else if (per_cond >= 50) {
            col_cond = c_yellow;
        } else if (per_cond >= 20) {
            col_cond = c_ltred;
        } else if (parts[pl[i]].hp > 0) {
            col_cond = c_red;
        } else { //Broken
            col_cond = c_dkgray;
        }

        std::string partname;
        // part bigness, if that's relevant.
        if (part_flag(pl[i], "VARIABLE_SIZE") && part_flag(pl[i], "ENGINE")) {
            //~ 2.8-Liter engine
            partname = string_format(_("%4.2f-Liter %s"),
                                     (float)(parts[pl[i]].bigness) / 100,
                                     part_info(pl[i]).name.c_str());
        } else if (part_flag(pl[i], "VARIABLE_SIZE") && part_flag(pl[i], "WHEEL")) {
            //~ 14" wheel
            partname = string_format(_("%d\" %s"),
                                     parts[pl[i]].bigness,
                                     part_info(pl[i]).name.c_str());
        } else {
            partname = part_info(pl[i]).name;
        }

        bool armor = part_flag(pl[i], "ARMOR");
        std::string left_sym, right_sym;
        if(armor) {
            left_sym = "("; right_sym = ")";
        } else if(part_info(pl[i]).location == "structure") {
            left_sym = "["; right_sym = "]";
        } else {
            left_sym = "-"; right_sym = "-";
        }

        mvwprintz(win, y, 1, i == hl? hilite(c_ltgray) : c_ltgray, left_sym.c_str());
        mvwprintz(win, y, 2, i == hl? hilite(col_cond) : col_cond, partname.c_str());
        mvwprintz(win, y, 2 + utf8_width(partname.c_str()), i == hl? hilite(c_ltgray) : c_ltgray, right_sym.c_str());
//         mvwprintz(win, y, 3 + utf8_width(part_info(pl[i]).name), c_ltred, "%d", parts[pl[i]].blood);

        if (i == 0 && is_inside(pl[i])) {
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

void vehicle::print_fuel_indicator (void *w, int y, int x, bool fullsize, bool verbose)
{
    WINDOW *win = (WINDOW *) w;
    const nc_color fcs[num_fuel_types] = { c_ltred, c_yellow, c_ltgreen, c_ltblue, c_ltcyan };
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_ltgray;
    int yofs=0;
    for (int i = 0; i < num_fuel_types; i++)
    {
        int cap = fuel_capacity(fuel_types[i]);
        if (cap > 0 && ( basic_consumption(fuel_types[i]) > 0 || fullsize ) )
        {
            mvwprintz(win, y+yofs, x, col_indf1, "E...F");
            int amnt = cap > 0? fuel_left(fuel_types[i]) * 99 / cap : 0;
            int indf = (amnt / 20) % 5;
            mvwprintz(win, y+yofs, x + indf, fcs[i], "%c", fsyms[indf]);
            if(verbose) {
              if(g->debugmon) {
                mvwprintz(win, y+yofs, x+6, fcs[i], "%d/%d",fuel_left(fuel_types[i]),cap);
              } else {
                mvwprintz(win, y+yofs, x+6, fcs[i], "%d",(fuel_left(fuel_types[i])*100)/cap);
                wprintz(win, c_ltgray, "%c",045);
              }
            }
            if(fullsize) yofs++;
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
    if (idir < 0 || idir > 1)
        idir = 0;
    for (int p = 0; p < parts.size(); p++)
    {
        int dx, dy;
        coord_translate (dir, parts[p].mount_dx, parts[p].mount_dy, dx, dy);
        parts[p].precalc_dx[idir] = dx;
        parts[p].precalc_dy[idir] = dy;
    }
}

std::vector<int> vehicle::boarded_parts()
{
    std::vector<int> res;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag (p, "BOARDABLE") &&
                parts[p].has_flag(vehicle_part::passenger_flag)) {
            res.push_back (p);
        }
    }
    return res;
}

int vehicle::free_seat()
{
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag (p, "BOARDABLE") &&
               !parts[p].has_flag(vehicle_part::passenger_flag)) {
            return p;
        }
    }
    return -1;
}

player *vehicle::get_passenger (int p)
{
    p = part_with_feature (p, "BOARDABLE", false);
    if (p >= 0 && parts[p].has_flag(vehicle_part::passenger_flag))
    {
     const int player_id = parts[p].passenger_id;
     if( player_id == g->u.getID()) {
      return &g->u;
     }
     int npcdex = g->npc_by_id (player_id);
     if (npcdex >= 0) {
      return g->active_npc[npcdex];
     }
    }
    return 0;
}

int vehicle::global_x ()
{
    return smx * SEEX + posx;
}

int vehicle::global_y ()
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

int vehicle::total_mass()
{
    if(cached_gen_turn + 10 > (int) g->turn) {
        return cached_mass;
    }
    int m = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        m += itypes[part_info(i).item]->weight;
        for (int j = 0; j < parts[i].items.size(); j++) {
            m += parts[i].items[j].type->weight;
        }
        if (part_flag(i,"BOARDABLE") && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m += 81500; // TODO: get real weight
        }
    }
    cached_mass = m/1000;
    cached_gen_turn = (int) g->turn;
    return m/1000;
}

void vehicle::center_of_mass(int &x, int &y)
{
    float xf = 0, yf = 0;
    int m_total = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        int m_part = 0;
        m_part += itypes[part_info(i).item]->weight;
        for (int j = 0; j < parts[i].items.size(); j++) {
            m_part += parts[i].items[j].type->weight;
        }
        if (part_flag(i,"BOARDABLE") && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m_part += 81500; // TODO: get real weight
        }
        xf += parts[i].precalc_dx[0] * m_part / 1000;
        yf += parts[i].precalc_dy[0] * m_part / 1000;
        m_total += m_part;
    }
    m_total = m_total / 1000;
    xf /= m_total;
    yf /= m_total;
    x = int(xf + 0.5); //round to nearest
    y = int(yf + 0.5);
}

int vehicle::fuel_left (ammotype ftype, bool for_engine)
{
    int fl = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "FUEL_TANK") &&
            (ftype == part_info(p).fuel_type ||
            (for_engine && ftype == "battery" && part_info(p).fuel_type == "plutonium"))) {
            fl += parts[p].amount;
        }
    }
    return fl;
}

int vehicle::fuel_capacity (ammotype ftype)
{
    int cap = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "FUEL_TANK") && ftype == part_info(p).fuel_type) {
            cap += part_info(p).size;
        }
    }
    return cap;
}

int vehicle::refill (ammotype ftype, int amount)
{
    for (int p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, "FUEL_TANK") &&
            part_info(p).fuel_type == ftype &&
            parts[p].amount < part_info(p).size &&
            parts[p].hp > 0)
        {
            int need = part_info(p).size - parts[p].amount;
            if (amount < need)
            {
                parts[p].amount += amount;
                return 0;
            }
            else
            {
                parts[p].amount += need;
                amount -= need;
            }
        }
    }
    return amount;
}

int vehicle::drain (ammotype ftype, int amount) {
  int drained = 0;

  for (int p = 0; p < fuel.size(); p++) {
    vehicle_part &tank=parts[fuel[p]];
    if (part_info(fuel[p]).fuel_type == ftype && tank.amount > 0) {
      if (tank.amount > (amount - drained)) {
        tank.amount -= (amount - drained);
        drained = amount;
        break;
      } else {
        drained += tank.amount;
        tank.amount = 0;
      }
    }
  }

  return drained;
}

int vehicle::basic_consumption (ammotype ftype)
{
    if (ftype == "plutonium")
      ftype = "battery";
    int cnt = 0;
    int fcon = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "ENGINE") && parts[p].active() &&
            ftype == part_info(p).fuel_type &&
            parts[p].hp > 0)
        {
            fcon += part_power(p);
            cnt++;
        }
    }
    if (fcon < 100 && cnt > 0) {
        fcon = 100;
    }
    return fcon;
}

int vehicle::total_power (bool fueled)
{
    int pwr = 0;
    int cnt = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, "ENGINE") && parts[p].active() &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            pwr += part_power(p);
            cnt++;
        }
    if (cnt > 1) {
        pwr = pwr * 4 / (4 + cnt -1);
    }
    return pwr;
}

int vehicle::solar_power ()
{
    int pwr = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "SOLAR_PANEL") && parts[p].hp > 0) {
            int part_x = global_x() + parts[p].precalc_dx[0];
            int part_y = global_y() + parts[p].precalc_dy[0];
            // Can't use g->in_sunlight() because it factors in vehicle roofs.
            if( !g->m.has_flag_ter_or_furn( "INDOORS", part_x, part_y ) ) {
                pwr += (part_power(p) * g->natural_light_level()) / DAYLIGHT_LEVEL;
            }
        }
    }
    return pwr;
}

int vehicle::acceleration (bool fueled)
{
    return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
}

int vehicle::max_velocity (bool fueled)
{
    return total_power (fueled) * 80;
}

int vehicle::safe_velocity (bool fueled)
{
    int pwrs = 0;
    int cnt = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, "ENGINE") && parts[p].active() &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            int m2c = 100;

            if( part_info(p).fuel_type == "gasoline" )    m2c = 60;
            else if( part_info(p).fuel_type == "plasma" ) m2c = 75;
            else if( part_info(p).fuel_type == "battery" )   m2c = 90;
            else if( part_info(p).fuel_type == "muscle" ) m2c = 45;

            pwrs += part_power(p) * m2c / 100;
            cnt++;
        }
    if (cnt > 0) {
        pwrs = pwrs * 4 / (4 + cnt -1);
    }
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}

int vehicle::noise (bool fueled, bool gas_only)
{
    int pwrs = 0;
    int cnt = 0;
    int muffle = 100;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "MUFFLER") && parts[p].hp > 0 && part_info(p).bonus < muffle) {
            muffle = part_info(p).bonus;
        }
    }

    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "ENGINE") && parts[p].active() &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            int nc = 10;

            if( part_info(p).fuel_type == "gasoline" )    nc = 25;
            else if( part_info(p).fuel_type == "plasma" ) nc = 10;
            else if( part_info(p).fuel_type == "battery" )   nc = 3;
            else if( part_info(p).fuel_type == "muscle" ) nc = 5;

            if (!gas_only || part_info(p).fuel_type == "gasoline")
            {
                int pwr = part_power(p) * nc / 100;
                if (muffle < 100 && (part_info(p).fuel_type == "gasoline" ||
                    part_info(p).fuel_type == "plasma"))
                    pwr = pwr * muffle / 100;
                pwrs += pwr;
                cnt++;
            }
        }
    }
    return pwrs;
}

float vehicle::wheels_area (int *cnt)
{
    int count = 0;
    int total_area = 0;
    std::vector<int> wheel_indices = all_parts_with_feature("WHEEL");
    for (int i = 0; i < wheel_indices.size(); i++)
    {
        int p = wheel_indices[i];
        int width = part_info(p).wheel_width;
        int bigness = parts[p].bigness;
        // 9 inches, for reference, is about normal for cars.
        total_area += ((float)width/9) * bigness;
        count++;
    }
    if (cnt) {
        *cnt = count;
    }
    return total_area;
}

float vehicle::k_dynamics ()
{
    const int max_obst = 13;
    int obst[max_obst];
    for (int o = 0; o < max_obst; o++) {
        obst[o] = 0;
    }
    std::vector<int> structure_indices = all_parts_at_location("structure");
    for (int i = 0; i < structure_indices.size(); i++)
    {
        int p = structure_indices[i];
        int frame_size = part_with_feature(p, "OBSTACLE") ? 30 : 10;
        int pos = parts[p].mount_dy + max_obst / 2;
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
    for (int o = 0; o < max_obst; o++) {
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

float vehicle::k_mass ()
{
    float wa = wheels_area();
    if (wa <= 0)
       return 0;

    float ma0 = 50.0;

    // calculate safe speed reduction due to mass
    float km = ma0 / (ma0 + (total_mass()/8) / (8 * (float) wa));

    return km;
}

float vehicle::strain ()
{
    int mv = max_velocity();
    int sv = safe_velocity();
    if (mv <= sv)
        mv = sv + 1;
    if (velocity < safe_velocity())
        return 0;
    else
        return (float) (velocity - sv) / (float) (mv - sv);
}

bool vehicle::valid_wheel_config ()
{
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int count = 0;
    std::vector<int> wheel_indices = all_parts_with_feature("WHEEL");
    if(wheel_indices.size() == 0) {
        //No wheels!
        return false;
    } else if(wheel_indices.size() == 1) {
        //Has to be a stable wheel
        if(part_info(wheel_indices[0]).has_flag("STABLE")) {
            //Valid only if the vehicle is 1 square in size (1 structural part)
            return (all_parts_at_location("structure").size() == 1);
        } else {
            return false;
        }
    }
    for (int w = 0; w < wheel_indices.size(); w++) {
        int p = wheel_indices[w];
        if (!count) {
            x1 = x2 = parts[p].mount_dx;
            y1 = y2 = parts[p].mount_dy;
        }
        if (parts[p].mount_dx < x1) {
            x1 = parts[p].mount_dx;
        }
        if (parts[p].mount_dx > x2) {
            x2 = parts[p].mount_dx;
        }
        if (parts[p].mount_dy < y1) {
            y1 = parts[p].mount_dy;
        }
        if (parts[p].mount_dy > y2) {
            y2 = parts[p].mount_dy;
        }
        count++;
    }
    float xo = 0, yo = 0;
    float wo = 0, w2;
    for (int p = 0; p < parts.size(); p++)
    { // lets find vehicle's center of masses
        w2 = itypes[part_info(p).item]->weight;
        if (w2 < 1)
            continue;
        xo = xo * wo / (wo + w2) + parts[p].mount_dx * w2 / (wo + w2);
        yo = yo * wo / (wo + w2) + parts[p].mount_dy * w2 / (wo + w2);
        wo += w2;
    }
//    g->add_msg("cm x=%.3f y=%.3f m=%d  x1=%d y1=%d x2=%d y2=%d", xo, yo, (int) wo, x1, y1, x2, y2);
    if ((int)xo < x1 || (int)xo > x2 || (int)yo < y1 || (int)yo > y2) {
        return false; // center of masses not inside support of wheels (roughly)
    }
    return true;
}

void vehicle::consume_fuel ()
{
    ammotype ftypes[3] = { "gasoline", "battery", "plasma" };
    for (int ft = 0; ft < 3; ft++)
    {
        float st = strain() * 10;
        int amnt = (int) (basic_consumption (ftypes[ft]) * (1.0 + st * st) /
                          (ftypes[ft] == "battery" ? 1 : 100));
        if (!amnt)
            continue; // no engines of that type
        bool elec = ftypes[ft] == "battery";
        bool found = false;
        for (int j = 0; j < (elec? 2 : 1); j++)
        {
            for (int p = 0; p < parts.size(); p++)
                // if this is a fuel tank
                // and its type is same we're looking for now
                // and for electric engines:
                //  - if j is 0, then we're looking for plutonium (it's first)
                //  - otherwise we're looking for batteries (second)
                if (part_flag(p, "FUEL_TANK") &&
                    (part_info(p).fuel_type == (elec? (j ? "battery" : "plutonium") : ftypes[ft])) &&
                    parts[p].amount > 0)
                {
                    parts[p].amount -= amnt / ((elec && !j)?100:1);
                    if (parts[p].amount < 0)
                        parts[p].amount = 0;
                    found = true;
                    break;
                }
            if (found)
                break;
        }
    }
}

void vehicle::power_parts ()//TODO: more categories of powered part!
{
    int power=0;
    if(one_in(6)) {   // Use power at the same rate the engine makes power.
      if(lights_on)power += lights_power;
      if(overhead_lights_on)power += overhead_power;
      if(tracking_on)power += tracking_power;
      if(fridge_on) power += fridge_power;
      if(power <= 0)return;
      for(int f=0;f<fuel.size() && power > 0;f++)
      {
          if(part_info(fuel[f]).fuel_type == "battery")
          {
              if(parts[fuel[f]].amount < power)
              {
                  power -= parts[fuel[f]].amount;
                  parts[fuel[f]].amount = 0;
              }
              else
              {
                  parts[fuel[f]].amount -= power;
                  power = 0;
              }
          }
      }
    }
    if(power)
    {
        lights_on = false;
        tracking_on = false;
        overhead_lights_on = false;
        fridge_on = false;
        if(player_in_control(&g->u) || g->u_see(global_x(), global_y()) )
            g->add_msg("The %s's battery dies!",name.c_str());
    }
}

void vehicle::charge_battery (int amount)
{
    for(int f=0;f<fuel.size() && amount > 0;f++)
    {
        if(part_info(fuel[f]).fuel_type == "battery")
        {
            int empty = part_info(fuel[f]).size - parts[fuel[f]].amount;
            if(empty < amount)
            {
                amount -= empty;
                parts[fuel[f]].amount = part_info(fuel[f]).size;
            }
            else
            {
                parts[fuel[f]].amount += amount;
                amount = 0;
            }
        }
    }
}


void vehicle::idle() {
  if (engine_on && total_power () > 0) {
    if(one_in(6)) {
        int strn = (int) (strain () * strain() * 100);

        for (int p = 0; p < parts.size(); p++)
        {
          if (part_flag(p, "ENGINE") && parts[p].active())
          {
              //Charge the battery if the engine has an alternator
              if(part_flag(p,"ALTERNATOR")) {
                  charge_battery(part_info(p).power);
              }
              if(fuel_left(part_info(p).fuel_type, true) && parts[p].hp > 0 && rng (1, 100) < strn)
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
      consume_fuel();
      int sound = noise()/10 + 2;
      g->sound(global_x(), global_y(), sound, "hummm.");

      if (one_in(10)) {
        int smk = noise (true, true); // Only generate smoke for gas cars.
        if (smk > 0) {
          int rdx = rng(0, 2);
          int rdy = rng(0, 2);
          g->m.add_field(g, global_x() + rdx, global_y() + rdy, fd_smoke, (sound / 50) + 1);
        }
      }
    }
  }
  else {
    if (g->u_see(global_x(), global_y()) && engine_on)
        g->add_msg(_("The engine dies!"));
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
                if (total_power (false) < 1)
                    g->add_msg (_("The %s doesn't have an engine!"), name.c_str());
                else
                    g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
            }
            cruise_velocity = 0;
            return;
        }
        else if (!engine_on) {
          g->add_msg (_("The %s's engine isn't on!"), name.c_str());
          cruise_velocity = 0;
          return;
        }

        consume_fuel ();

        int strn = (int) (strain () * strain() * 100);

        for (int p = 0; p < parts.size(); p++)
        {
            if (part_flag(p, "ENGINE") && parts[p].active())
            {
                //Charge the battery if the engine has an alternator
                if(part_flag(p,"ALTERNATOR")) {
                    charge_battery(part_info(p).power);
                }
                if(fuel_left(part_info(p).fuel_type, true) && parts[p].hp > 0 && rng (1, 100) < strn)
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
        if (smk > 0)
        {
            int rdx, rdy;
            coord_translate (exhaust_dx, exhaust_dy, rdx, rdy);
            g->m.add_field(g, global_x() + rdx, global_y() + rdy, fd_smoke, (smk / 50) + 1);
        }
        std::string soundmessage;
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

bool vehicle::collision( std::vector<veh_collision> &veh_veh_colls, int dx, int dy,
                         bool &can_move, int &imp, bool just_detect )
{
    std::vector<int> structural_indices = all_parts_at_location("structure");
    for( int i = 0; i < structural_indices.size() && can_move; i++ ) {
        const int p = structural_indices[i];
        // coords of where part will go due to movement (dx/dy)
        // and turning (precalc_dx/dy [1])
        const int dsx = global_x() + dx + parts[p].precalc_dx[1];
        const int dsy = global_y() + dy + parts[p].precalc_dy[1];
        veh_collision coll = part_collision( p, dsx, dsy, just_detect );
        if( coll.type != veh_coll_nothing && just_detect ) {
            return true;
        } else if( coll.type == veh_coll_veh ) {
            veh_veh_colls.push_back( coll );
        } else if( coll.type != veh_coll_nothing ) { //run over someone?
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

veh_collision vehicle::part_collision (int part, int x, int y, bool just_detect)
{
    bool pl_ctrl = player_in_control (&g->u);
    int mondex = g->mon_at(x, y);
    int npcind = g->npc_at(x, y);
    bool u_here = x == g->u.posx && y == g->u.posy && !g->u.in_vehicle;
    monster *z = mondex >= 0? &g->zombie(mondex) : 0;
    player *ph = (npcind >= 0? g->active_npc[npcind] : (u_here? &g->u : 0));

    // if in a vehicle assume it's this one
    if (ph && ph->in_vehicle) {
        ph = 0;
    }

    int target_part = -1;
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
       ret.part = part;
       ret.target = oveh;
       ret.target_part = target_part;
       ret.target_name = oveh->name.c_str();
       return ret;
    }

    //Damage armor before damaging any other parts
    int parm = part_with_feature (part, "ARMOR");
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
    } else if (g->m.move_cost_ter_furn(x, y) == 0 && g->m.is_destructable_ter_furn(x, y)) {
        collision_type = veh_coll_destructable; // destructible (wall)
        mass2 = 200;
        e=0.30;
        part_dens = 60;
    } else if (g->m.move_cost_ter_furn(x, y) == 0 && !g->m.has_flag_ter_or_furn("SWIMMABLE", x, y)) {
        collision_type = veh_coll_other; // not destructible
        mass2 = 1000;
        e=0.10;
        part_dens = 80;
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
                    g->m.destroy(g, x, y, false);
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
                z->add_effect(ME_STUNNED, turns_stunned);
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
                ph->hitall (g, dam, 40);
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
            g->m.adjust_field_strength(g, point(x, y), fd_blood, 1 );
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
    ret.type = collision_type;
    ret.imp = part_dmg;
    return ret;
}

void vehicle::handle_trap (int x, int y, int part)
{
    int pwh = part_with_feature (part, "WHEEL");
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
int vehicle::stored_volume(int part) {
   if (!part_flag(part, "CARGO")) {
        return 0;
   }
   int cur_volume = 0;
   for (int i = 0; i < parts[part].items.size(); i++) {
       cur_volume += parts[part].items[i].volume();
   }
   return cur_volume;
}

int vehicle::max_volume(int part) {
    if (part_flag(part, "CARGO")) {
        return vehicle_part_types[parts[part].id].size;
    }
    return 0;
}

// free space
int vehicle::free_volume(int part) {
   const int maxvolume = this->max_volume(part);
   return ( maxvolume - stored_volume(part) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool vehicle::is_full(const int part, const int addvolume, const int addnumber) {
   const int maxitems = MAX_ITEM_IN_VEHICLE_STORAGE;
   const int maxvolume = this->max_volume(part);

   if ( addvolume == -1 ) {
       if ( parts[part].items.size() < maxitems ) return true;
       int cur_volume=stored_volume(part);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( parts[part].items.size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(part);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

bool vehicle::add_item (int part, item itm)
{
    const int max_storage = MAX_ITEM_IN_VEHICLE_STORAGE; // (game.h)
    const int maxvolume = this->max_volume(part);         // (game.h => vehicle::max_volume(part) ) in theory this could differ per vpart ( seat vs trunk )

    // const int max_weight = ?! // TODO: weight limit, calc per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certian point, or when hitting bumps etc

    if (!part_flag(part, "CARGO")) {
        return false;
    }

    if (parts[part].items.size() >= max_storage)
        return false;
    it_ammo *ammo = dynamic_cast<it_ammo*> (itm.type);
    if (part_flag(part, "TURRET")) {
        if (!ammo || (ammo->type != part_info(part).fuel_type ||
                 ammo->type == "gasoline" ||
                 ammo->type == "plasma")) {
            return false;
        }
    }
    int cur_volume = 0;
    int add_volume = itm.volume();
    bool tryaddcharges=(itm.charges  != -1 && (itm.is_food() || itm.is_ammo()));
    // iterate anyway since we need a volume total
      for (int i = 0; i < parts[part].items.size(); i++) {
        cur_volume += parts[part].items[i].volume();
        if( tryaddcharges && parts[part].items[i].type->id == itm.type->id ) {
          parts[part].items[i].charges+=itm.charges;
          return true;
        }
      }

    if ( cur_volume + add_volume > maxvolume ) {
      return false;
    }
    parts[part].items.push_back (itm);
    return true;
}

void vehicle::remove_item (int part, int itemdex)
{
    if (itemdex < 0 || itemdex >= parts[part].items.size())
        return;
    parts[part].items.erase (parts[part].items.begin() + itemdex);
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

void fridge_function(std::vector<item> &list, int duration) {
    for(size_t i = 0; i < list.size(); i++) {
        item &it = list[i];
        // this works for all kind of items even non-food
        it.bday += duration;
        if(it.item_tags.count("HOT") > 0) {
            it.item_tags.erase(it.item_tags.find("HOT"));
        }
        fridge_function(it.contents, duration);
    }
}

void vehicle::gain_moves (int mp)
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

    refill ("battery", solar_power());
    
    int rain_chance = 0;
    bool acid_rain = false;
    if(g->weather == WEATHER_DRIZZLE) {
        rain_chance = g->traps[tr_funnel]->funnel_turns_per_charge(4);
    } else if(g->weather >= WEATHER_RAINY && g->weather <= WEATHER_LIGHTNING) {
        rain_chance = g->traps[tr_funnel]->funnel_turns_per_charge(8);
    } else if(g->weather == WEATHER_ACID_DRIZZLE) {
        rain_chance = g->traps[tr_funnel]->funnel_turns_per_charge(4);
        acid_rain = true;
    } else if(g->weather == WEATHER_ACID_RAIN) {
        rain_chance = g->traps[tr_funnel]->funnel_turns_per_charge(8);
        acid_rain = true;
    }

    // check for smoking parts
    for (int p = 0; p < parts.size(); p++)
    {
        vehicle_part &vp = parts[p];
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
        if(vp.hp > 0 && vp.active() && vp.amount >= 1 && part_flag(p, "ENGINE")) {
            int p_eng = p;
            parts[p_eng].amount--;
            for (int ix = -1; ix <= 1; ix++) {
                for (int iy = -1; iy <= 1; iy++) {
                    if (!rng(0, 2)) {
                        g->m.add_field(g, part_x + ix, part_y + iy, fd_smoke, rng(2, 4));
                    }
                }
            }
        }
        if(rain_chance > 0 && vp.hp > 0 && part_flag(p, "FUNNEL") && one_in(rain_chance)) {
            // Create a temporay container for interaction with item::add_rain_to_container
            item tmpcontainer(item_controller->find_template("jerrycan"), 0);
            if(!vp.items.empty()) {
                tmpcontainer.contents.push_back(vp.items[0]);
            }
            tmpcontainer.add_rain_to_container(acid_rain, 1);
            if(!tmpcontainer.contents.empty()) {
                if(tmpcontainer.contents[0].type->id == "water" && part_with_feature(p, "CRAFTRIG") >= 0) {
                    const int drained = drain("battery", tmpcontainer.contents[0].charges);
                    if(drained > 0) {
                        assert(drained <= tmpcontainer.contents[0].charges);
                        const int left_over = refill("water", drained);
                        if(left_over > 0) {
                            refill("battery", left_over);
                            tmpcontainer.contents[0].charges = left_over;
                        } else {
                            tmpcontainer.contents.clear();
                        }
                    }
                }
                if(tmpcontainer.contents.empty()) {
                    vp.items.clear();
                } else if(vp.items.empty()) {
                    vp.items.push_back(tmpcontainer.contents[0]);
                } else {
                    vp.items[0] = tmpcontainer.contents[0];
                }
            }
        }
        if(vp.hp > 0 && vp.active() && part_flag(p, "FRIDGE")) {
            // Drain energy under all circumstances
            const int fuel_drained = drain("battery", 10);
            if(fuel_drained != 10 || vp.items.empty()) {
                continue;
            }
            // turn_diff is used incases the vehicle got left the reality-bubble
            // in that case the turn_diff will be > 1
            int turn_diff;
            if(vp.amount == 0) {
                // Init to previous turn
                vp.amount = ((int) g->turn) - 1;
                turn_diff = 1;
            } else {
                turn_diff = ((int) g->turn) - vp.amount;
                vp.amount = (int) g->turn;
                if(turn_diff < 10) {
                    if(one_in(10)) {
                        // Ups did not work in 10% of all turns -> aging still happens
                        continue;
                    }
                } else if(turn_diff >= 10) {
                    turn_diff = (turn_diff * 9) / 10; // Age slower but still age
                }
            }
            // Food still gets bad, but slower:
            // fridge works only 90% of the time
            fridge_function(parts[p].items, turn_diff);
        }
        if (turret_mode) { // handle turrets
            fire_turret (p);
        }
    }
}

void vehicle::find_horns ()
{
    horns.clear();
    for (int p = 0; p < parts.size(); p++) {
        if(part_flag( p,"HORN" )) {
            horns.push_back(p);
        }
    }
}

void vehicle::find_power ()
{
    lights.clear();
    lights_power = 0;
    overhead_power = 0;
    tracking_power = 0;
    fridge_power = 0;
    for (int p = 0; p < parts.size(); p++) {
        if(part_flag(parts.size()-1,"LIGHT") || part_flag(parts.size()-1,"CONE_LIGHT")) {
            lights.push_back(parts.size()-1);
            lights_power += part_info(parts.size()-1).power;
        }
        if (part_flag(parts.size()-1,"CIRCLE_LIGHT")) {
            overhead_power += part_info(parts.size()-1).power;
        }
        if(part_flag(parts.size()-1, "TRACK")) {
            tracking_power += part_info(parts.size()-1).power;
        }
        if(part_flag(parts.size()-1, "FRIDGE")) {
            fridge_power += part_info(parts.size()-1).power;
        }
    }
}

void vehicle::find_fuel_tanks ()
{
    fuel.clear();
    for (int p = 0; p < parts.size(); p++) {
        if(part_flag( p,"FUEL_TANK" )) {
            fuel.push_back(p);
        }
    }
}

void vehicle::find_exhaust ()
{
    int en = -1;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "ENGINE") && part_info(p).fuel_type == "gasoline" && parts[p].active()) {
            en = p;
            break;
        }
    }
    if (en < 0) {
        exhaust_dy = 0;
        exhaust_dx = 0;
        return;
    }
    exhaust_dy = parts[en].mount_dy;
    exhaust_dx = parts[en].mount_dx;
    for (int p = 0; p < parts.size(); p++) {
        if (parts[p].mount_dy == exhaust_dy &&
            parts[p].mount_dx < exhaust_dx) {
            exhaust_dx = parts[p].mount_dx;
        }
    }
    exhaust_dx--;
}

void vehicle::refresh_insides ()
{
    insides_dirty = false;
    for (int p = 0; p < parts.size(); p++) {
        /* If there's no roof, or there is a roof but it's broken, it's outside.
         * (Use short-circuiting && so broken frames don't screw this up) */
        if ( !(part_with_feature(p, "ROOF") >= 0 && parts[p].hp > 0) ) {
            parts[p].inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for (int i = 0; i < 4; i++) { // let's check four neighbour parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (parts[p].mount_dx + ndx,
                                                             parts[p].mount_dy + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (int j = 0; j < parts_n3ar.size(); j++) {
                int pn = parts_n3ar[j];
                if (part_flag(pn, "ROOF") && parts[pn].hp > 0) { // another roof -- cover
                    cover = true;
                    break;
                }
                else
                if (part_flag(pn, "OBSTACLE") && parts[pn].hp > 0) {
                    // found an obstacle, like board or windshield or door
                    if (parts[pn].inside || (part_flag(pn, "OPENABLE") && parts[pn].open)) {
                        continue; // door and it's open -- can't cover
                    }
                    cover = true;
                    break;
                }
                //Otherwise keep looking, there might be another part in that square
            }
            if (!cover) {
                parts[p].inside = false;
                break;
            }
        }
    }
}

bool vehicle::is_inside (int p)
{
    if (p < 0 || p >= parts.size()) {
        return false;
    }
    if (insides_dirty) {
        refresh_insides ();
    }
    return parts[p].inside;
}

void vehicle::unboard_all ()
{
    std::vector<int> bp = boarded_parts ();
    for (int i = 0; i < bp.size(); i++) {
        g->m.unboard_vehicle (global_x() + parts[bp[i]].precalc_dx[0],
                              global_y() + parts[bp[i]].precalc_dy[0]);
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
        for (int i = 0; i < pl.size(); i++)
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
        dres = damage_direct (parm, dmg, type);
        // half damage for internal part(over parts not covered)
        bool overhead = part_flag(pdm, "ROOF") ||
                        part_info(pdm).location == "on_roof";
        damage_direct (pdm, overhead ? dmg : dmg / 2, type);
    }
    return dres;
}

void vehicle::damage_all (int dmg1, int dmg2, int type, const point &impact)
{
    if (dmg2 < dmg1) { std::swap(dmg1, dmg2); }
    if (dmg1 < 1) { return; }
    if(g->u.controlling_vehicle && g->m.veh_at(g->u.posx, g->u.posy) == this) {
        g->add_msg("Your %s takes %d-%d damage", name.c_str(), dmg1, dmg2);
    }
    for (int p = parts.size() - 1; p >= 0; p--) {
        int distance = 1 + square_dist( parts[p].mount_dx, parts[p].mount_dy, impact.x, impact.y );
        if( distance > 1 && one_in( distance ) ) {
            damage_direct (p, rng( dmg1, dmg2 ) / (distance * distance), type);
        }
    }
}

int vehicle::damage_direct (int p, int dmg, int type)
{
    if (parts[p].hp <= 0) {
        /* Already-destroyed part - chance it could be torn off into pieces.
         * Chance increases with damage, and decreases with part max durability
         * (so lights, etc are easily removed; frames and plating not so much) */
        if(rng(0, part_info(p).durability / 10) < dmg) {
            int x_pos = global_x() + parts[p].precalc_dx[0];
            int y_pos = global_y() + parts[p].precalc_dy[0];
            if(part_info(p).location == "structure") {
                //For structural parts, remove other parts first
                std::vector<int> parts_in_square = parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
                for(int index = parts_in_square.size() - 1; index >= 0; index--) {
                    //Ignore the frame being destroyed
                    if(parts_in_square[index] != p) {
                        if(g->u_see(x_pos, y_pos)) {
                            g->add_msg(_("The %s's %s is torn off!"), name.c_str(),
                                    part_info(parts_in_square[index]).name.c_str());
                        }
                        item part_as_item = item_from_part(parts_in_square[index]);
                        g->m.add_item_or_charges(x_pos, y_pos, part_as_item, true);
                        remove_part(parts_in_square[index]);
                    }
                    /* After clearing the frame, remove it if normally legal to
                     * do so (it's not (0, 0) and not holding the vehicle
                     * together). At a later date, some more complicated system
                     * (such as actually making two vehicles from the split
                     * parts) would be ideal. */
                    if(can_unmount(p)) {
                        if(g->u_see(x_pos, y_pos)) {
                            g->add_msg(_("The %s's %s is destroyed!"),
                                    name.c_str(), part_info(p).name.c_str());
                        }
                        break_part_into_pieces(p, x_pos, y_pos, true);
                        remove_part(p);
                    }
                }
            } else {
                //Just break it off
                if(g->u_see(x_pos, y_pos)) {
                    g->add_msg(_("The %s's %s is destroyed!"),
                                    name.c_str(), part_info(p).name.c_str());
                }
                break_part_into_pieces(p, x_pos, y_pos, true);
                remove_part(p);
            }
            insides_dirty = true;
        }
        return dmg;
    }

    int tsh = part_info(p).durability / 10;
    if (tsh > 20) {
        tsh = 20;
    }
    int dres = dmg;
    if (dmg >= tsh || type != 1)
    {
        dres -= parts[p].hp;
        int last_hp = parts[p].hp;
        parts[p].hp -= dmg;
        if (parts[p].hp < 0)
            parts[p].hp = 0;
        if (!parts[p].hp && last_hp > 0)
            insides_dirty = true;
        if(last_hp > 0 && g->u.controlling_vehicle && g->m.veh_at(g->u.posx, g->u.posy) == this) {
            g->add_msg("Your %s takes %d damage", part_info(p).name.c_str(), dmg);
        }
        if (part_flag(p, "FUEL_TANK"))
        {
            ammotype ft = part_info(p).fuel_type;
            if (ft == "gasoline" || ft == "plasma")
            {
                int pow = parts[p].amount / 40;
    //            debugmsg ("damage check dmg=%d pow=%d", dmg, pow);
                if (parts[p].hp <= 0)
                    leak_fuel (p);
                if (type == 2 ||
                    (one_in (ft == "gasoline" ? 2 : 4) && pow > 5 && rng (75, 150) < dmg))
                {
                    g->u.add_memorial_log(_("The fuel tank of the %s exploded!"), name.c_str());
                    g->explosion (global_x() + parts[p].precalc_dx[0], global_y() + parts[p].precalc_dy[0],
                                pow, 0, ft == "gasoline");
                    parts[p].hp = 0;
                }
            }
        }
        else
        if (parts[p].hp <= 0 && part_flag(p, "UNMOUNT_ON_DAMAGE"))
        {
            g->m.spawn_item(global_x() + parts[p].precalc_dx[0],
                           global_y() + parts[p].precalc_dy[0],
                           part_info(p).item, 1, 0, g->turn);
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
    ammotype ft = part_info(p).fuel_type;
    if (ft == "gasoline")
    {
        int x = global_x();
        int y = global_y();
        for (int i = x - 2; i <= x + 2; i++)
            for (int j = y - 2; j <= y + 2; j++)
                if (g->m.move_cost(i, j) > 0 && one_in(2))
                {
                    if (parts[p].amount < 100)
                    {
                        parts[p].amount = 0;
                        return;
                    }
                    g->m.spawn_item(i, j, "gasoline");
                    g->m.spawn_item(i, j, "gasoline");
                    parts[p].amount -= 100;
                }
    }
    parts[p].amount = 0;
}

void vehicle::fire_turret (int p, bool burst)
{
    if (!part_flag (p, "TURRET") || parts[p].inactive())
        return;
    it_gun *gun = dynamic_cast<it_gun*> (itypes[part_info(p).item]);
    if (!gun)
        return;
    int charges = burst? gun->burst : 1;
    if (!charges)
        charges = 1;
    ammotype amt = part_info (p).fuel_type;
    if (amt == "gasoline" || amt == "plasma")
    {
        if (amt == "gasoline")
            charges = 20; // hacky
        int fleft = fuel_left (amt);
        if (fleft < 1)
            return;
        it_ammo *ammo = dynamic_cast<it_ammo*>(itypes[amt == "gasoline" ? "gasoline" : "plasma"]);
        if (!ammo)
            return;
        if (fire_turret_internal (p, *gun, *ammo, charges))
        { // consume fuel
            if (amt == "plasma")
                charges *= 10; // hacky, too
            for (int p = 0; p < parts.size(); p++)
            {
                if (part_flag(p, "FUEL_TANK") &&
                    part_info(p).fuel_type == amt &&
                    parts[p].amount > 0)
                {
                    parts[p].amount -= charges;
                    if (parts[p].amount < 0)
                        parts[p].amount = 0;
                }
            }
        }
    } else {
        if (parts[p].items.size() > 0) {
            it_ammo *ammo = dynamic_cast<it_ammo*> (parts[p].items[0].type);
            if (!ammo || ammo->type != amt || parts[p].items[0].charges < 1) {
                return;
            }
            if (charges > parts[p].items[0].charges) {
                charges = parts[p].items[0].charges;
            }
            if (fire_turret_internal (p, *gun, *ammo, charges)) {
                // consume ammo
                if (charges >= parts[p].items[0].charges) {
                    parts[p].items.erase (parts[p].items.begin());
                } else {
                    parts[p].items[0].charges -= charges;
                }
            }
        }
    }
}

bool vehicle::fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, int charges)
{
    int x = global_x() + parts[p].precalc_dx[0];
    int y = global_y() + parts[p].precalc_dy[0];
    // code copied form mattack::smg, mattack::flamethrower
    int t, fire_t;
    monster *target = 0;
    int range = ammo.type == "gasoline" ? 5 : 12;
    int closest = range + 1;
    for (int i = 0; i < g->num_zombies(); i++) {
        int dist = rl_dist( x, y, g->zombie(i).posx(), g->zombie(i).posy() );
        if (g->zombie(i).friendly == 0 && dist < closest &&
              !g->zombie(i).is_hallucination() &&
              g->m.sees(x, y, g->zombie(i).posx(), g->zombie(i).posy(), range, t) ) {
            target = &(g->zombie(i));
            closest = dist;
            fire_t = t;
        }
    }
    if( !target ) {
        return false;
    }

    std::vector<point> traj = line_to( x, y, target->posx(), target->posy(), fire_t );
    for( int i = 0; i < traj.size(); i++ ) {
        if( traj[i].x == g->u.posx && traj[i].y == g->u.posy ) {
            return false; // won't shoot at player
        }
    }

    // Check for available power for turrets that use it.
    const int power = fuel_left("battery");
    if( gun.item_tags.count( "USE_UPS" ) ) {
        if( power < 5 ) { return false; }
    } else if( gun.item_tags.count( "USE_UPS_20" ) ) {
        if( power < 20 ) { return false; }
    } else if( gun.item_tags.count( "USE_UPS_40" ) ) {
        if( power < 40 ) { return false; }
    }
    if( g->u_see(x, y) ) {
        g->add_msg(_("The %s fires its %s!"), name.c_str(), part_info(p).name.c_str());
    }
    npc tmp;
    tmp.name = rmp_format(_("<veh_player>The %s"), part_info(p).name.c_str());
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
    // Spawn a fake UPS to power any turreted weapons that need electricity.
    item tmp_ups( itypes["UPS_on"], 0 );
    // Drain a ton of power
    tmp_ups.charges = drain( "battery", 1000 );
    item &ups_ref = tmp.i_add(tmp_ups);
    g->fire( tmp, target->posx(), target->posy(), traj, true );
    // Rturn whatever is left.
    refill( "battery", ups_ref.charges );
    if( ammo.type == "gasoline" ) {
        for( int i = 0; i < traj.size(); i++ ) {
            g->m.add_field(g, traj[i].x, traj[i].y, fd_fire, 1);
        }
    }

    return true;
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::open(int part_index)
{
  if(!part_info(part_index).has_flag("OPENABLE")) {
    debugmsg("Attempted to open non-openable part %d (%s) on a %s!", part_index,
               vehicle_part_types[parts[part_index].id].name.c_str(), name.c_str());
  } else {
    open_or_close(part_index, true);
  }
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::close(int part_index)
{
  if(!part_info(part_index).has_flag("OPENABLE")) {
    debugmsg("Attempted to close non-closeable part %d (%s) on a %s!", part_index,
               vehicle_part_types[parts[part_index].id].name.c_str(), name.c_str());
  } else {
    open_or_close(part_index, false);
  }
}

void vehicle::open_or_close(int part_index, bool opening)
{
    if(!opening) {
        const int x = global_x() + parts[part_index].precalc_dx[0];
        const int y = global_y() + parts[part_index].precalc_dy[0];
        if(g->npc_at(x, y) != -1 || g->mon_at(x, y) != -1 || (g->u.posx == x && g->u.posy == y)) {
            g->add_msg("There is someone in that door");
            return;
        }
    }
    
  parts[part_index].open = opening ? 1 : 0;
  insides_dirty = true;

  if(part_info(part_index).has_flag("MULTISQUARE")) {
    /* Find all other closed parts with the same ID in adjacent squares.
     * This is a tighter restriction than just looking for other Multisquare
     * Openable parts, and stops trunks from opening side doors and the like. */
    for(int next_index = 0; next_index < parts.size(); next_index++) {
      //Look for parts 1 square off in any cardinal direction
      int xdiff = parts[next_index].mount_dx - parts[part_index].mount_dx;
      int ydiff = parts[next_index].mount_dy - parts[part_index].mount_dy;
      if(std::abs(xdiff) <= 1 && std::abs(ydiff) <= 1 &&
              (part_info(next_index).id == part_info(part_index).id) &&
              (parts[next_index].open == opening ? 0 : 1)) {
        open_or_close(next_index, opening);
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




bool vehicle_part::inactive() const {
    return flags & mfb(INACTIVE);
}

bool vehicle_part::active() const {
    return !inactive();
}

#include "veh_type.h"
#include "game.h"

void add_item(const itype *type, const char *mat, const char *rep_item, double amount, vpart_info::type_count_pair_vector &result) {
    // If the item is made if mat, add rep_item to the list
    // of repair-items.
    // Add more if the m1 is the material, add less if m2 is the material
    const itype *rep_it = itypes[rep_item];
    if(rep_it == 0 || rep_it->name == "null") {
        return;
    }
    // Example: item of vehicle part is a steel frame,
    // damage is 20 % -> amount = <weight-of-steel-frame> * 0.2
    // Repair-item is steel_chunk, therefor
    // item_count = <weight-of-steel-frame> * 0.2 / <weight-of-chunk>
    // = 240 * 0.2 / 6 = 8
    int item_count = static_cast<int>(amount / rep_it->weight);
    if((type->m2 == "" || type->m2 == "null") && type->m1 == mat) {
        if(item_count >= 1) {
            result.push_back(vpart_info::type_count_pair(rep_item, item_count));
        }
    } else if(type->m1 == mat) { // 66 % m1 and 33% m2
        item_count = (item_count * 2) / 3;
        if(item_count >= 1) {
            result.push_back(vpart_info::type_count_pair(rep_item, item_count));
        }
    } else if(type->m2 == mat) {
        item_count = item_count / 3;
        if(item_count >= 1) {
            result.push_back(vpart_info::type_count_pair(rep_item, item_count));
        }
    }
}

std::vector<item> vpart_info::get_remaining_scraps() const {
    std::vector< ::item> result;
    const int hp = durability / 500;
    if(hp <= 0) {
        return result;
    }
    type_count_pair_vector ff = get_repair_materials(hp);
    for(size_t i = 0; i < ff.size(); i++) {
        for(size_t j = 0; j < ff[i].second; j++) {
            result.push_back(::item(itypes[ff[i].first], (int) g->turn));
        }
    }
    return result;
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
    const itype *type = itypes[item];
    if(type == 0 || type->id == "null") {
        return result;
    }
    // relative amount of damage
    const double rel_damage = static_cast<double>(hp_to_repair) / durability;
    // amount (as weight) of damage that must be repaired
    const double amount = rel_damage * type->weight;
    // material     item-type for repair   relative-damage
    ::add_item(type, "steel", "steel_lump", amount, result);
    if(result.empty()) {
        // Try again, this time use the smaller chunks, this is for
        // when the damage is to small to require a full lump
        ::add_item(type, "steel", "steel_chunk", amount, result);
    }
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
    ::add_item(type, "kevlar", "kevlar_plate", amount, result);
    ::add_item(type, "chitin", "chitin_piece", amount, result);
    ::add_item(type, "bone", "bone", amount, result);
    return result;
}

bool vehicle::is_single_tile() const {
    if(parts.empty()) {
        // It's a 0-tile vehicle
        return false;
    }
    for(size_t i = 0; i < parts.size(); i++) {
        if(parts[i].mount_dx != 0 || parts[i].mount_dy != 0) {
            return false;
        }
    }
    return true;
}

bool vehicle::is_or_might_move() const {
    return velocity != 0 || cruise_velocity != 0;
}

bool vehicle::can_tow(game *g, vehicle *&other, int &other_part) const {
    if(!is_single_tile() || is_or_might_move()) {
        // Can only tow single tile non-moving vehicles
        return false;
    }
    const int gx = const_cast<vehicle*>(this)->global_x() + parts[0].precalc_dx[0];
    const int gy = const_cast<vehicle*>(this)->global_y() + parts[0].precalc_dy[0];
    if((other = g->m.veh_at(gx, gy + 1, other_part)) != 0 && !other->is_or_might_move()) { return true; }
    if((other = g->m.veh_at(gx + 1, gy, other_part)) != 0 && !other->is_or_might_move()) { return true; }
    if((other = g->m.veh_at(gx - 1, gy, other_part)) != 0 && !other->is_or_might_move()) { return true; }
    if((other = g->m.veh_at(gx, gy - 1, other_part)) != 0 && !other->is_or_might_move()) { return true; }
    return false;
}

bool vehicle::tow_to(game *g, vehicle *other, int other_part, player *p) {
    static const itype_id rope_type("rope_30");
    assert(g != 0);
    assert(other != 0);
    assert(other_part >= 0);
    assert(p != 0);
    if(!p->inv.has_amount(rope_type, 1)) {
        popup(_("You need a %s to do two vehicles together!"), item_controller->find_template(rope_type)->name.c_str());
        return false;
    }
    std::list<item> rope = p->inv.use_amount(rope_type, 1);
    assert(!rope.empty());
    // Store the used rope
    parts[0].items.push_back(rope.front());
    // Store name of this vehicle
    parts[0].items[0].mode = name;
    // Global coords of this vehicle
    const int x = global_x() + parts[0].precalc_dx[0] - other->global_x();
    const int y = global_y() + parts[0].precalc_dy[0] - other->global_y();
    // Get the local cooridinates of the new parts inside the other vehicle
    const int px = other->parts[other_part].mount_dx;
    const int py = other->parts[other_part].mount_dy;
    
    int nx, ny;
    int ox = px + 1, oy = py;
    other->coord_translate(ox, oy, nx, ny);
    if(nx != x || ny != y) { ox = px; oy = py + 1; other->coord_translate(ox, oy, nx, ny); }
    if(nx != x || ny != y) { ox = px; oy = py - 1; other->coord_translate(ox, oy, nx, ny); }
    if(nx != x || ny != y) { ox = px - 1; oy = py; other->coord_translate(ox, oy, nx, ny); }
    
    // Change local coords
    for(size_t i = 0; i < parts.size(); i++) {
        parts[i].mount_dx = ox;
        parts[i].mount_dy = oy;
    }
    const std::string this_name = name;
    // Move parts
    other->parts.insert(other->parts.end(), parts.begin(), parts.end());
    // Destroy me
    g->m.destroy_vehicle(this);
    // Update this vehicle, see install_part/remove_part
    other->precalc_mounts(0, other->face.dir());
    other->insides_dirty = true;
    other->sort_parts();
    g->m.update_vehicle_cache(other, false);
    p->moves -= 300;
    g->add_msg("You tow the %s to the %s", this_name.c_str(), other->name.c_str());
    return true;
}

bool vehicle::can_untow(int part) {
    static const itype_id rope_type("rope_30");
    assert(part >= 0 && part < parts.size());
    if(parts[part].items.size() != 1 || parts[part].items[0].type->id != rope_type) {
        // Not towed with a rope
        return false;
    }
    const int dx = parts[part].mount_dx;
    const int dy = parts[part].mount_dy;
    if(dx == 0 && dy == 0) {
        // Can not untow the root part
        return false;
    }
    // Check to see if all parts would still be connected
    //First, find all the squares connected to the one we're removing
    std::vector<vehicle_part> connected_parts;

    for(int i = 0; i < 4; i++) {
        int next_x = i < 2 ? (i == 0 ? -1 : 1) : 0;
        int next_y = i < 2 ? 0 : (i == 2 ? -1 : 1);
        std::vector<int> parts_over_there = parts_at_relative(dx + next_x, dy + next_y);
        //Ignore empty squares
        if(parts_over_there.size() > 0) {
            //Just need one part from the square to track the x/y
            connected_parts.push_back(parts[parts_over_there[0]]);
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
            if(!is_connected(connected_parts[0], connected_parts[next_part], parts[part])) {
                //Removing that part would break the vehicle in two
                return false;
            }
        }

    }
    return true;
}

void vehicle::untow(game *g, int part, player *p) {
    assert(g != 0);
    assert(part >= 0 && part < parts.size());
    assert(p != 0);
    assert(!parts[part].items.empty());
    const int mx = parts[part].mount_dx;
    const int my = parts[part].mount_dy;
    const int gx = global_x() + parts[part].precalc_dx[0];
    const int gy = global_y() + parts[part].precalc_dy[0];
    // Recover rope and name of the towed vehicle
    item rope = parts[part].items[0];
    parts[part].items.erase(parts[part].items.begin());
    const std::string new_veh_name = rope.mode;
    rope.mode = "";
    p->add_or_drop(rope, g);
    
    // Remove all the parts of the towed vehicle
    std::vector<vehicle_part> tmp_parts;
    for(size_t i = 0; i < parts.size(); i++) {
        if(parts[i].mount_dx == mx && parts[i].mount_dy == my) {
            parts[i].mount_dx = 0;
            parts[i].mount_dy = 0;
            tmp_parts.push_back(parts[i]);
            parts.erase(parts.begin() + i);
            i--;
        }
    }
    sort_parts();
    // Update this vehicle, see install_part/remove_part
    precalc_mounts(0, face.dir());
    insides_dirty = true;
    g->m.update_vehicle_cache(this, false);
    
    // Create the new vehicle
    vehicle *new_veh = g->m.add_vehicle (g, "custom", gx, gy, 270, 0, 0);
    if(new_veh == 0) {
        // FIXME: this will lose the parts in tmp_parts!
        debugmsg ("error constructing vehicle");
        return;
    }
    // Set name and parts, override any existing parts
    new_veh->name = new_veh_name;
    new_veh->parts.swap(tmp_parts);
    // Update the new (untowed) vehicle, see install_part/remove_part
    new_veh->precalc_mounts(0, new_veh->face.dir());
    new_veh->insides_dirty = true;
    new_veh->sort_parts();
    g->m.update_vehicle_cache(new_veh, true);
    p->moves -= 300;
    g->add_msg("You untow the %s from the %s", new_veh->name.c_str(), name.c_str());
}

#include "crafting_inventory_t.h"
#include "crafting.h"
extern void print_list(std::ostream &buffer, std::list<item> &items);

bool vehicle::examine(game *g, player *p, int part) {
    assert(part >= 0);
    vehicle_part &vp = parts[part];
    vpart_info &vpi = vehicle_part_types[vp.id];
    if(vpi.has_flag("KILN")) {
        crafting_inventory_t cinv(g, p);
        // recipe is used to check for components
        // it is automaticly copied from a recipe loaded with json
        static recipe charcoal_recipe;
        if(charcoal_recipe.components.empty()) {
            recipe *r = recipe_by_name("charcoal");
            if(r == 0) {
                // If there is no recipe for charcoal, assume this as default
                charcoal_recipe.components.resize(1);
                charcoal_recipe.components[0].push_back(component("2x4", 3));
                charcoal_recipe.time = 60000;
                charcoal_recipe.count = 1;
            } else {
                charcoal_recipe.components = r->components;
                charcoal_recipe.time = r->time;
                charcoal_recipe.count = r->count;
            }
        }
        if(vp.items.empty()) {
            if(!cinv.has_all_requirements(charcoal_recipe)) {
                std::ostringstream buffer;
                buffer << _("You need this to make charcoal:\n");
                ::list_missing_ones(buffer, charcoal_recipe);
                popup_top(buffer.str().c_str());
                return false;
            }
            if(!query_yn(_("Put items into the kiln?"))) {
                return false;
            }
            std::ostringstream buffer;
            buffer << _("You put some items into the kiln: ");
            std::list<item> items;
            cinv.gather_and_consume(charcoal_recipe, items, items);
            print_list(buffer, items);
            for(std::list<item>::iterator a = items.begin(); a != items.end(); ++a) {
                vp.items.push_back(*a);
                vp.items.back().bday = (int) g->turn;
            }
            p->moves -= 150;
            g->add_msg_if_player(p, buffer.str().c_str());
            return true;
        } else if(vp.items[0].type->id != "charcoal") {
            const int age = (int) g->turn - vp.items[0].bday;
            const int time_to_do = charcoal_recipe.time - age * 100;
            if(time_to_do > 0) {
                g->add_msg_if_player(p, "The kiln is still working (for at least %i minutes)", time_to_do / (10 * 100));
                return false;
            }
            // Done, make charcoal
            vp.items.clear();
            item newit(item_controller->find_template("charcoal"), (int) g->turn);
            if(charcoal_recipe.count > 0) {
                newit.charges *= charcoal_recipe.count;
            }
            vp.items.push_back(newit);
        }
        assert(!vp.items.empty() && vp.items[0].type->id == "charcoal");
        if(!query_yn(_("The kiln contains %s (%i) - grab it?"), vp.items[0].type->name.c_str(), vp.items[0].charges)) {
            return false;
        }
        p->add_or_drop(vp.items[0], g);
        p->moves -= 150;
        vp.items.clear();
        return true;
    }
    if(vpi.has_flag("FUNNEL")) {
        if(vp.items.empty()) {
            g->add_msg_if_player(p, "The funnel is empty");
            return false;
        }
        if(!query_yn(_("The funnel has collected %s (%i) - fill it into a container?"), vp.items[0].type->name.c_str(), vp.items[0].charges)) {
            return false;
        }
        if(!g->handle_liquid(vp.items[0], false, false)) {
            return true;
        }
        p->moves -= 150;
        vp.items.clear();
        return true;
    }
    return false;
}

void vehicle::sort_parts() {
    std::vector<vehicle_part> ptmp(parts.size());
    for(size_t i = 0; i < parts.size(); i++) {
        size_t p = static_cast<size_t>(-1);
        for(size_t j = 0; j < parts.size(); j++) {
            if(parts[j].id.empty()) {
                continue;
            }
            if(p == static_cast<size_t>(-1)) {
                p = j;
                continue;
            }
            if(parts[p].mount_dx > parts[j].mount_dx) {
                p = j;
                continue;
            }
            if(parts[p].mount_dx < parts[j].mount_dx) {
                continue;
            }
            if(parts[p].mount_dy > parts[j].mount_dy) {
                p = j;
                continue;
            }
        }
        assert(p != static_cast<size_t>(-1));
        ptmp[i] = parts[p];
        parts[p].id.clear();
    }
    parts.swap(ptmp);
    find_fuel_tanks();
    find_exhaust();
    find_horns();
    find_power();
}
