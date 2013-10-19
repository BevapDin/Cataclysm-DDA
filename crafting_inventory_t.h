#ifndef _CRAFTING_INVENTORY_T_H_
#define _CRAFTING_INVENTORY_T_H_

#include <string>
#include <vector>
#include <list>
#include "item.h"
#include "crafting.h"

namespace picojson {
    class value;
}

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
 * The class does not copy every item from its positional point (e.g. from the
 * map or from the inventory), it stores a reference to those places instead.
 * That means it stores a reference to the surrounding 5x5 map tiles and
 * does not copy the items from those tiles.
 * Same for items in vehicles.
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
        point position;
        /** Points to the vector returned by map::i_at */
        std::vector<item> *items;
        items_on_map(const point &p, std::vector<item> *i) : position(p), items(i) { }
    };
    /**
     * Items that are in the cargo slot of a vehicle.
     * This is only a wrapper that helps accessing the item-vector of
     * the vehicle_part class.
     */
    struct items_in_vehicle_cargo {
        /** The vehicle the items are in. */
        vehicle *veh;
        /** The part number of the cargo unit */
        int part_num;
        /** points to vehicle_part::items of the cargo unit of the car */
        std::vector<item> *items;
        items_in_vehicle_cargo(vehicle *v, int p, std::vector<item> *i) : veh(v), part_num(p), items(i) { }
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
        item the_item;
        item_from_vpart(vehicle *v, int p, const item &i) : veh(v), part_num(p), the_item(i) { }
    };
    /**
     * This contains a pseudo-item that represents a bionic. It is used at
     * least by the toolset bionics to represent their functionalities.
     */
    struct item_from_bionic {
        std::string bio_id; /* internal bionic id (bionic::id), not the name of the pseudo item */
        item the_item;
        item_from_bionic(const std::string &n, const item &i) : bio_id(n), the_item(i) { }
    };
    /**
     * This contains pseudo-items that represent thins on the map (but not
     * actuall items). Example: water, fire.
     */
    struct item_from_surrounding {
        /** Point on the map */
        point position;
        /** The pseudo-item representing something (like water or fire) */
        item the_item;
        item_from_surrounding(const point &p, const item &i) : position(p), the_item(i) { }
    };

    /**
     * Theses bit flags corrospond to the sources of item (e.g.
     * #by_bionic or #on_map)
     */
    typedef enum {
        S_MAP      = 1<< 0, // items from on_map and surroundings (fire, water) are considered
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
    
    /**
     * Abstract requirement for type.
     * This class hides the diffenrece beewteen counting
     * by charges and counting by amount.
     * Example: (need 6 items of type "welder")
     * <code>requirement req("welder", 6, C_AMOUNT);</code>
     * Need an welder with at least 200 charges
     * <code>requirement req("welder", 200, C_CHARGES);</code>
     * To check if a item fullfiles the requirement, use:
     * <code>if(req(the_item) >= req.count) { }</code>
     */
    struct requirement {
        count_type ctype;
        itype_id type;
        int count;

        requirement(const itype_id &t, int c, count_type ct) : ctype(ct), type(t), count(c) { }
        requirement(const component &comp, bool as_tool);
        
        /**
         * Get how much the item counts towards the requirement.
         * An item that is invcompatible does not count at all
         * (returns 0). For compatible items (item.type == required type)
         * it return either 1 if counted by amount or the charges
         * of the item if counted by charges.
         */
        int get_charges_or_amount(const item &the_item) const;
        
        int operator()(const item &the_item) const {
            return get_charges_or_amount(the_item);
        }
        requirement& operator-=(const item& the_item) {
            count -= get_charges_or_amount(the_item);
            return *this;
        }
    };
    
    /**
     * This represents a specifc item (might be a pseudo-item),
     * that can inidiviualy used.
     */
    struct candidate_item {
    private:
        // Where (abstract) the item is (on the map, in a car, a bionic, ...)
        typedef enum {
            LT_MAP,
            LT_SURROUNDING,
            LT_VEHICLE_CARGO,
            LT_VPART,
            LT_WEAPON,
            LT_INVENTORY,
            LT_BIONIC,
        } LocationType;
        LocationType location;
        union {
            struct {
                items_on_map *mapitems;
                int mindex;
            };
            struct {
                // invlet of item in player's inventory (or worn, weapon has its own location)
                signed char invlet;
            };
            struct {
                items_in_vehicle_cargo *vitems;
                // Index of the item in the cargo of the vehilce part (vpart::items)
                int iindex;
            };
            struct {
                // pointer to the pseudo-item representing this vpart
                item_from_vpart *vpartitem;
            };
            struct {
                item_from_bionic *bionic;
            };
            struct {
                item_from_surrounding *surroundings;
            };
        };
    public:
        // a copy of the item
        item the_item;
        
        candidate_item(const item &vpitem):
            location(LT_WEAPON),
            the_item(vpitem)
            { }

        candidate_item(items_on_map &ifm, int i, const item &vpitem):
            location(LT_MAP),
            mapitems(&ifm),
            mindex(i),
            the_item(vpitem)
            { }

        candidate_item(char ch, const item &vpitem):
            location(LT_INVENTORY),
            invlet(ch),
            the_item(vpitem)
            { }

        candidate_item(items_in_vehicle_cargo &ifv, int i, const item &vpitem):
            location(LT_VEHICLE_CARGO),
            vitems(&ifv),
            iindex(i),
            the_item(vpitem)
            { }

        candidate_item(item_from_vpart &ifv):
            location(LT_VPART),
            vpartitem(&ifv),
            the_item(ifv.the_item)
            { }

        candidate_item(item_from_bionic &ifb):
            location(LT_BIONIC),
            bionic(&ifb),
            the_item(ifb.the_item)
            { }

        candidate_item(item_from_surrounding &ifs):
            location(LT_SURROUNDING),
            surroundings(&ifs),
            the_item(ifs.the_item)
            { }

        candidate_item(crafting_inventory_t &cinv, const std::string &data) { deserialize(cinv, data); }
        candidate_item(crafting_inventory_t &cinv, const picojson::value &v) { deserialize(cinv, v); }
            
        // used to sort these items (sorted mainly by location)
        bool operator<(const candidate_item &other) const;
        // a string containing the item name (item::tname) and a hint
        // to the location ("nearby").
        std::string to_string() const;
        // check if this item is part of the sources specified as source_flags
        bool is_source(source_flags sources) const;
        
        std::string serialize() const;
        void serialize(picojson::value &v) const;
        bool valid() const;
        
        void consume(game *g, player *p, requirement &req, std::list<item> &ret) const;
    private:
        void deserialize(crafting_inventory_t &cinv, const std::string &data);
        void deserialize(crafting_inventory_t &cinv, const picojson::value &v);
        int drainVehicle(const std::string &ftype, int amount, std::list<item> &ret) const;
    };
    typedef std::vector<candidate_item> civec;
    
    /**
     * List of items at their current polocation.
     */
    typedef civec item_ref_list;
    
protected:
    static bool has_different_types(const civec &cv);
    // check if all items int he vector are actually equal (same type, same damage, ...)
    // basicly they are equal if the user would considered them equall.
    static bool all_equal(const civec &cv);
    // removes all entries in cv, that are not part of the specified sources
    static void filter(civec &cv, source_flags sources);

    int collect_candidates(const requirement &req, int source, civec &result);
    
    friend candidate_item; // for deserialize

	player *p;
	std::list<items_on_map> on_map;
	std::list<items_in_vehicle_cargo> in_veh;
    std::list<item_from_vpart> vpart;
	std::list<item_from_bionic> by_bionic;
	std::list<item_from_surrounding> surround;
    
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
    /**
     * Same as other compute_time_modi, but stores the item
     * which was choosen in item_that_got_used.
     */
    double compute_time_modi(const std::vector<component> &tools, const item *&item_that_got_used) const;
    double compute_time_modi(const itype_id &itid, int count, const item *&item_that_got_used) const;
    double compute_time_modi(const std::vector<component> *begin, size_t count) const;
    
    /**
     * This is called during #init, with each bionic that is installed.
     * The function adds pseduo-items for some bionics (with the power
     * level of the player as charges) to by_bionic.
     */
    void add_bio_toolset(const bionic &bio, const calendar &turn);
    /**
     * Populate on_map with references to items on map, 
     * in_veh with references to items in vehicles (as cargo),
     * vpart with pseudo items as wrapper for vehicle vparts.
     * surround with pseudo items from the surrounding.
     */
    void form_from_map(game *g, point position, int distance);
    
    typedef enum { assume_components, assume_tools, assume_tools_force_available } consume_flags;
    int consume(const std::vector<component> &x, consume_flags flags, std::list<item> &used_items);
    
    /**
     * Consume the items to fullfill the requirement.
     * If more items are given only the first few are consumed
     * until the requirement is fullfilled.
     */
    void consume(requirement req, const civec &selected_items, std::list<item> &used_items);
    
    /**
     * Ask the user to choose a single item from the list of candidates.
     * This function tries to avoid the user interaction if possible,
     * (e.g. only on candidate item, or all are equally good, or one is
     * superior to all the others, ...)
     * @param candidates The items the user can choose from - must not be empty!.
     * @return The selected candidate
     */
    candidate_item ask_for_single_item(const civec &candidates);
    /**
     * Given a list of candidate items and a requirement, ask the
     * user for enough items to fullfill the given requirement.
     * This function tries to avoid the user interaction if possible,
     * (e.g. only on candidate item, or all are equally good, or one is
     * superior to all the others, ...)
     * @param req The requirement that the selected candidates must (in sum)
     * fullfill.
     * @param candidates The items the user can choose from - must not be empty!.
     * @param selected_candidates [out] The selected candidates, this will be
     * a subset of of the candidates that fullfiles the requirement.
     */
    void ask_for_items_to_use(const requirement &req, const civec &candidates, civec &selected_candidates);
    
    /**
     * Wrapper for ask_for_items_to_use, that stores the selected
     * items inthe input list.
     */
    void ask_for_items_to_use(const requirement &req, civec &candidates);
    
    /**
     * Reduce the list of candidates but still match the requirement.
     * This function removes candidates from the end of the list.
     */
    void reduce(requirement req, civec &candidates);
    
    /**
     * Ask the user to select items according to the requirements in
     * components.
     * Example: let components be { { "2x4", 10 }, { "log", 1 } }
     * and the user selected "2x4", than the function returns 0
     * and has references to at least 10 "2x4" items stored in
     * selected_items.
     * Note: the selected items may be more than actually required in
     * case the items come from the inventory. In this case only the 
     * invlet is stored, but because items stack this invlet may refer
     * to many identical item.
     * @param components A list of components needed
     * @param selected_items [out] the user (or autmoticly) selected items
     * are stored here.
     * @return The index into components for which the items have been selected.
     */
    int select_items_to_use(const std::vector<component> &components, consume_flags flags, civec &selected_items);
    
    
    
    void gather_inputs(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec);
    std::list<item> consume_gathered(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec);

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
     * surrounding him.
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
     * Check to se if the requirement is fullfilled LT_BIONICone of
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
    std::list<item> consume_items(const std::vector< std::vector<component> > &components);
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
    void consume_tools(const std::vector< std::vector<component> > &tools, bool force_available);
    /**
     * Wrapper to consume_items, same as
     * <code>consume_tools(std::vector(1, component(type, charges)));</code>
     */
    void consume_tools(const itype_id &type, int charges, bool force_available);
    /**
     * This is similar to consume_items, but is used in vehilce
     * interaction.
     */
    item consume_vpart_item(game *g, const itype_id &itid);

    
    void gather_input(const recipe &making, std::vector<std::string> &strVec);

    std::string serialize(int index, const civec &vec);
    int deserialize(const std::string &data, civec &vec);

    std::list<item> consume_gathered(const recipe &making, std::vector<std::string> &strVec);
    
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
     * <code>has(component(the_item, quantity), as_tool)</code>
     * instead
     */
    bool has_amount(const itype_id &the_item, int quantity) const {
        return has(requirement(the_item, quantity, C_AMOUNT));
    }
    /**
     * FIXME: legacy, use
     * <code>has(component(the_item, quantity), as_tool)</code>
     * instead
     */
    bool has_charges(const itype_id &the_item, int quantity) const {
        return has(requirement(the_item, quantity, C_CHARGES));
    }
    /**
     * FIXME: legacy, should be changed to "func:" system
     */
    bool has_items_with_quality(const std::string &name, int level, int amount) const;
    
protected:

#if 0
	std::list<item> use_amount(const itype_id &type, const int amount, const bool use_container = false);
    std::list<item> use_charges(const itype_id &type, const int amount);
    int amount_of(const itype_id &the_item) const {
        return count(the_item, C_AMOUNT, -1, S_ALL);
    }
    int charges_of(const itype_id &the_item) const {
        return count(the_item, C_CHARGES, -1, S_ALL);
    }
    
    // Check the map (and vehicles) only, not the player inventory and not bioncs
    bool map_has_amount(const itype_id &the_item, int quantity) const {
        return (count(the_item, C_AMOUNT, quantity, S_MAP | S_VEHICLE | S_SOUROUND) >= quantity);
    }
    bool map_only_has_amount(const itype_id &the_item, int quantity) const {
        return (count(the_item, C_AMOUNT, quantity, S_MAP) >= quantity);
    }
    int map_amount_of(const itype_id &the_item) const {
        return count(the_item, C_AMOUNT, -1, S_MAP | S_VEHICLE | S_SOUROUND);
    }
    int map_charges_of(const itype_id &the_item) const {
        return count(the_item, C_CHARGES, -1, S_MAP | S_VEHICLE | S_SOUROUND);
    }
    // Check the map only, no vehicles, not surroundings, not the player inventory and not bioncs
    bool map_has_charges(const itype_id &the_item, int quantity) const {
        return (count(the_item, C_CHARGES, quantity, S_MAP | S_VEHICLE | S_SOUROUND) >= quantity);
    }
    
    bool has_amount (const std::vector<std::pair<itype_id, int> > &items) const;
    
    



#endif

    
	


protected:
#if 0
    int player_charges_of(const itype_id &type) const;
#endif
     

	static int charges_of(const itype_id &type, const item &the_item);
	static int amount_of(const itype_id &type, const item &the_item);

#if 0
    std::list<item> p_use_charges(const itype_id &type, int amount);
#endif
};

#endif
