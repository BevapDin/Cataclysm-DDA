#ifndef _CRAFTING_INVENTORY_T_H_
#define _CRAFTING_INVENTORY_T_H_

#include <string>
#include <vector>
#include <list>
#include "item.h"
#include "crafting.h"

class vehicle;
struct bionic;
struct recipe;
struct construction_stage;
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
     * This is only a wrapper that helps accessing the item-vector of
     * the map class.
     */
    struct items_on_map {
        /** Point on the map */
        point origin;
        /** Points to the vector returned by map::i_at */
        std::vector<item> *items;
        items_on_map(const point &p, std::vector<item> *i) : origin(p), items(i) { }
    };
    /**
     * Items that are in the cargo slot of a vehicle.
     * This is only a wrapper that helps accessing the item-vector of
     * the vehicle_part class.
     */
    struct items_in_vehicle {
        /** The vehicle the items are in. */
        vehicle *veh;
        /** The part number of the cargo unit */
        int part_num;
        /** points to vehicle_part::items of the cargo unit of the car */
        std::vector<item> *items;
        items_in_vehicle(vehicle *v, int p, std::vector<item> *i) : veh(v), part_num(p), items(i) { }
    };
    /**
     * This contains a pseudo-item that represents a special vehicle_part,
     * like the kitchen. Some vehicle_parts may add several of these
     * pseudo-items.
     */
    struct item_from_vpart {
        /** The vehicle with the vehicle_part (like a kitchen). */
        vehicle *veh;
        /** The part number of the vehicle_part*/
        int part_num;
        /** the pseudo-item of the vehicle_part (like water_clean from kitchen) */
        item it;
        item_from_vpart(vehicle *v, int p, const item &i) : veh(v), part_num(p), it(i) { }
    };
    /**
     * This contains a pseudo-item that represents a bionic. It is used at
     * least by the toolset bionics to represent their functionalities.
     */
    struct item_from_bionic {
        std::string bio_name; /* internal bionic type, not the name of the pseudo item */
        item it;
        item_from_bionic(const std::string &n, const item &i) : bio_name(n), it(i) { }
    };
    /**
     * This contains pseudo-items that represent thins on the map (but not
     * actuall items). Example: water, fire.
     */
    struct item_from_souround {
        /** Point on the map */
        point origin;
        /** The pseudo-item representing something (like water or fire) */
        item it;
        item_from_souround(const point &p, const item &i) : origin(p), it(i) { }
    };

    /**
     * Theses bit flags corrospond to the sources of item (e.g.
     * #by_bionic or #on_map)
     */
    typedef enum {
        S_MAP      = 1<< 0, // items from on_map and souroundings (fire, water) are considered
        S_PLAYER   = 1<< 1, // items from the player (inventory, bioncs, weapon, worn) are considered
        S_VEHICLE  = 1<< 2, // items from vehicles (including the special vehicle_parts like kitchen)
        
        S_ALL      = S_MAP | S_VEHICLE | S_PLAYER,
        S_FROM_MAP = S_MAP | S_VEHICLE,
        S_FROM_PLAYER = S_PLAYER,
    } source_flags;
    
    typedef enum {
        C_CHARGES,
        C_AMOUNT,
    } count_type;
    
    struct requirement {
        itype_id type;
        int count;
        count_type ctype;

        requirement(const itype_id &t, int c, count_type ct) : type(t), count(c), ctype(ct) { }
        requirement(const component &comp, bool as_tool);
        
        int get_charges_or_amount(const item &it) const;
    };
    
    /**
     * This represents a specifc item (might be a pseudo-item),
     * that can inidiviualy used.
     */
    struct candidate_item {
    private:
        // Where (abstract) the item is (on the map, in a car, a bionic, ...)
        enum {
            from_souround,
            from_bionic,
            on_map,
            in_vehicle,
            in_inventory,
            weapon,
            in_vpart,
        } location;
        union {
            struct {
                int mapx;
                int mapy;
                int mindex;
            };
            struct {
                signed char invlet;
            };
            struct {
                vehicle *veh;
                int part_num;
                union {
                    int vindex;
                    item_from_vpart *vpartitem;
                };
            };
            struct {
                item_from_bionic *bionic;
            };
            struct {
                item_from_souround *souroundings;
            };
        };
    public:
        // a copy of the item
        item the_item;
        
        candidate_item(const item &vpitem):
            location(weapon), the_item(vpitem) { }
        candidate_item(int x, int y, int i, const item &vpitem):
            location(on_map),mapx(x),mapy(y),mindex(i), the_item(vpitem) { }
        candidate_item(char ch, const item &vpitem):
            location(in_inventory),invlet(ch), the_item(vpitem) { }
        candidate_item(vehicle *veh, int part_num, int i, const item &vpitem):
            location(in_vehicle),veh(veh), part_num(part_num), vindex(i), the_item(vpitem) { }
        candidate_item(vehicle *veh, int part_num, item_from_vpart &i):
            location(in_vpart),veh(veh), part_num(part_num), vpartitem(&i), the_item(i.it) { }
        candidate_item(item_from_bionic &it):
            location(from_bionic), bionic(&it), the_item(it.it) { }
        candidate_item(item_from_souround &it):
            location(from_souround), souroundings(&it), the_item(it.it) { }
            
        // used to sort these items (sorted mainly by location)
        bool operator<(const candidate_item &other) const;
        // a string containing the item name (item::tname) and a hint
        // to the location ("nearby").
        std::string to_string() const;
        // check if this item is part of the sources specified as source_flags
        bool is_source(source_flags sources) const;
        
        void consume(game *g, player *p, requirement &req, std::list<item> &ret);
    private:
        int drainVehicle(const std::string &ftype, int amount, std::list<item> &ret);
    };
    typedef std::vector<candidate_item> civec;
    
