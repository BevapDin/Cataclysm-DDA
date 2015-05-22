#ifndef ADVANCED_INV_H
#define ADVANCED_INV_H

#include "output.h"
#include "enums.h"

#include <string>
#include <array>

class uimenu;
class vehicle;
class item;

typedef std::vector< std::pair<item *, int> > itemslice;

enum aim_location {
    AIM_INVENTORY = 0,
    AIM_SOUTHWEST,
    AIM_SOUTH,
    AIM_SOUTHEAST,
    AIM_WEST,
    AIM_CENTER,
    AIM_EAST,
    AIM_NORTHWEST,
    AIM_NORTH,
    AIM_NORTHEAST,
    AIM_DRAGGED,
    AIM_ALL,
    AIM_CONTAINER,
    AIM_WORN,
    NUM_AIM_LOCATIONS
};

enum advanced_inv_sortby {
    SORTBY_NONE,
    SORTBY_NAME,
    SORTBY_WEIGHT,
    SORTBY_VOLUME,
    SORTBY_CHARGES,
    SORTBY_CATEGORY,
    SORTBY_DAMAGE
};

struct advanced_inv_listitem;

/**
 * Defines the source of item stacks.
 */
struct advanced_inv_area {
    const aim_location id;
    // Used for the small overview 3x3 grid
    const int hscreenx = 0;
    const int hscreeny = 0;
    // relative (to the player) position of the map point
    tripoint off;
    /** Long name, displayed, translated */
    const std::string name = "fake";
    /** Shorter name (2 letters) */
    const std::string shortname = "FK"; // FK in my coffee
    // absolute position of the map point.
    tripoint pos;
    /** Can we put items there? Only checks if location is valid, not if
        selected container in pane is. For full check use canputitems() **/
    bool canputitemsloc;
    // vehicle pointer and cargo part index
    vehicle *veh;
    int vstor;
    // description, e.g. vehicle name, label, or terrain
    std::string desc;
    // flags, e.g. FIRE, TRAP, WATER
    std::string flags;
    // total volume and weight of items currently there
    int volume, weight;
    // maximal count / volume of items there.
    int max_size, max_volume;

    advanced_inv_area( aim_location id ) : id( id ) {}
    advanced_inv_area( aim_location id, int hscreenx, int hscreeny, tripoint off, std::string name, std::string shortname ) :
        id( id ), hscreenx( hscreenx ), hscreeny( hscreeny ), off( off ), name( name ), shortname( shortname ),
        pos(0, 0, 0), canputitemsloc( false ), veh( nullptr ), vstor( -1 ), desc( "" ), volume( 0 ), weight( 0 ), max_size( 0 ), max_volume( 0 )
    {
    }

    void init();
    // if you want vehicle cargo, specify so via `in_vehicle'
    int free_volume(bool in_vehicle = false) const;
    int get_item_count( bool in_vehicle = false ) const;
    // Other area is actually the same item source, e.g. dragged vehicle to the south and AIM_SOUTH
    bool is_same( const advanced_inv_area &other ) const;
    // does _not_ check vehicle storage, do that with `can_store_in_vehicle()' below
    bool canputitems( const advanced_inv_listitem *advitem = nullptr );
    // if you want vehicle cargo, specify so via `in_vehicle'
    item* get_container(bool in_vehicle = false);
    void set_container( const advanced_inv_listitem *advitem );
    bool is_container_valid( const item *it ) const;
    void set_container_position();
    aim_location offset_to_location() const;
    bool can_store_in_vehicle() const
    {
        // disallow for non-valid vehicle locations
        if(id > AIM_DRAGGED || id < AIM_SOUTHWEST) {
            return false;
        }
        return (veh != nullptr && vstor >= 0);
    }
};

// see item_factory.h
class item_category;


/**
 * Entry that is displayed in a adv. inv. pane. It can either contain a
 * single item or a category header or nothing (empty entry).
 * Most members are used only for sorting.
 */
