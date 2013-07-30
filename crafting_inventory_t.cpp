#include "crafting_inventory_t.h"
#include "game.h"
#include "bionics.h"
#include "player.h"
#include "item_factory.h"
#include "inventory.h"

/*
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "catajson.h"
#include "input.h"
#include "options.h"
#include "output.h"
#include "catacharset.h"
*/

bool crafting_inventory_t::has_amount(const std::vector<std::pair<itype_id, int> > &items) const {
	for(size_t i = 0; i < items.size(); i++) {
		if(!has_amount(items[i].first, items[i].second)) {
			return false;
		}
	}
	return true;
}

bool crafting_inventory_t::has_amount(const itype_id &it, int quantity) const
{
	return (count(it, &amount_of, quantity, S_ALL) >= quantity);
}

bool crafting_inventory_t::has_charges(const itype_id &it, int quantity) const
{
	return (count(it, &charges_of, quantity, S_ALL) >= quantity);
}

int crafting_inventory_t::amount_of(const itype_id &it) const
{
	return count(it, &amount_of, -1, S_ALL);
}

int crafting_inventory_t::charges_of(const itype_id &it) const
{
	return count(it, &charges_of, -1, S_ALL);
}

bool crafting_inventory_t::map_has_amount(const itype_id &it, int quantity) const
{
	return (count(it, &amount_of, quantity, S_MAP | S_VEHICLE) >= quantity);
}

bool crafting_inventory_t::map_has_charges(const itype_id &it, int quantity) const
{
	return (count(it, &charges_of, quantity, S_MAP | S_VEHICLE) >= quantity);
}

int crafting_inventory_t::map_amount_of(const itype_id &it) const
{
	return count(it, &amount_of, -1, S_MAP | S_VEHICLE);
}

int crafting_inventory_t::map_charges_of(const itype_id &it) const
{
	return count(it, &charges_of, -1, S_MAP | S_VEHICLE);
}

int crafting_inventory_t::amount_of(const itype_id &type, const item &it)
{
	int count = 0;
	if(it.matches_type(type))
	{
		// check if it's a container, if so, it should be empty
		if (it.type->is_container())
		{
			if (it.contents.empty())
			{
				count++;
			}
		}
		else
		{
			count++;
		}
	}
	for (int k = 0; k < it.contents.size(); k++)
	{
		count += amount_of(type, it.contents[k]);
	}
	return count;
}

int crafting_inventory_t::charges_of(const itype_id &type, const item &it)
{
    int count = 0;
	if(it.matches_type(type))
	{
		if (it.charges < 0)
		{
			count++;
		}
		else
		{
			count += it.charges;
		}
	}
	for (int k = 0; k < it.contents.size(); k++)
	{
		count += charges_of(type, it.contents[k]);
	}
    return count;
}

double crafting_inventory_t::compute_time_modi(const std::vector<component> &tools) const {
	if(tools.empty()) {
		return 1.0; // No tool needed, no influnce by tools => 1 is neutral
	}
	double best_modi = 0.0;
	for(std::vector<component>::const_iterator a = tools.begin(); a != tools.end(); a++) {
		const itype_id &type = a->type;
		std::vector<const item*> items = all_items_by_type(type);
		for(std::vector<const item*>::const_iterator b = items.begin(); b != items.end(); b++) {
			const item &it = **b;
			const double modi = it.get_functionality_time_modi(type);
			if(modi > 0.0 && (best_modi == 0.0 || best_modi > modi)) {
				best_modi = modi;
			}
		}
	}
	if(best_modi > 0.0) {
		return best_modi;
	}
	return 1.0;
}

std::vector<const item*> crafting_inventory_t::all_items_by_type(const std::vector<component> &types) const
{
    std::vector<const item*> ret;
	for(size_t k = 0; k < types.size(); k++) {
		const itype_id &type = types[k].type;
		std::vector<const item*> items = all_items_by_type(type);
		ret.insert(ret.end(), items.begin(), items.end());
	}
	return ret;
}

std::vector<const item*> crafting_inventory_t::all_items_by_type(const itype_id &type) const
{
    std::vector<const item*> ret;
	{
		std::vector<item*> tmp = const_cast<player*>(p)->inv.all_items_by_type(type);
		for(std::vector<item*>::const_iterator a = tmp.begin(); a != tmp.end(); a++) {
			ret.push_back(*a);
		}
	}
	for(std::list<item_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
		const std::vector<item> &items = *(a->items);
		for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
			if(b->made_of(LIQUID)) {
				continue;
			}
			if(b->type->id == type) {
				ret.push_back(&*b);
			}
		}
	}
	for(std::list<item_in_vehicle>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
		const std::vector<item> &items = *(a->items);
		for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
			if(b->type->id == type) {
				ret.push_back(&*b);
			}
		}
	}
	for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
		const item &it = *a;
		if(it.type->id == type) {
			ret.push_back(&it);
		}
	}
	for(std::list<item_from_souround>::const_iterator a = souround.begin(); a != souround.end(); ++a) {
		const item &it = a->it;
		if(it.type->id == type) {
			ret.push_back(&it);
		}
	}
	return ret;
}

