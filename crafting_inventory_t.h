#ifndef _CRAFTING_INVENTORY_T_H_
#define _CRAFTING_INVENTORY_T_H_

#include <string>
#include <vector>
#include <list>
#include "item.h"
#include "crafting.h"

class vehicle;
/**
 * This inventory represents all the things the player may use
 * when crafting (or in similar activities like building a car).
 * It contains:
 * 1. the inventory of the player,
 * 2. items on the surrounding map tiles,
 * 3. bioncs (not all, only those that might be used in crafting like the toolset)
 * 4. items that are inside of containers in cars
 * (this uses the same radius as for map tiles, so the player may use items that
 * are _on_ the map or in a car on that map tile)
 * 5. surronding natural "things" like fire or a water source.
 * 
 * The class does not copy every item from its original point (e.g. from the
 * map or from the inventory), it stores a reference to those places instead.
 * That means it stores a reference to the sourounding 5x5 map tiles and
 * does not copy the items from those tiles.
 * 
 * The several functions to examin if something exists (has_*) iterate
 * over these stored references.
 */
class crafting_inventory_t {
public:
	/**
	 * Items that are on the ground (or in furniture) on the map.
	 */
	struct item_on_map {
		/** Point on the map */
		point origin;
		/** Points to the vector returned by map::i_at */
		std::vector<item> *items;
		item_on_map(const point &p, std::vector<item> *i) : origin(p), items(i) { }
	};
	struct item_in_vehicle {
		/** The vehicle the items are in. */
		vehicle *veh;
		/** The part number of the cargo unit */
		int part_num;
		/** points to vehicle_part::items of the cargo unit of the car */
		std::vector<item> *items;
		item_in_vehicle(vehicle *v, int p, std::vector<item> *i) : veh(v), part_num(p), items(i) { }
	};
	/** bionics are represented as pseudo-items only */
	typedef item item_from_bionic;
	struct item_from_souround {
		/** Point on the map */
		point origin;
		/** The pseudo-item representing something (like water or fire) */
		item it;
		item_from_souround(const point &p, const item &i) : origin(p), it(i) { }
	};

	player *p;

	std::list<item_on_map> on_map;
	std::list<item_in_vehicle> in_veh;
	std::list<item_from_bionic> by_bionic;
	std::list<item_from_souround> souround;


	std::list<item> use_amount(const itype_id &type, const int amount, const bool use_container = false);
	std::list<item> use_charges(const itype_id &type, const int amount);

	bool has_amount (const std::vector<std::pair<itype_id, int> > &items) const;

	bool has_amount (const itype_id &type, int quantity) const;
	bool has_charges(const itype_id &type, int quantity) const;

	int amount_of (const itype_id &type) const;
	int charges_of(const itype_id &type) const;

	// Check the map (and vehicles) only, not the player inventory and not bioncs
	bool map_has_amount (const itype_id &type, int quantity) const;
	bool map_has_charges(const itype_id &type, int quantity) const;

	int map_amount_of (const itype_id &type) const;
	int map_charges_of(const itype_id &type) const;

	/**
	 * Check if this tool/component is available (checks charges too),
	 * set component::available to -1 or +1
	 * Works for single components/tools or for a list of.
	 * All 4 functions return true if at least one item is available.
	 */
	bool has_tool(component &tool) const;
	bool has_any_tool(std::vector<component> &set_of_tools) const;
	bool has_component(component &component) const;
	bool has_any_component(std::vector<component> &set_of_components) const;
	
	bool has(const component &component) const;


	std::list<item> consume_items(const std::vector<component> &components);
	void consume_tools(const std::vector<component> &tools, bool force_available);
	
	// Shortcuts for consuming only one item
	std::list<item> consume_items(const itype_id &type, int count);
	void consume_tools(const itype_id &type, int charges, bool force_available);

	bool has_items_with_quality(const std::string &name, int level, int amount) const;

	// See veh_interact.cpp
	item consume_vpart_item (game *g, const itype_id &itid);

	std::vector<const item*> all_items_by_type(const itype_id &itid) const;
	/**
	 * Similar to all_items_by_type(itype_id), but gets all items that match
	 * _any_ of the components required.
	 * This should be used for the tools setting of recipes where the player
	 * needs any tool of those listed.
	 * This function is equal to calling all_items_by_type(itype_id) for each
	 * component and than mergin the results.
	 */
	std::vector<const item*> all_items_by_type(const std::vector<component> &types) const;
	
	/**
	 * Examine all matching tools and report the best (lowest)
	 * time_modi.
	 */
	double compute_time_modi(const std::vector<component> &tools) const;

	crafting_inventory_t(game *g, player *p);

protected:
	typedef int(*CountFunction)(const itype_id &, const item &);

	enum {
		S_PLAYER  = 1<< 0,
		S_MAP     = 1<< 1,
		S_VEHICLE = 1<< 2,
		S_ALL     = 0xFFFFFFFF
	} source_flags;

	/** Returns the number of charges of this item type. Stops counting when reaching max charges */
	int count(const itype_id &type, CountFunction function, int max, int sources) const;

	static int charges_of(const itype_id &type, const item &it);
	static int amount_of(const itype_id &type, const item &it);

	void add_bio_toolset(const std::string &tool, calendar &turn);
	void form_from_map(game *g, point origin, int distance);
};

#endif
