#include "veh_type.h"
#include "game.h"

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
	::add_item(type, "kevlar", "kevlar_plate", amount, result);
	::add_item(type, "chitin", "chitin_piece", amount, result);
	::add_item(type, "bone", "bone", amount, result);
	return result;
}

#if 0
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
#endif