struct advanced_inv_listitem {
    /**
     * Index of the item in the original storage container (or inventory).
     */
    int idx;
    /**
     * The location of the item, never AIM_ALL.
     */
    aim_location area;
    /**
     * The actual item, this is null, when this entry represents a category header.
     */
    item *it;
    /**
     * The displayed name of the item/the category header.
     */
    std::string name;
    /**
     * Name of the item (singular) without damage (or similar) prefix, used for sorting.
     */
    std::string name_without_prefix;
    /**
     * Whether auto pickup is enabled for this item (based on the name).
     */
    bool autopickup;
    /**
     * The stack count represented by this item, should be >= 1, should be 1
     * for anything counted by charges.
     */
    int stacks;
    /**
     * The volume of all the items in this stack, used for sorting.
     */
    int volume;
    /**
     * The weight of all the items in this stack, used for sorting.
     */
    int weight;
    /**
     * The item category, or the category header.
     */
    const item_category *cat;
    /**
     * Is the item stored in a vehicle?
     */
    bool from_vehicle;
    /**
     * Whether this is a category header entry, which does *not* have a reference
     * to an item, only @ref cat is valid.
     */
    bool is_category_header() const;
    /**
     * Whether this is an item entry (where @ref it is a valid pointer).
     */
    bool is_item_entry() const;
    /**
     * Create a category header entry.
     * @param cat The category reference, must not be null.
     */
    advanced_inv_listitem(const item_category *cat);
    /**
     * Creates an empty entry, both category and item pointer are null.
     */
    advanced_inv_listitem();
    /**
     * Create a normal item entry.
     * @param an_item The item pointer, stored in @ref it. Must not be null.
     * @param index The index, stored in @ref idx.
     * @param count The stack size, stored in @ref stacks.
     * @param area The source area, stored in @ref area. Must not be AIM_ALL.
     * @param from_vehicle Is the item from a vehicle cargo space?
     */
    advanced_inv_listitem(item *an_item, int index, int count, 
            aim_location area, bool from_vehicle);
};

/**
 * Displayed pane, what is shown on the screen.
 */
class advanced_inventory_pane
{
    private:
        aim_location area = NUM_AIM_LOCATIONS;
        // pointer to the square this pane is pointing to
        bool viewing_cargo = false;
    public:
        // set the pane's area via its square, and whether it is viewing a vehicle's cargo
        void set_area(advanced_inv_area &square, bool in_vehicle_cargo = false)
        {
            area = square.id;
            viewing_cargo = square.can_store_in_vehicle() && in_vehicle_cargo;
        }
        aim_location get_area() const
        {
            return area;
        }
        bool in_vehicle() const
        {
            return viewing_cargo;
        }
        /**
         * Index of the selected item (index of @ref items),
         */
        int index;
        advanced_inv_sortby sortby;
        WINDOW *window;
        std::vector<advanced_inv_listitem> items;
        /**
         * The current filter string.
         */
        std::string filter;
        /**
         * Whether to recalculate the content of this pane.
         * Implies @ref redraw.
         */
        bool recalc;
        /**
         * Whether to redraw this pane.
         */
        bool redraw;

        void add_items_from_area(advanced_inv_area &square, bool vehicle_override = false);
        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        /**
         * @param it The item to check, oly the name member is examined.
         * @return Whether the item should be filtered (and not shown).
         */
        bool is_filtered(const advanced_inv_listitem &it) const;
        /**
         * Same as the other, but checks the real item.
         */
        bool is_filtered(const item *it) const;
        /**
         * Scroll @ref index, by given offset, set redraw to true,
         * @param offset Must not be 0.
         */
        void scroll_by(int offset);
        /**
         * Scroll the index in category mode by given offset.
         * @param offset Must be either +1 or -1
         */
        void scroll_category(int offset);
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        advanced_inv_listitem *get_cur_item_ptr();
        /**
         * Set the filter string, disables filtering when the filter string is empty.
         */
        void set_filter( const std::string &new_filter );
        /**
         * Insert additional category headers on the top of each page.
         */
        void paginate( size_t itemsPerPage );
    private:
        /** Scroll to next non-header entry */
        void skip_category_headers(int offset);
        /** Only add offset to index, but wrap around! */
        void mod_index(int offset);

        mutable std::map<std::string, bool> filtercache;
};

class advanced_inventory
{
    public:
        advanced_inventory();
        ~advanced_inventory();

