#ifndef _CRAFTING_INVENTORY_T_H_
#define _CRAFTING_INVENTORY_T_H_

#include <string>
#include <vector>
#include <list>
#include "item.h"
#include "crafting.h"

class calendar;
struct player_activity;
class vehicle;
struct bionic;
struct requirement_data;
class player;
class map_stack;
class vehicle_stack;
class map;
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
 * 
 * Usage (outside of requirement_datas):
 * 1. crreate a crafting_inventory_t object,
 * 2. check requirement_data: use has_*_tools and has_*_components
 * 3. consume items: consume_*_tools and consume_*_items
 * Make sure that you call the matching functions for checking
 * and for consuming (e.g. has_all_tools and consume_all_tools).
 * <code>
 * crafting_inventory_t cinv(g, p);
 * if(cinv.has_components("2x4", 1) && cinv.has_tools("func:welder", 2) && cinv.has_tools("knife")) {
 *  ... // make something with the 2x4 and the welder charges and the knife
 *  cinv.consume_components("2x4", 1);
 *  cinv.consume_tools("func:welder", 2);
 * }
 * </code>
 * Or with a list of components:
 * <code>
 * std::vector< components > tools;
 * tools.push_back(component("knife", -1));
 * tools.push_back(component("func:welder", 2));
 * std::vector< components > comps;
 * comps.push_back(component("2x4", 10));
 * comps.push_back(component("log", 1));
 * if(cinv.has_any_components(comps) && cinv.has_all_tools(tools)) {
 *  ... do somthing
 *  cinv.consume_any_compontens(comps);
 *  cinv.consume_all_tools(tools);
 * }
 * </code>
 * The crafting uses this:
 * <code>
 * if(cinv.has_all_requirements(making)) {
 *  player->assign_activity(CRAFT, making);
 *  cinv.gather_inputs(making, player->activity);
 * }
 * </code>
 * And later when the crafting is completed
 * <code>
 * std::list< item > items, tools;
 * cinv.consume_gathered(making, player->activity, items, tools);
 * ... (optional) do something with consumed tools/items
 * </code>
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
        tripoint position;
        map *m;
        map_stack items() const;
        items_on_map(const tripoint &p, map *m) : position(p), m(m) { }
    };
    /**
     * Items that are in the cargo slot of a vehicle.
     * This is only a wrapper that helps accessing the item-vector of
     * the vehicle_part class.
     */
    struct items_in_vehicle_cargo {
        /** The vehicle the items are in. */
        vehicle *veh;
        /** The relative coords (mount_dx/mount_dy) of the cargo unit */
        int mount_dx;
        int mount_dy;
        vehicle_stack items() const;
        items_in_vehicle_cargo(vehicle *v, int x, int y) : veh(v), mount_dx(x), mount_dy(y) { }
    };
    /**
     * This contains a pseudo-item that represents a special vehicle_part,
     * like the kitchen. Some vehicle_parts may add several of these
     * pseudo-items.
     */
    struct item_from_vpart {
        /** The vehicle with the vehicle_part (like a kitchen). */
        vehicle *veh;
        /** The relative coords (mount_dx/mount_dy) of the vehicle_part*/
        int mount_dx;
        int mount_dy;
        /** the pseudo-item of the vehicle_part (like water_clean from kitchen) */
        item the_item;
        item_from_vpart(vehicle *v, int x, int y, const item &i) : veh(v), mount_dx(x), mount_dy(y), the_item(i) { }
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
        tripoint position;
        /** The pseudo-item representing something (like water or fire) */
        item the_item;
        item_from_surrounding(const tripoint &p, const item &i) : position(p), the_item(i) { }
    };


    // Where (abstract) the item is (on the map, in a car, a bionic, ...)
    typedef enum {
        LT_MAP            = (1 <<  0),
        LT_SURROUNDING    = (1 <<  1),
        LT_VEHICLE_CARGO  = (1 <<  2),
        LT_VPART          = (1 <<  3),
        LT_INVENTORY      = (1 <<  5),
        LT_BIONIC         = (1 <<  6),
    } LocationType;
    /**
     * Theses bit flags corrospond to the sources of item (e.g.
     * #by_bionic or #on_map)
     * This includes all of LocationType, but only those 3
     * listed values are currently used as input to functions.
     * The functions themself use the flags defined in LocationType.
     * So this values are only shortcuts.
     */
    typedef enum {
        S_MAP = LT_MAP | LT_SURROUNDING | LT_VEHICLE_CARGO | LT_VPART,
        S_PLAYER = LT_INVENTORY | LT_BIONIC,
        
        S_ALL = S_MAP | S_PLAYER,
    } source_flags;
    
    typedef enum {
        C_CHARGES,
        C_AMOUNT,
    } count_type;
    
    struct candidate_t;
    
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
        /**
         * How to count the requirement (by charges or by amount)
         */
        count_type ctype;
        /**
         * What is required, may be a real type like "rag" or "welder",
         * or a functionality like "func:welder".
         */
        itype_id type;
        /**
         * How much is needed, must be > 0.
         */
        long count;

        requirement(const itype_id &t, long c, count_type ct) : ctype(ct), type(t), count(c) { }
        requirement(const component &comp, bool as_tool);
        
        /**
         * Get how much the item counts towards the requirement.
         * An item that is invcompatible does not count at all
         * (returns 0). For compatible items (item.type == required type)
         * it return either 1 if counted by amount or the charges
         * of the item if counted by charges.
         * This is recursive and checks the content of the items too.
         */
        long get_charges_or_amount(const item &the_item) const;
        /**
         * Use up the mathcing items up to #count.
         * This also decreases the #count (not below 0).
         * The used up items are stored in used_items.
         * This is recursive and checks the content of the items too.
         * @return tru if the item should delete (charges==0, or
         * ctype==C_AMOUNT and item was used up).
         */
        bool use(item &it, std::list<item> &used_items);
       
        long operator()(const item &the_item) const {
            return get_charges_or_amount(the_item);
        }
        long operator()(const candidate_t &candidate) const;
    };
    
    /**
     * This represents a specifc item (might be a pseudo-item),
     * that can inidiviualy used.
     */
    struct candidate_t {
    private:
        friend struct requirement;
        LocationType location;
        union {
            struct {
                items_on_map *mapitems;
                int mindex;
            };
            struct {
                player *the_player;
                // position of item in player's inventory (or worn, weapon has its own location)
                int invpos;
                int invcount;
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
        // type requested by the requirement_data or similar, might not be the same
        // as the type of the actuall item.
        itype_id usageType;
        
        LocationType getLocation() const { return location; }
        
        // These constructors initialize the object
        // and set the location member.
        // The candidate is valid (valid() == true) after
        // beeing constrcuted this way.
        candidate_t() { make_invalid(); }
        candidate_t(items_on_map &ifm, int i, const itype_id &type);
        candidate_t(player *p, int pos, int count, const itype_id &type);
        candidate_t(items_in_vehicle_cargo &ifv, int i, const itype_id &type);
        candidate_t(item_from_vpart &ifv, const itype_id &type);
        candidate_t(item_from_bionic &ifb, const itype_id &type);
        candidate_t(item_from_surrounding &ifs, const itype_id &type);

        // Create a candidate by deserizing the date, the
        // created item _might_ not be valid.
        // Always use valid() after his to check.
        candidate_t(crafting_inventory_t &cinv, JsonObject &obj) { deserialize(cinv, obj); }
            
        // used to sort these items for the user
        bool operator<(const candidate_t &other) const;
        /**
         * Returns a string representing the candidate. It is
         * supposed to be for the user to see this.
         * @return A string containing the item name (item::tname)
         * and a hint to the location ("nearby").
         * @param withTime Include the time modifier (if any) - this
         * is only usefull for tools.
         */
        std::string to_string(bool withTime) const;
        /**
         * Check if this candidate is part of the sources specified
         * as source_flags.
         */
        bool is_source(source_flags sources) const {
            return (location & sources) != 0;
        }
        /**
         * Check if this object is correctly setup and points to a
         * proper item.
         * This should be checked after deserialize.
         */
        bool valid() const;
        /**
         * Get a reference to the original item that is represented here.
         * This does _not_ return #the_item, but a reference to the
         * original, e.g. on the map or in the player inventory.
         * @return A reference to an item on the map, or in the player
         * inventory or one of the pseudo items only existing inside
         * crafting_inventory_t. Don't change this item directly.
         */
        const item &get_item() const;

        float get_time_modi() const;
        
        // Serialize into v, overriding any existing content.
        void serialize(JsonOut &json) const;
        
        /**
         * Split part of this candidate into another one.
         * Check valid() afterwards for the result.
         * If count_to_remove is too big or the candidate can not be
         * splitted, the return might be invlid.
         * The this objects stays always valid.
         * The requirement is used to check if count_to_remove
         * means charges or amounts.
         */
        candidate_t split(const requirement &req, int count_to_remove);
        
        void consume(player *p, requirement &req, std::list<item> &ret) const;
    private:
        void deserialize(crafting_inventory_t &cinv, JsonObject &obj);
        void make_invalid() { location = LT_BIONIC; bionic = NULL; }
    };
    typedef std::vector<candidate_t> candvec;
    
    class solution;
    class complex_req;
    /**
     * A sinle requirement. This is always of an actuall type,
     * never of a functionality. Single referces to the number of
     * item types (which is one for this requirement), not to the
     * amount.
     */
    class simple_req {
    public:
        /**
         * The single item-type requirement.
         */
        requirement req;
        /**
         * Points to the component object that is stored
         * in a requirement_data.
         */
        const component *comp;
        /**
         * Indicates if this requirement is fullfilled.
         */
        available_status available;
        complex_req *parent;
        std::vector<simple_req*> overlays;
        
        long cnt_on_map;
        long cnt_on_player;
        candvec candidate_items;
        
        simple_req(const requirement &r, const component *c, complex_req *p = 0) : req(r), comp(c), parent(p) { }
        
        bool is_possible() const;
        
        /**
         * Move items from candidate_items of this to the other
         * to fullfill the requirement of the other simple_req.
         * call recount_candidate_sources for both at the end.
         * Note: the candidate_items of the other are
         * _not_ erased but included in the requirement check.
         */
        void move_as_required_to(simple_req &other);
        
        typedef enum {
            ts_normal      = (0 <<  0),
            ts_found_items = (1 <<  1), // list found items too
            ts_compress    = (1 <<  2), // list only the complex_req.
            ts_overlays    = (1 <<  3), // allways list overlays
            ts_selected    = (1 <<  4), // list the selected items
        } to_string_flags;
        std::string to_string(int flags = ts_normal) const;
    protected:
        friend class solution;
        friend class complex_req;
        
        /**
         * Counts cnt_on_map and cnt_on_player from current
         * content of candidate_items.
         */
        void recount_candidate_sources();
        /**
         * Checks simply by comparing the sum of cnt_* with
         * req.count
         */
        bool req_is_fullfilled() const {
            return cnt_on_map + cnt_on_player >= req.count;
        }
        
        // Called during solution::gather
        void gather(crafting_inventory_t &cinv, bool store);
        
        // Called during solution::init
        void find_overlays(simple_req &rc);
        /**
         * Possible merge the other requirement into this->req,
         * assumes that the type is equal, fails if
         * the ctype of the other req. is different (returns false).
         * Otherwise adds the count and return true.
         */
        bool merge(const requirement &otherReq);
        void set_unavailable(available_status av = a_false);
        void separate(simple_req &other);
        void check_overlay(crafting_inventory_t &cinv, bool store, simple_req &other);
    };

    class complex_req {
    public:
    protected:
        typedef std::vector<simple_req> SimpReqVec;
        /**
         * Input, a list of single requirement_data.
         */
        SimpReqVec simple_reqs;
        /**
         * Input, is this input used as tools?
         * Tools are sometimes differntly handled than components.
         * Tools are not destroyed like components, but charges
         * of the tools might be consumed.
         * Also the amount of a required tool is always 1.
         */
        bool as_tool;
        /**
         * Output, which items the user selected that should be used
         */
        candvec selected_items;
        /**
         * Index into #simple_reqs of which the user choose to use.
         */
        int selected_simple_req_index;
        double toolfactor;
    public:
        complex_req() : selected_simple_req_index(-1) { }
        
        void select_items_to_use();
        
        bool is_possible() const;
        
        bool contains_req_type(const itype_id &type) const;
        simple_req *get_req_type(const itype_id &type);
        
        std::string to_string(int flags = simple_req::ts_normal) const;
        /**
         * Is this complex requirement fullfilled even when
         * ignoring the given simple_req?
         * sr must be part of simple_reqs!
         */
        bool has_alternativ(const simple_req &sr) const;
        
        // Serialize to json string
        std::string serialize() const;
        // Deserialize from json
        void deserialize(crafting_inventory_t &cinv, JsonArray &arr);
    protected:
        friend class solution;
        friend class simple_req;
        
        // Called during solution::gather
        void gather(crafting_inventory_t &cinv, bool store);
        
        // Called during solution::init
        template<typename T>
        void init(const std::vector<T> &components, solution &s);
        template<typename T>
        void init(const T &components, solution &s);
        void add(const component &c);
        void add_or_merge(const simple_req &rs2);
        void init_pointers();
        void consume(crafting_inventory_t &cinv, std::list<item> &used_items);
    };

    /**
     * Usage:
     * 1. make an instance
     * 2. init the instance (use exactly ONE init*-function)
     * 3. gather possible matching items, this sets up what
     * existing items can be used and if a component is available.
     * 4. either check that all components (or alternatives) are
     * available with is_possible() and be done.
     * 4. OR select which items should actually be used (this
     * may prompt the user for some chooses) with select_items_to_use.
     * 5. (Only do this after select_items_to_use) serialize the
     * choosen items with #serialize. To deserialize this call
     * deserialize after init (and skip gather and select_items_to_use).
     * 6. After either deserialize or select_items_to_use one may
     * call consume to actually use up the items.
     */
    class solution {
    public:
        typedef std::vector<complex_req> CompReqVec;
        CompReqVec complex_reqs;
        double toolfactor;
        
        solution() { }
        /**
         * Calls init_single_req and gather
         */
        template<typename T>
        solution(const T &comp, crafting_inventory_t &cinv);
        /**
         * Calls init_need_any and gather
         */
        template<typename T>
        solution(const std::vector<T> &comp, crafting_inventory_t &cinv);
        
        void serialize(player_activity &activity) const;
        void deserialize(crafting_inventory_t &cinv, player_activity &activity);
        
        void init(const requirement_data &making);
        /**
         * Init this as if from a requirement_data that contains only one
         * complex requirement stored in comps.
         * @param as_tool If this requirement means tools or components.
         */
        template<typename T>
        void init_need_any(const std::vector<T> &comps);
        /**
         * Init this as if from a requirement_data that contains only one
         * complex requirement stored in comp.
         * @param as_tool If this requirement means a tool or a component.
         */
        template<typename T>
        void init_single_req(const T &comp);
        
        /**
         * Sets up which actuall items to use. This may ask the user for
         * input.
         */
        void select_items_to_use();
        /**
         * Find out which items of the crafting_inventory_t can be used.
         * Must only be called after init.
         * @param store if true, stores all possible items in the
         * #candidate_items vector in simple_req, other wise only the
         * count is stored.
         */
        void gather(crafting_inventory_t &cinv, bool store);
        /**
         * Check if all requirement_data are fullfilled.
         * Must only be called after gather.
         */
        bool is_possible() const;
        /**
         * Use up the selected items (selected_items in complex_req).
         * Must only be called after select_items_to_use or after deserialize.
         */
        void consume(crafting_inventory_t &cinv, std::list<item> &used_items, std::list<item> &used_tools);
        void consume(crafting_inventory_t &cinv, std::list<item> &used);
        
        std::string to_string(int flags = simple_req::ts_normal) const;
        /**
         * Write the requirement_data back into a requirement_data. Only the tools and
         * the components vaector of the requirement_data are overriden.
         */
        void save(requirement_data &making) const;
    protected:
        void find_overlays();
        void erase_empty_reqs();
        void init_pointers();
    };
    
protected:
    static bool has_different_types(const candvec &cv);
    // check if all items int he vector are actually equal (same type, same damage, ...)
    // basicly they are equal if the user would considered them equall.
    static bool all_equal(const candvec &cv);
    // removes all entries in cv, that are not part of the specified sources
    static void filter(candvec &cv, source_flags sources);

    int collect_candidates(const requirement &req, int source, candvec &result);
    
    friend candidate_t; // for deserialize
    friend solution;

    player *p;
    std::list<items_on_map> on_map;
    std::list<items_in_vehicle_cargo> in_veh;
    std::list<item_from_vpart> vpart;
    std::list<item_from_bionic> by_bionic;
    std::list<item_from_surrounding> surround;
    
    /**
     * Adds a item_from_vpart object to the vpart list.
     */
    void add_vpart(vehicle *veh, int part, const std::string &vpart_flag_name, const ammotype &fuel);
    
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
    int count(const requirement &req, int max, int sources) const;
    
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
    void form_from_map(tripoint position, int distance);
    
    typedef enum { assume_components, assume_tools } consume_flags;
    int consume(const std::vector<component> &x, consume_flags flags, std::list<item> &used_items);
    
    /**
     * Consume the items to fullfill the requirement.
     * If more items are given only the first few are consumed
     * until the requirement is fullfilled.
     */
    void consume(requirement req, consume_flags flags, const candvec &selected_items, std::list<item> &used_items);
    
    /**
     * Ask the user to choose a single item from the list of candidates.
     * This function tries to avoid the user interaction if possible,
     * (e.g. only on candidate item, or all are equally good, or one is
     * superior to all the others, ...)
     * @param candidates The items the user can choose from - must not be empty!.
     * @return The selected candidate
     */
    static candidate_t ask_for_single_item(const requirement &req, consume_flags flags, const candvec &candidates);
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
    static void ask_for_items_to_use(const requirement &req, consume_flags flags, const candvec &candidates, candvec &selected_candidates);
    
    /**
     * Wrapper for ask_for_items_to_use, that stores the selected
     * items inthe input list.
     */
    static void ask_for_items_to_use(const requirement &req, consume_flags flags, candvec &candidates);
    
    /**
     * Reduce the list of candidates but still match the requirement.
     * This function removes candidates from the end of the list.
     */
    static void reduce(requirement req, candvec &candidates);
    
    /**
     * Ask the user to select items according to the requirement_data in
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
    int select_items_to_use(const std::vector<component> &components, consume_flags flags, candvec &selected_items);
    
    static std::string serialize(int index, const candvec &vec);
    int deserialize(JsonArray &arr, candvec &vec);
    
    /**
     * Given a list of tools and their usage (type),
     * calculates the time modi for each tool and returns the worst
     * modi.
     * @param tools The tools that will be used. All of them will be
     * used. The list may be empty.
     * @param type The type of tool that was requested (e.g. by a requirement_data).
     * This might not be the same type as each tools. And each tool might
     * have different time modififactions based on what it is used for.
     * E.g. a hammer is fast for hammering, but not so fast for prying.
     * @return The time modi. A value > 1 means it takes longer than normal,
     * a value < 1 means it is faster. A value of 0.0 means no matching
     * tool was found or the tools have all a modi of 1.0. Caller should
     * expect a value of 0.0. In most cases this means simply to ignore
     * these tools for calculation of the modi.
     */
    static double calc_time_modi(const candvec &tools);
    static void merge_time_modi(double modi, double &result);
    
    void gather_inputs(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec, double *timeModi);
    void gather_inputs(const std::vector<component> &cv, consume_flags flags, std::vector<std::string> &strVec, double *timeModi);

    void consume_gathered(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec, std::list<item> &used_items);

    /**
     * Check if this tool/component is available (checks charges if needed),
     * sets component::available to -1 or +1
     * @param as_tool Assume the component represents a tool.
     * Tools are always counted by charge or (if the required count
     * is negativ) by amount (in this case one item is always enough).
     * @return true if the tool/component is available.
     */
    bool has(component &x, bool as_tool) const;

    /**
     * See #crafting_inventory_t(player*, int)
     * This is the wrapper that does what is descripted there.
     */
    void init(int range);
public:
    /**
     * Same as <code>crafting_inventory_t(g, p, PICKUP_RANGE)</code>
     */
    crafting_inventory_t(player *p);
    /**
     * Create an inventory wrapper for the given player.
     * The position of the player is used to add items from the map,
     * surrounding him.
     * @param range The pickup range, items (anf vehicle parts) that
     * are inside this range are added to this inventory.
     * If range is -1, no items from the map or from vehicles are added
     * at all.
     */
    crafting_inventory_t(player *p, int range);
    
    // Interface for requirement_datas: check that the requirement_data is possible
    /**
     * Disspatch everything to the solution class.
     */
    bool has_all_requirements(const requirement_data &making);
    bool has_all_requirements(const requirement_data &making, int batch_size);
    bool has_all_requirements(const requirement_data &making, solution &s);
    
    // Interface for requirement_datas: gather what tools and components to use
    /**
     * Lets the user select which components/tools to use and
     * stores the selected ones in the activity.
     * It also modifies the activity time according to the tools
     * used and their time modi.
     * @param activity The selected tools / components are stored
     * here (in str_values), also the turns left for this activity
     * might be changed.
     */
    void gather_input(const requirement_data &making, player_activity &activity);
    void gather_input(const requirement_data &making, player_activity &activity, int batch_size);
    void gather_input(const requirement_data &making, solution &s, player_activity &activity);
    /**
     * Loads the tools/components that the user selected in gather_input,
     * and consumes them.
     * This function uses the requirement_data to ask the user again if any of
     * the input items has vanished (e.g. a fire went out).
     * @param used_items The list of all the used_up items
     * or the used up charges. These are actuall items.
     * @param used_tools The list of all the used tools,
     * does not include the used up tools. This might include functionality
     * items (e.g. "func:welder"). The list should be used when
     * dissabmling the item.
     */
    void consume_gathered(
        const requirement_data &making,
        player_activity &activity,
        std::list<item> &used_items,
        std::list<item> &used_tools
    );
    void consume_gathered(
        const requirement_data &making,
        player_activity &activity,
        int batch_size,
        std::list<item> &used_items,
        std::list<item> &used_tools
    );
    void consume_gathered(
        const requirement_data &making,
        solution &s,
        player_activity &activity,
        std::list<item> &used_items,
        std::list<item> &used_tools
    );
    
    void gather_and_consume(
        const requirement_data &making,
        std::list<item> &used_items,
        std::list<item> &used_tools
    );
    void gather_and_consume(
        const requirement_data &making,
        solution &s,
        std::list<item> &used_items,
        std::list<item> &used_tools
    );

    /**
     * Basicly the interface to crafting (and construction, and vehicle
     * repair/install).
     * This function checks that the components needed are all available.
     * See the #has_any and #has.
     * Note: component::available is set according to the available
     * state (-1, 0 or 1).
     */
    
    // Check that at least one component is available.
    bool has_any_components(const std::vector<item_comp> &comps) const;
    // Check that the single component is available
    bool has_components(const item_comp &comps) const;
    // Also provide a singular form (has_component vs has_component_s_)
    bool has_component(const item_comp &comps) const { return has_components(comps); }
    // Check that the single component is available
    bool has_components(const itype_id &type, int count) const;
    // Also provide a singular form (has_component vs has_component_s_)
    bool has_component(const itype_id &type, int count) const { return has_components(type, count); }
    
    /**
     * Same as has_all_components, only the input is assumed to be
     * tools and their charges (their must be a tool with that charge)
     * or simple tools requirement_data (without charge) which means the
     * tool itself must be available.
     */
    
    bool has_any_tools(const std::vector<tool_comp> &tools) const;
    bool has_tools(const tool_comp &tool) const;
    bool has_tool(const tool_comp &tool) const { return has_tools(tool); }
    bool has_tools(const itype_id &type, int count) const;
    bool has_tool(const itype_id &type, int count) const { return has_tools(type, count); }
    bool has_tools(const itype_id &type) const;
    bool has_tool(const itype_id &type) const { return has_tools(type); }
    
    
    
    /**
     * Check to se if the requirement is fullfilled by one of
     * the given sources.
     * @param sources a combination (or-ed) of source_flags.
     */
    bool has(const requirement &req, source_flags sources = S_ALL) const;
    /**
     * Returns the count of items/charges (as defined by the
     * requirement) that are fullfilled (might be smaller than
     * the actuall required).
     * @param sources a combination (or-ed) of source_flags. Only
     * items from the listed sources are considered.
     */
    int count(const requirement &req, source_flags sources = S_ALL) const;
    
    // Consume items
    /**
     * Consume components.
     * @return the consumed items.
     */
    std::list<item> consume_any_components(const std::vector<item_comp> &comps);
    std::list<item> consume_components(const item_comp &comps);
    std::list<item> consume_component(const item_comp &comps) { return consume_components(comps); }
    std::list<item> consume_components(const itype_id &type, int count);
    std::list<item> consume_component(const itype_id &type, int count) { return consume_components(type, count); }
    
    // Consume tools
    /**
     * Consume tool charges, if no charges have been required, nothing
     * is changed.
     */
    std::list<item> consume_any_tools(const std::vector<tool_comp> &tools);
    std::list<item> consume_tools(const tool_comp &tools);
    std::list<item> consume_tool(const tool_comp &tool) { return consume_tools(tool); }
    std::list<item> consume_tools(const itype_id &type, int charges);
    std::list<item> consume_tool(const itype_id &type, int charges) { return consume_tools(type, charges); }
    std::list<item> consume_tools(const itype_id &type);
    std::list<item> consume_tool(const itype_id &type) { return consume_tools(type); }
    
    
    // Check requirement_data, avoid these two functions, use
    // has_tools and has_items instead.
    // The has_tools and has_components handle counting by charges and
    // counting by amount better.
    bool has_amount(const itype_id &the_item, int quantity) const {
        return has(requirement(the_item, quantity, C_AMOUNT));
    }
    bool has_charges(const itype_id &the_item, int quantity) const {
        return has(requirement(the_item, quantity, C_CHARGES));
    }
    
    
    
    
    // FIXME remove this, change in calling code
    void consume_items(const std::vector<component> &comps);
    void consume_items(const std::vector<item_comp> &comps);
    
    /**
     * This is similar to consume_components, but is used in vehilce
     * interaction.
     */
    item consume_vpart_item(const std::string &vpid);
    /**
     * FIXME: legacy, should be changed to "func:" system
     */
    bool has_items_with_quality(const std::string &name, int level, int amount) const;
    
    void add_surround(const tripoint &p, const item &it);
private:
    typedef std::map<itype_id, int> CacheMap;
    mutable CacheMap counted_by_charges;
    mutable CacheMap counted_by_amount;
};

#endif