protected:
    static bool has_different_types(const std::vector<candidate_item> &cv);
    // check if all items int he vector are actually equal (same type, same damage, ...)
    // basicly they are equal if the user would considered them equall.
    static bool all_equal(const std::vector<candidate_item> &cv);
    // removes all entries in cv, that are not part of the specified sources
    static void filter(std::vector<candidate_item> &cv, source_flags sources);

    int collect_candidates(const requirement &req, int source, std::vector<candidate_item> &result);

public:
    
    
protected:
	player *p;
	std::list<items_on_map> on_map;
	std::list<items_in_vehicle> in_veh;
    std::list<item_from_vpart> vpart;
	std::list<item_from_bionic> by_bionic;
	std::list<item_from_souround> souround;
    
    /**
     * This is a general counting function.
     * It can count by charges or by amount,
     * it can also count only in specifc sources (see source_flags).
     * @param type the itemtype to count.
     * @param what what to count, either the count of items or their
     * (summed) charges.
     * @param max if this is greater than 0 counting steps as soon as
     * this value is reached. This is usefull when one wants to check
     * if a ceratin count exists, but does not care about the actuall
     * value.
     * @param sources A or-ed collection of source_flags. Only items
     * from these sources are considered.
     */
    int count(const itype_id &type, count_type what, int max, int sources) const;
        
    /**
     * Examine all matching tools and report the best (lowest)
     * time_modi.
     */
    double compute_time_modi(const std::vector<component> &tools) const;
    double compute_time_modi(const std::vector<component> &tools, const item *&item_that_got_used) const;
    double compute_time_modi(const itype_id &itid, int count, const item *&item_that_got_used) const;
    double compute_time_modi(const std::vector<component> *begin, size_t count) const;
    
    /**
     * This is called during #init, with each bionic that is installed.
     * The function adds pseduo-items for some bionics (with the power
     * level of the player as charges) to by_bionic.
     */
    void add_bio_toolset(const bionic &bio, const calendar &turn);
    
    void form_from_map(game *g, point origin, int distance);
    
    typedef enum { assume_components, assume_tools, assume_tools_force_available } consume_flags;
    std::list<item> consume(const std::vector<component> &x, consume_flags flags);

    /**
     * See #crafting_inventory_t(game*, player*, int)
     * This is the wrapper that does what is descripted there.
     */
    void init(game *g, int range);