int crafting_inventory_t::count(const itype_id &type, CountFunction func, int max, int sources) const {
	int count = 0;
	if(sources & S_PLAYER) {
		if(func == (CountFunction) &charges_of) {
			count += p->charges_of(type);
		} else if(func == (CountFunction) &amount_of) {
			count += p->amount_of(type);
		}
		if(max > 0 && count >= max) {
			return count;
		}
	}
	if(sources & S_MAP) {
		for(std::list<item_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
			const std::vector<item> &items = *(a->items);
			for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
				if(b->made_of(LIQUID)) {
					continue;
				}
				count += (*func)(type, *b);
				if(max > 0 && count >= max) {
					return count;
				}
			}
		}
	}
	if(sources & S_VEHICLE) {
		for(std::list<item_in_vehicle>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
			const std::vector<item> &items = *(a->items);
			for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
				// TODO: skip liquids in vehicles?
	//			if(b->made_of(LIQUID)) {
	//				continue;
	//			}
				count += (*func)(type, *b);
				if(max > 0 && count >= max) {
					return count;
				}
			}
		}
	}
	if(sources & S_PLAYER) {
		for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
			const item &it = *a;
			count += (*func)(type, it);
			if(max > 0 && count >= max) {
				return count;
			}
		}
	}
	if(sources & S_MAP) {
		for(std::list<item_from_souround>::const_iterator a = souround.begin(); a != souround.end(); ++a) {
			const item &it = a->it;
			count += (*func)(type, it);
			if(max > 0 && count >= max) {
				return count;
			}
		}
	}
	return count;
}

void crafting_inventory_t::form_from_map(game *g, point origin, int range) {
	for (int x = origin.x - range; x <= origin.x + range; x++) {
		for (int y = origin.y - range; y <= origin.y + range; y++) {
			if (g->m.has_flag(sealed, x, y)) {
				continue;
			}
			point p(x, y);
			item_on_map items(p, &(g->m.i_at(x, y)));
			on_map.push_back(items);

			ter_id terrain_id = g->m.ter(x, y);
			furn_id furniture_id = g->m.furn(x, y);
			if ((g->m.field_at(x, y).findField(fd_fire)) || (terrain_id == t_lava)) {
				item fire(g->itypes["fire"], 0);
				fire.charges = 1;
				item_from_souround ifs(p, fire);
				souround.push_back(ifs);
			}
			if (furniture_id == f_toilet || terrain_id == t_water_sh || terrain_id == t_water_dp){
				item water(g->itypes["water"], 0);
				water.charges = 50;
				item_from_souround ifs(p, water);
				souround.push_back(ifs);
			}

			int vpart = -1;
			vehicle *veh = g->m.veh_at(x, y, vpart);
			if (veh) {
				const int kpart = veh->part_with_function(vpart, vpc_kitchen);

				if (kpart >= 0) {
					item kitchen(g->itypes["installed_kitchen_unit"], 0);
					kitchen.charges = veh->fuel_left("battery", true);
					souround.push_back(item_from_souround(p, kitchen));

					item water(g->itypes["water_clean"], 0);
					water.charges = veh->fuel_left("water");
					souround.push_back(item_from_souround(p, water));
				}

				const int cpart = veh->part_with_function(vpart, vpc_cargo);
				if (cpart >= 0) {
					item_in_vehicle inveh(veh, cpart, &(veh->parts[cpart].items));
					in_veh.push_back(inveh);
				}
			}
		}
	}
}

void crafting_inventory_t::add_bio_toolset(const std::string &tool, calendar &turn) {
	std::string bionic_name("bio_tools_");
	bionic_name += tool;
	if(p->has_bionic(bionic_name)) {
		std::string tool_name("toolset_");
		tool_name += tool;
		item tools(item_controller->find_template(tool_name), turn);
		tools.charges = p->power_level;
		by_bionic.push_back(tools);
	}
}