        void display();
    private:
        // current location of AIM_ALL move_all_items() transfer
//        int aim_all_location = 0;
        // stores items from both map and vehicle for AIM_ALL transfers
//        std::map<int, std::list<item>> veh_items, map_items;
        // swap the panes and WINDOW pointers via std::swap()
        void swap_panes();
        /**
         * Refers to the two panels, used as index into @ref panels.
         */
        enum side {
            left  = 0,
            right = 1,
            NUM_PANES = 2
        };
        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        // minimap that displays things around character
        WINDOW *minimap, *mm_border;
        const int minimap_width  = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();

        bool inCategoryMode;

        int itemsPerPage;
        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;
        /**
         * Which panels is active (item moved from there).
         */
        side src;
        /**
         * Which panel is the destination (items want to go to there).
         */
        side dest;
        /**
         * True if (and only if) the filter of the active panel is currently
         * being edited.
         */
        bool filter_edit;
        /**
         * Two panels (left and right) showing the items, use a value of @ref side
         * as index.
         */
        std::array<advanced_inventory_pane, NUM_PANES> panes;
        static const advanced_inventory_pane null_pane;
        std::array<advanced_inv_area, NUM_AIM_LOCATIONS> squares;

        WINDOW *head;
        WINDOW *left_window;
        WINDOW *right_window;

        bool exit;

        // store/load settings (such as index, filter, etc)
        void save_settings(bool only_panes);
        void load_settings();
        void do_return_entry();

        static std::string get_sortname(advanced_inv_sortby sortby);
        bool move_all_items();
        void print_items(advanced_inventory_pane &pane, bool active);
        void recalc_pane(side p);
        void redraw_pane(side p);
        // Returns the x coordinate where the header started. The header is
        // displayed right right of it, everything left of it is till free.
        int print_header(advanced_inventory_pane &pane, aim_location sel);
        void init();
        aim_location find_destination(const advanced_inv_listitem &it, bool &in_vehicle);
        /**
         * Translate an action ident from the input context to an aim_location.
         * @param ret If the action ident referred to a location, its id is stored
         * here. Only valid when the function returns true.
         * @return true if the action did refer to an location (which has been
         * stored in ret), false otherwise.
         */
        static bool get_square(const std::string action, aim_location &ret);
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu(advanced_inventory_pane &pane);
        /**
         * Checks whether one can put items into the supplied location.
         * If the supplied location is AIM_ALL, query for the actual location
         * (stores the result in def) and check that destination.
         * @return false if one can not put items in the destination, true otherwise.
         * The result true also indicates the def is not AIM_ALL (because the
         * actual location has been queried).
         */
        bool query_destination(aim_location &def);
        /**
         * Add the item to the destination area.
         * @param destarea Where add the item to. This must not be AIM_ALL.
         * @param new_item The item to add.
         * @param inv_item Pointer-pointer for the inventory's item pointer, if applicable.
         * @return true if adding has been done, false if adding the item failed.
         */
        bool add_item( aim_location destarea, bool in_vehicle, item &new_item );
        /**
         * Move content of source container into destination container (destination pane = AIM_CONTAINER)
         * @param src_container Source container
         * @param dest_container Destination container
         */
        bool move_content(item &src, item &dest);
        /**
         * Setup how many items/charges (if counted by charges) should be moved.
         * @param destarea Where to move to. This must not be AIM_ALL.
         * @param sitem The source item, it must contain a valid reference to an item!
         * @param amount The input value is ignored, contains the amount that should
         * be moved. Only valid if this returns true.
         * @return false if nothing should/can be moved. True only if there can and
         * should be moved. A return value of true indicates that amount now contains
         * a valid item count to be moved.
         */
        bool query_charges(aim_location destarea, bool in_veh, const advanced_inv_listitem &sitem, bool askamount, long &amount );
        /**
         * Remove the item from source area. Must not be used on items with area
         * AIM_ALL or AIM_INVENTORY! (but is... and seems to work)
         * @param sitem The item reference that should be removed, along with the
         * source area.
         */
        void remove_item( advanced_inv_listitem &sitem );
        void menu_square(uimenu *menu);

        static char get_location_key( aim_location area );
};

#endif