public:
    /**
     * Same as <code>crafting_inventory_t(g, p, PICKUP_RANGE)</code>
     */
    crafting_inventory_t(game *g, player *p);
    /**
     * Create an inventory wrapper for the given player.
     * The position of the player is used to add items from the map,
     * sourounding him.
     * @param range The pickup range, items (anf vehicle parts) that
     * are inside this range are added to this inventory.
     * If range is -1, no items from the map or from vehicles are added
     * at all.
     */
    crafting_inventory_t(game *g, player *p, int range);
    
    /**
     * Basicly the interface to crafting (and construction, and vehicle
     * repair/install).
     * This function checks that the components needed are all available.
     * See the #has_all, #has_any and #has.
     * Note: the input are sets of sets of components.
     * @return true if: if components is empty or contains only empty inner
     * sets.
     * true also if each of the outer sets fullfiles #has_any.
     */
    bool has_all_components(std::vector< std::vector<component> > &components) const;
    /**
     * Same as has_all_components, only the input is assumed to be
     * tools and their charges (their must be a tool with that charge)
     * or simple tools requirements (without charge) which means the
     * tool itself must be available.
     */
    bool has_all_tools(std::vector< std::vector<component> > &tools) const;
    
    /**
     * Check to se if the requirement is fullfilled from_bionicone of
     * the given sources.
     * @param sources a combination (or-ed) of source_flags.
     */
    bool has(const requirement &req, int sources = S_ALL) const;
    /**
     * Returns the count of items/charges (as defined by the
     * requirement) that are fullfilled (might be smaller than
     * the actuall required).
     * @param sources a combination (or-ed) of source_flags. Only
     * items from the listed sources are considered.
     */
    int count(const requirement &req, int sources = S_ALL) const;
    /**
     * Check if this tool/component is available (checks charges too),
     * sets component::available to -1 or +1
     * @return true if the tool/component is available.
     */
    bool has(component &x, bool as_tool) const;
    /**
     * @see #has
     * @return true if at least one tool/component is available.
     */
    bool has_any(std::vector<component> &set_of_x, bool as_tool) const;
    /**
     * @see #has_any
     * @return true if at least one tool/component from each subset is available.
     */
    bool has_all(std::vector< std::vector<component> > &xs, bool as_tool) const;
    /**
     * Consume components.
     * @return the consumed items.
     */
    std::list<item> consume_items(const std::vector<component> &components);
    /**
     * Wrapper to consume_items, same as
     * <code>consume_items(std::vector(1, component(type, count)));</code>
     */
    std::list<item> consume_items(const itype_id &type, int count);
    /**
     * Consume tool charges, if no charges have been required, nothing
     * is changed.
     */
    void consume_tools(const std::vector<component> &tools, bool force_available);
    /**
     * Wrapper to consume_items, same as
     * <code>consume_tools(std::vector(1, component(type, charges)));</code>
     */
    void consume_tools(const itype_id &type, int charges, bool force_available);
    // See veh_interact.cpp
    item consume_vpart_item(game *g, const itype_id &itid);

    
    
    /**
     * FIXME: change to store used items/tools before starting the action
     */
    double compute_time_modi(const recipe &making) const;
    /**
     * FIXME: change to store used items/tools before starting the action
     */
    double compute_time_modi(const construction_stage &stage) const;
    
    
    
    /**
     * FIXME: legacy, use
     * <code>has(component(it, quantity), as_tool)</code>
     * instead
     */
    bool has_amount(const itype_id &it, int quantity) const {
        return has(requirement(it, quantity, C_AMOUNT));
    }
    /**
     * FIXME: legacy, use
     * <code>has(component(it, quantity), as_tool)</code>
     * instead
     */
    bool has_charges(const itype_id &it, int quantity) const {
        return has(requirement(it, quantity, C_CHARGES));
    }
    /**
     * FIXME: legacy, should be changed to "func:" system
     */
    bool has_items_with_quality(const std::string &name, int level, int amount) const;
    
protected:

#if 0
	std::list<item> use_amount(const itype_id &type, const int amount, const bool use_container = false);
    std::list<item> use_charges(const itype_id &type, const int amount);
    int amount_of(const itype_id &it) const {
        return count(it, C_AMOUNT, -1, S_ALL);
    }
    int charges_of(const itype_id &it) const {
        return count(it, C_CHARGES, -1, S_ALL);
    }
    
    // Check the map (and vehicles) only, not the player inventory and not bioncs
    bool map_has_amount(const itype_id &it, int quantity) const {
        return (count(it, C_AMOUNT, quantity, S_MAP | S_VEHICLE | S_SOUROUND) >= quantity);
    }
    bool map_only_has_amount(const itype_id &it, int quantity) const {
        return (count(it, C_AMOUNT, quantity, S_MAP) >= quantity);
    }
    int map_amount_of(const itype_id &it) const {
        return count(it, C_AMOUNT, -1, S_MAP | S_VEHICLE | S_SOUROUND);
    }
    int map_charges_of(const itype_id &it) const {
        return count(it, C_CHARGES, -1, S_MAP | S_VEHICLE | S_SOUROUND);
    }
    // Check the map only, no vehicles, not souroundings, not the player inventory and not bioncs
    bool map_has_charges(const itype_id &it, int quantity) const {
        return (count(it, C_CHARGES, quantity, S_MAP | S_VEHICLE | S_SOUROUND) >= quantity);
    }
    
    bool has_amount (const std::vector<std::pair<itype_id, int> > &items) const;
    
    



#endif

    
	


protected:
#if 0
    int player_charges_of(const itype_id &type) const;
#endif
     

	static int charges_of(const itype_id &type, const item &it);
	static int amount_of(const itype_id &type, const item &it);

#if 0
    std::list<item> p_use_charges(const itype_id &type, int amount);
#endif
};

#endif