crafting_inventory_t::crafting_inventory_t(game *g, player *p) : p(p) {
	form_from_map(g, point(p->posx, p->posy), PICKUP_RANGE);

	// iterator of all bionics of the player and grab the toolsets automaticly
	// This allows easy addition of more toolsets
	for(std::vector<bionic>::const_iterator a = p->my_bionics.begin(); a != p->my_bionics.end(); ++a) {
		const bionic &bio = *a;
		if(bio.id.compare(0, 10, "bio_tools_") == 0) {
			std::string tool_name("toolset_");
			tool_name += bio.id.substr(10);
			item tools(item_controller->find_template(tool_name), g->turn);
			tools.charges = p->power_level;
			by_bionic.push_back(tools);
		}
	}
	/*
	add_bio_toolset("knife", g->turn);
	add_bio_toolset("screwdriver", g->turn);
	add_bio_toolset("wrench", g->turn);
	add_bio_toolset("hacksaw", g->turn);
	add_bio_toolset("hammer", g->turn);
	add_bio_toolset("welder", g->turn);
	add_bio_toolset("hotplate", g->turn);
	add_bio_toolset("soldering_iron", g->turn);
	*/
}

std::list<item> crafting_inventory_t::consume_items(const itype_id &type, int count)
{
	std::vector<component> components;
	components.push_back(component(type, count));
	return consume_items(components);
}

std::list<item> crafting_inventory_t::consume_items(const std::vector<component> &components)
{
    std::list<item> ret;
    // For each set of components in the recipe, fill you_have with the list of all
    // matching ingredients the player has.
    std::vector<component> player_has;
    std::vector<component> map_has;
    std::vector<component> mixed;
    std::vector<component> player_use;
    std::vector<component> map_use;
    std::vector<component> mixed_use;

    for (int i = 0; i < components.size(); i++)
    {
        const itype_id &type = components[i].type;
        int count = abs(components[i].count);
        bool pl = false, mp = false;

        if (count > 0 && item_controller->find_template(type)->count_by_charges())
        {
            if (p->has_charges(type, count))
            {
                player_has.push_back(components[i]);
                pl = true;
            }
            if (map_has_charges(type, count))
            {
                map_has.push_back(components[i]);
                mp = true;
            }
            if (!pl && !mp && p->charges_of(type) + map_charges_of(type) >= count)
            {
                mixed.push_back(components[i]);
            }
        }
        else // Counting by units, not charges
        {

            if (p->has_amount(type, count))
            {
                player_has.push_back(components[i]);
                pl = true;
            }
            if (map_has_amount(type, count))
            {
                map_has.push_back(components[i]);
                mp = true;
            }
            if (!pl && !mp && p->amount_of(type) + map_amount_of(type) >= count)
            {
                mixed.push_back(components[i]);
            }

        }
    }

    if (player_has.size() + map_has.size() + mixed.size() == 1) // Only 1 choice
    {
        if (player_has.size() == 1)
        {
            player_use.push_back(player_has[0]);
        }
        else if (map_has.size() == 1)
        {
            map_use.push_back(map_has[0]);
        }
        else
        {
            mixed_use.push_back(mixed[0]);
        }
    }
    else // Let the player pick which component they want to use
    {
        std::vector<std::string> options; // List for the menu_vec below
        // Populate options with the names of the items
        for (int i = 0; i < map_has.size(); i++)
        {
            std::string tmpStr = item_controller->find_template(map_has[i].type)->name + " (nearby)";
            options.push_back(tmpStr);
        }
        for (int i = 0; i < player_has.size(); i++)
        {
            options.push_back(item_controller->find_template(player_has[i].type)->name);
        }
        for (int i = 0; i < mixed.size(); i++)
        {
            std::string tmpStr = item_controller->find_template(mixed[i].type)->name +" (on person & nearby)";
            options.push_back(tmpStr);
        }

        // unlike with tools, it's a bad thing if there aren't any components available
        if (options.size() == 0)
        {
            debugmsg("Attempted a recipe with no available components!");
            return ret;
        }

        // Get the selection via a menu popup
        int selection = menu_vec(false, "Use which component?", options) - 1;
        if (selection < map_has.size())
        {
            map_use.push_back(map_has[selection]);
        }
        else if (selection < map_has.size() + player_has.size())
        {
            selection -= map_has.size();
            player_use.push_back(player_has[selection]);
        }
        else
        {
            selection -= map_has.size() + player_has.size();
            mixed_use.push_back(mixed[selection]);
        }
    }

    for (int i = 0; i < player_use.size(); i++)
    {
        if (item_controller->find_template(player_use[i].type)->count_by_charges() &&
            player_use[i].count > 0)
        {
            std::list<item> tmp = p->use_charges(player_use[i].type, player_use[i].count);
            ret.splice(ret.end(), tmp);
        }
        else
        {
            std::list<item> tmp = p->use_amount(player_use[i].type, abs(player_use[i].count),
                                               (player_use[i].count < 0));
            ret.splice(ret.end(), tmp);
        }
    }
    for (int i = 0; i < map_use.size(); i++)
    {
        if (item_controller->find_template(map_use[i].type)->count_by_charges() &&
            map_use[i].count > 0)
        {
            std::list<item> tmp = g->m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                                                map_use[i].type, map_use[i].count);
            ret.splice(ret.end(), tmp);
        }
        else
        {
           std::list<item> tmp =  g->m.use_amount(point(p->posx, p->posy), PICKUP_RANGE,
                                               map_use[i].type, abs(map_use[i].count),
                                               (map_use[i].count < 0));
           ret.splice(ret.end(), tmp);
        }
    }
    for (int i = 0; i < mixed_use.size(); i++)
    {
        if (item_controller->find_template(mixed_use[i].type)->count_by_charges() &&
            mixed_use[i].count > 0)
        {
            int from_map = mixed_use[i].count - p->charges_of(mixed_use[i].type);
            std::list<item> tmp;
            tmp = p->use_charges(mixed_use[i].type, p->charges_of(mixed_use[i].type));
            ret.splice(ret.end(), tmp);
            tmp = g->m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                                mixed_use[i].type, from_map);
            ret.splice(ret.end(), tmp);
        }
        else
        {
            bool in_container = (mixed_use[i].count < 0);
            int from_map = abs(mixed_use[i].count) - p->amount_of(mixed_use[i].type);
            std::list<item> tmp;
            tmp = p->use_amount(mixed_use[i].type, p->amount_of(mixed_use[i].type),
                               in_container);
            ret.splice(ret.end(), tmp);
            tmp = g->m.use_amount(point(p->posx, p->posy), PICKUP_RANGE,
                               mixed_use[i].type, from_map, in_container);
            ret.splice(ret.end(), tmp);
        }
    }
    return ret;
}

void crafting_inventory_t::consume_tools(const itype_id &type, int charges, bool force_available)
{
	std::vector<component> components;
	components.push_back(component(type, charges));
	return consume_tools(components, force_available);
}

void crafting_inventory_t::consume_tools(const std::vector<component> &tools, bool force_available)
{
 bool found_nocharge = false;
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  if (!force_available && tools[i].available != 1)
  {
   continue;
  }
  const itype_id &type = tools[i].type;
  if (tools[i].count > 0) {
   int count = tools[i].count;
   if (p->has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (p->has_amount(type, 1) || map_has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge)
  return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0)
  p->use_charges(player_has[0].type, player_has[0].count);
 else if (map_has.size() == 1 && player_has.size() == 0)
  g->m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                   map_has[0].type, map_has[0].count);
 else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = item_controller->find_template(map_has[i].type)->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(item_controller->find_template(player_has[i].type)->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec(false, "Use which tool?", options) - 1;
  if (selection < map_has.size())
   g->m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                    map_has[selection].type, map_has[selection].count);
  else {
   selection -= map_has.size();
   p->use_charges(player_has[selection].type, player_has[selection].count);
  }
 }
}

bool crafting_inventory_t::has_tool(component &tool) const
{
    const itype_id &type = tool.type;
    if(tool.count <= 0)
    {
        if(has_amount(type, 1))
        {
            tool.available = 1;
        }
        else
        {
            tool.available = -1;
        }
    }
    else
    {
        if(has_charges(type, tool.count))
        {
            tool.available = 1;
        }
        else
        {
            tool.available = -1;
        }
    }
    return tool.available == 1;
}

bool crafting_inventory_t::has_any_tool(std::vector<component> &set_of_tools) const
{
    if(set_of_tools.size() == 0)
    {
        return true; // no tool needed, we have that
    }
    bool has_tool_in_set = false;
    for(std::vector<component>::iterator tool_it = set_of_tools.begin(); tool_it != set_of_tools.end(); tool_it++)
    {
        component &tool = *tool_it;
        if(has_tool(tool))
        {
            has_tool_in_set = true;
        }
    }
    return has_tool_in_set;
}

bool crafting_inventory_t::has_any_component(std::vector<component> &set_of_components) const
{
    if(set_of_components.size() == 0)
    {
        return true; // no item needed?
    }
    bool has_comp_in_set = false;
    for(std::vector<component>::iterator comp_it = set_of_components.begin(); comp_it != set_of_components.end(); comp_it++)
    {
        component &comp = *comp_it;
        if(has_component(comp))
        {
            has_comp_in_set = true;
        }
    }
    return has_comp_in_set;
}

bool crafting_inventory_t::has_component(component &comp) const
{
    const itype_id &type = comp.type;
    if (item_controller->find_template(type)->count_by_charges() && comp.count > 0)
    {
        if (has_charges(type, comp.count))
        {
            comp.available = 1;
        }
        else
        {
            comp.available = -1;
        }
    }
    else if (has_amount(type, abs(comp.count)))
    {
        comp.available = 1;
    }
    else
    {
        comp.available = -1;
    }
    return comp.available == 1;
}
