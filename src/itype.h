#ifndef _ITYPE_H_
#define _ITYPE_H_

#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"
#include "rng.h"
#include "material.h"
#include "mtype.h"

#include <string>
#include <vector>
#include <set>
#include <bitset>
#include <memory>

// for use in category specific inventory lists
enum item_cat {
    IC_NULL = 0,
    IC_COMESTIBLE,
    IC_AMMO,
    IC_ARMOR,
    IC_GUN,
    IC_BOOK,
    IC_TOOL,
    IC_CONTAINER
};

struct recipe;

typedef std::string itype_id;
extern std::vector<std::string> artifact_itype_ids;
extern std::vector<std::string> standard_itype_ids;

// see item_factory.h
class item_category;

struct itype;
extern std::map<std::string, itype *> itypes;

typedef std::string ammotype;

enum software_type {
    SW_NULL,
    SW_USELESS,
    SW_HACKING,
    SW_MEDICAL,
    SW_SCIENCE,
    SW_DATA,
    NUM_SOFTWARE_TYPES
};

enum bigness_property_aspect {
    BIGNESS_ENGINE_DISPLACEMENT, // combustion engine CC displacement
    BIGNESS_WHEEL_DIAMETER,      // wheel size in inches, including tire
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. ""00_shot"")
itype_id default_ammo(ammotype guntype);

struct explosion_data {
    // Those 4 values are forwarded to game::explosion.
    int power;
    int shrapnel;
    bool fire;
    bool blast;
    explosion_data()
    : power( -1 )
    , shrapnel( 0 )
    , fire( false )
    , blast( true )
    {
    }
};

struct islot_container {
    /**
     * Volume, scaled by the default-stack size of the item that
     * is contained in this container.
     */
    int contains;
    /**
     * Can be sealed so it's usable to transport liquids (and not just hold them)
     */
    bool seals;
    /**
     * Is watertight - can be used to store liquids.
     */
    bool watertight;

    islot_container()
    : contains( 0 )
    , seals( false )
    , watertight( false )
    {
    }
};

struct islot_variable_bigness {
    /**
     * Minimal value of the bigness value of items of this type.
     */
    int min_bigness;
    /**
     * Maximal value of the bigness value of items of this type.
     */
    int max_bigness;
    /**
     * What the bigness actually represent see @ref bigness_property_aspect
     */
    bigness_property_aspect bigness_aspect;

    islot_variable_bigness()
    : min_bigness( 0 )
    , max_bigness( 0 )
    , bigness_aspect( BIGNESS_ENGINE_DISPLACEMENT )
    {
    }
};

struct islot_bionic {
    /**
     * Arbitrary difficulty scale, see bionics.cpp for its usage.
     */
    int difficulty;
    /**
     * Id of the bionic, see @ref bionics (global std:map).
     */
    std::string bionic_id;

    islot_bionic()
    : difficulty( 0 )
    , bionic_id()
    {
    }
};

struct islot_stationary {
    /**
     * Category - arbitrary identifier, used to select random descriptions
     * from a list of descriptions. see @ref snippet_library
     */
    std::string snippet_category;

    islot_stationary()
    : snippet_category()
    {
    }
};

struct islot_armor {
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     * TODO: use num_hp or similar instead of magic value
     */
    std::bitset<13> covers;
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     * TODO: use num_hp or similar instead of magic value
     */
    std::bitset<13> sided;
    /**
     * How much this item encumbers the player.
     */
    signed char encumber;
    /**
     * Percentage of the body part area that this item covers.
     * This determines how likely it is to hit the item instead of the player.
     */
    unsigned char coverage;
    /**
     * TODO: document me.
     */
    unsigned char thickness;
    /**
     * Resistance to environmental effects.
     */
    unsigned char env_resist;
    /**
     * How much warmth this item provides.
     */
    signed char warmth;
    /**
     * How much storage this items provides when worn.
     */
    unsigned char storage;
    /**
     * Whether this is a power armor item.
     */
    bool power_armor;

    islot_armor()
    : covers( 0 )
    , sided( 0 )
    , encumber( 0 )
    , coverage( 0 )
    , thickness( 0 )
    , env_resist( 0 )
    , warmth( 0 )
    , storage( 0 )
    , power_armor( false )
    {
    }
};

struct islot_tool {
    /**
     * The ammo that is used to load this tools. Might be "null" in case
     * this tool does not use ammo.
     */
    ammotype ammo;
    /**
     * Maximal amount of charges that this tool can have.
     */
    long max_charges;
    /**
     * TODO: merge with @ref rand_charges
     */
    long def_charges;
    /**
     * A set of default charges of this tool when it's created. Ignores max_charges.
     */
    std::vector<long> rand_charges;
    /**
     * How many charges this tool consumes during a turn when it's active.
     */
    unsigned char charges_per_use;
    /**
     * How many charges this tool consumes when it is activated (it's iuse function
     * is invoked).
     */
    unsigned char turns_per_charge;
    /**
     * When the tool runs out of charges, it will revert to this item type.
     * Can be "null", in that case the item is destroyed.
     */
    itype_id revert_to;
    /**
     * TODO: document me
     */
    itype_id subtype;

    islot_tool()
    : ammo( "null" )
    , max_charges( 0 )
    , def_charges( 0 )
    , rand_charges()
    , charges_per_use( 0 )
    , turns_per_charge( 0 )
    , revert_to( "null" )
    , subtype()
    {
    }
};

struct islot_book {
    /**
     * Which skill it upgrades, if any. Can be NULL.
     * TODO: this should be a pointer to const
     */
    Skill *type;
    /**
     * The value it takes the skill to.
     */
    unsigned char level;
    /**
     * The skill level required to understand it.
     */
    unsigned char req;
    /**
     * How fun reading this is, can be negative.
     */
    signed char fun;
    /**
     * Intelligence required to read, at all.
     */
    unsigned char intel;
    /**
     * How long, in 10-turns (aka minutes), it takes to read.
     * "To read" means getting 1 skill point, not all of them.
     */
    unsigned int time;
    /**
     * Fun books have chapters; after all are read, the book is less fun.
     */
    int chapters;
    /**
     * What recipes can be learned from this book.
     * Key is the recipe, value is TODO
     */
    std::map<recipe *, int> recipes;
    /**
     * Iuse function that is invoked when the book has been read.
     */
    use_function use_method;

    int invoke( player *p, item *it, bool active ) const {
        if( !use_method.is_none() ) {
            use_method.call( p, it, active );
        }
        return 0;
    }

    islot_book()
    : type( nullptr )
    , level( 0 )
    , req( 0 )
    , fun( 0 )
    , intel( 0 )
    , time( 0 )
    , chapters( 0 )
    , recipes()
    , use_method()
    {
    }
};

// TODO: this shares a lot with the ammo item type, merge into a separate slot type?
struct islot_gun {
    /**
     * What type of ammo this gun uses.
     */
    ammotype ammo;
    /**
     * What skill this gun uses.
     * TODO: This is also indicates the type of gun (handgun/rifle/etc.) - that
     * should probably be made explicit.
     * TODO: this should be a pointer to a const Skill
     */
    Skill *skill_used;
    /**
     * Damage bonus from gun.
     */
    int dmg_bonus;
    /**
     * Armor-pierce bonus from gun.
     */
    int pierce;
    /**
     * Range bonus from gun.
     */
    int range;
    /**
     * Dispersion "bonus" from gun.
     */
    int dispersion;
    /**
     * Recoil "bonus" from gun.
     */
    int recoil;
    /**
     * Gun durability, affects gun being damaged during shooting.
     */
    int durability;
    /**
     * Burst size.
     */
    int burst;
    /**
     * Clip size.
     */
    int clip;
    /**
     * Reload time.
     */
    int reload_time;
    /**
     * Effects that are applied to the ammo when fired.
     */
    std::set<std::string> ammo_effects;
    /**
     * Location for gun mods.
     * Key is the location (untranslated!), value is the number of mods
     * that the location can have. The value should be > 0.
     */
    std::map<std::string, int> valid_mod_locations;

    islot_gun()
    : skill_used( nullptr )
    , dmg_bonus( 0 )
    , pierce( 0 )
    , range( 0 )
    , dispersion( 0 )
    , recoil( 0 )
    , durability( 0 )
    , burst( 0 )
    , clip( 0 )
    , reload_time( 0 )
    , ammo_effects()
    , valid_mod_locations()
    {
    }
};

struct islot_gunmod {
    /**
     * TODO: document me
     */
    int dispersion;
    /**
     * TODO: document me
     */
    int damage;
    /**
     * TODO: document me
     */
    int loudness;
    /**
     * TODO: document me
     */
    int clip;
    /**
     * TODO: document me
     */
    int recoil;
    /**
     * TODO: document me
     */
    int burst;
    /**
     * TODO: document me
     */
    ammotype newtype;
    /**
     * TODO: document me
     */
    std::set<std::string> acceptible_ammo_types;
    /**
     * TODO: document me
     */
    bool used_on_pistol;
    /**
     * TODO: document me
     */
    bool used_on_shotgun;
    /**
     * TODO: document me
     */
    bool used_on_smg;
    /**
     * TODO: document me
     */
    bool used_on_rifle;
    /**
     * TODO: document me
     */
    bool used_on_bow;
    /**
     * TODO: document me
     */
    bool used_on_crossbow;
    /**
     * TODO: document me
     */
    bool used_on_launcher;
    /**
     * TODO: document me
     * TODO: this should be a pointer to const Skill.
     */
    Skill *skill_used;
    /**
     * TODO: document me
     */
    std::string location;

    islot_gunmod()
    : dispersion( 0 )
    , damage( 0 )
    , loudness( 0 )
    , clip( 0 )
    , recoil( 0 )
    , burst( 0 )
    , newtype()
    , acceptible_ammo_types()
    , used_on_pistol( false )
    , used_on_shotgun( false )
    , used_on_smg( false )
    , used_on_rifle( false )
    , used_on_bow( false )
    , used_on_crossbow( false )
    , used_on_launcher( false )
    , skill_used( nullptr )
    , location()
    {
    }
};

// TODO: separate this into med/drugs and food/drink - should be separate instead of
// using the comesttype information.
struct islot_comest {
    /**
     * Many things make you thirstier!
     */
    signed int quench;
    /**
     * Nutrition imparted
     */
    unsigned int nutr;
    /**
     * How long it takes to spoil (hours / 600 turns)
     */
    unsigned int spoils;
    /**
     * Addictiveness potential
     */
    unsigned int addict;
    /**
     * Defaults # of charges (drugs, loaf of bread? etc)
     */
    long charges;
    /**
     * TODO: document me
     */
    std::vector<long> rand_charges;
    /**
     * TODO: document me
     */
    signed int stim;
    /**
     * TODO: document me
     */
    signed int healthy;
    /**
     * How long it takes for a brew to ferment.
     */
    unsigned int brewtime;
    /**
     * FOOD, DRINK, MED
     */
    std::string comesttype;
    /**
     * How fun its use is
     */
    signed int fun;
    /**
     * The container it comes in.
     */
    itype_id container;
    /**
     * Tool needed to consume (e.g. lighter for cigarettes)
     */
    itype_id tool;
    /**
     * Effects of addiction
     */
    add_type add;
    /**
     * Iuse function that is invoked when the item has been consumed.
     */
    use_function use_method;
    /**
     * TODO: document me
     */
    int stack_size;

    int invoke( player *p, item *it, bool active ) const {
        if( !use_method.is_none() ) {
            return use_method.call( p, it, active );
        }
        return 0;
    }
    bool has_use() const
    {
        return !use_method.is_none();
    }
    bool can_use( const std::string &iuse_name ) const;

    islot_comest()
    : quench( 0 )
    , nutr( 0 )
    , charges( 0 )
    , rand_charges()
    , stim( 0 )
    , healthy( 0 )
    , brewtime( 0 )
    , comesttype()
    , fun( 0 )
    , container()
    , tool()
    , add()
    , use_method()
    , stack_size( 0 )
    {
    }
};

struct islot_light_emission {
    /**
     * Exactly the same as item_tags LIGHT_*, this is for lightmap.
     */
    unsigned int strength;
    bool chargedim;

    islot_light_emission()
    : strength( 0 )
    , chargedim( false )
    {
    }
};

/**
 * Slots whose content depends on the active/inactive state.
 */
struct depending_slots {
    std::unique_ptr<islot_light_emission> light_emission_slot;
};

struct itype {
    itype_id id; // unique string identifier for this item,
    // can be used as lookup key in master itype map
    // Used for save files; aligns to itype_id above.
    unsigned int  price; // Its value

    std::unique_ptr<islot_container> container_slot;
    std::unique_ptr<islot_variable_bigness> variable_bigness_slot;
    std::unique_ptr<islot_bionic> bionic_slot;
    std::unique_ptr<islot_stationary> stationary_slot;
    std::unique_ptr<islot_armor> armor_slot;
    std::unique_ptr<islot_tool> tool_slot;
    std::unique_ptr<islot_book> book_slot;
    std::unique_ptr<islot_gun> gun_slot;
    std::unique_ptr<islot_comest> comest_slot;
    std::unique_ptr<islot_gunmod> gunmod_slot;
    // Explosion that happens when the item is set on fire
    std::unique_ptr<explosion_data> explode_in_fire_slot;
    // Depending slots: first entry is for inactive items,
    // second entry is for active items.
    depending_slots deps[2];

protected:
    friend class Item_factory;
    // private because is should only be accessed through itype::nname!
    // name and name_plural are not translated automatically
    // nname() is used for display purposes
    std::string name;        // Proper name, singular form, in American English.
    std::string name_plural; // name, plural form, in American English.
public:
    std::string description; // Flavor text

    char sym;       // Symbol on the map
    nc_color color; // Color on the map (color.h)

    std::string m1; // Main material
    std::string m2; // Secondary material -- "null" if made of just 1 thing

    phase_id phase; //e.g. solid, liquid, gas

    unsigned int volume; // Space taken up by this item
    unsigned int weight; // Weight in grams. Assumes positive weight. No helium, guys!
    std::map<std::string, int> qualities; //Tool quality indicators

    mtype   *corpse;

    signed int melee_dam; // Bonus for melee damage; may be a penalty
    signed int melee_cut; // Cutting damage in melee
    signed int m_to_hit;  // To-hit bonus for melee combat; -5 to 5 is reasonable

    std::set<std::string> item_tags;
    std::set<std::string> techniques;

    const item_category *category; // category pointer or NULL for automatic selection

    virtual std::string get_item_type_string() const
    {
        if( variable_bigness_slot.get() != nullptr ) {
            return "VEHICLE_PART";
        } else if( bionic_slot.get() != nullptr ) {
            return "BIONIC";
        } else if( armor_slot.get() != nullptr ) {
            return "ARMOR";
        } else if( tool_slot.get() != nullptr ) {
            return "TOOL";
        } else if( book_slot.get() != nullptr ) {
            return "BOOK";
        } else if( gun_slot.get() != nullptr ) {
            return "GUN";
        } else if( comest_slot.get() != nullptr ) {
            return "FOOD";
        }
        return "misc";
    }

    std::string bash_dmg_verb() const
    {
        return m2 == "null" || !one_in(3) ?
               material_type::find_material(m1)->bash_dmg_verb() :
               material_type::find_material(m2)->bash_dmg_verb();
    }
    std::string cut_dmg_verb() const
    {
        return m2 == "null" || !one_in(3) ?
               material_type::find_material(m1)->cut_dmg_verb() :
               material_type::find_material(m2)->cut_dmg_verb();
    }

    // Returns the name of the item type in the correct language and with respect to its grammatical number,
    // based on quantity (example: item type “anvil”, nname(4) would return “anvils” (as in “4 anvils”).
    virtual std::string nname(unsigned int quantity) const
    {
        return ngettext(name.c_str(), name_plural.c_str(), quantity);
    }

    virtual bool is_ammo() const
    {
        return false;
    }
    virtual bool is_gunmod() const
    {
        return false;
    }
    virtual bool is_software() const
    {
        return false;
    }
    virtual bool is_macguffin() const
    {
        return false;
    }
    virtual bool is_artifact() const
    {
        return false;
    }
    virtual bool count_by_charges() const
    {
        if( comest_slot ) {
            if (phase == LIQUID) {
                return true;
            } else {
                return comest_slot->charges > 1 ;
            }
        }
        return false;
    }
    int charges_to_use() const
    {
        if( tool_slot ) {
            return tool_slot->charges_per_use;
        }
        return 1;
    }
    int maximum_charges() const
    {
        if( tool_slot ) {
            return tool_slot->max_charges;
        }
        return 1;
    }

    bool has_use() const;
    bool can_use( std::string iuse_name ) const;
    /** Returns true if this covers bp */
    bool is_covering(body_part bp) const
    {
        return armor_slot && armor_slot->covers.test( bp );
    }
    /** Returns true if this is sided on bp */
    bool is_sided(body_part bp) const
    {
        return armor_slot && armor_slot->sided.test( bp );
    }
    int invoke( player *p, item *it, bool active );

    std::string dmg_adj(int dam)
    {
        return material_type::find_material(m1)->dmg_adj(dam);
    }

    std::vector<use_function> use_methods;// Special effects of use

    itype() : id("null"), price(0), name("none"), name_plural("none"), description(), sym('#'),
        color(c_white), m1("null"), m2("null"), phase(SOLID), volume(0),
        weight(0), qualities(), corpse(NULL),
        melee_dam(0), melee_cut(0), m_to_hit(0), item_tags(), techniques(),
        category(NULL) { }

    itype(std::string pid, unsigned int pprice, std::string pname, std::string pname_plural,
          std::string pdes, char psym, nc_color pcolor, std::string pm1, std::string pm2,
          phase_id pphase, unsigned int pvolume, unsigned int pweight, signed int pmelee_dam,
          signed int pmelee_cut, signed int pm_to_hit) : id(pid), price(pprice), name(pname),
        name_plural(pname_plural), description(pdes), sym(psym), color(pcolor), m1(pm1), m2(pm2),
        phase(pphase), volume(pvolume), weight(pweight),
        qualities(), corpse(NULL), melee_dam(pmelee_dam),
        melee_cut(pmelee_cut), m_to_hit(pm_to_hit), item_tags(), techniques(),
        category(NULL) { }

    virtual ~itype() {}
};

struct it_ammo : public virtual itype {
    ammotype type;          // Enum of varieties (e.g. 9mm, shot, etc)
    itype_id casing;        // Casing produced by the ammo, if any
    unsigned int damage;   // Average damage done
    unsigned int pierce;   // Armor piercing; static reduction in armor
    unsigned int range;    // Maximum range
    signed int dispersion; // Dispersion (low is good)
    unsigned int recoil;   // Recoil; modified by strength
    unsigned int count;    // Default charges
    int stack_size;

    itype_id container; // The container it comes in

    std::set<std::string> ammo_effects;

    it_ammo(): itype(), type(), casing(), damage(0), pierce(0), range(0), dispersion(0), recoil(0),
        count(0), container(), ammo_effects()
    {
        stack_size = 0;
    }

    virtual bool is_ammo() const
    {
        return true;
    }
    // virtual bool count_by_charges() { return id != "gasoline"; }
    virtual bool count_by_charges() const
    {
        return true;
    }
    virtual std::string get_item_type_string() const
    {
        return "AMMO";
    }
};


struct it_macguffin : public virtual itype {
    bool readable; // If true, activated with 'R'

    virtual bool is_macguffin() const
    {
        return true;
    }
    it_macguffin(std::string pid, unsigned int pprice, std::string pname, std::string pname_plural,
                 std::string pdes, char psym, nc_color pcolor, std::string pm1, std::string pm2,
                 unsigned int pvolume, unsigned int pweight, signed int pmelee_dam,
                 signed int pmelee_cut, signed int pm_to_hit, bool preadable,
                 int (iuse::*puse)(player *, item *, bool))
        : itype(pid, pprice, pname, pname_plural, pdes, psym, pcolor, pm1, pm2, SOLID, pvolume,
                pweight, pmelee_dam, pmelee_cut, pm_to_hit)
    {
        readable = preadable;
        use_methods.push_back( puse );
    }
};

struct it_software : public virtual itype {
    software_type swtype;
    int power;

    virtual bool is_software() const
    {
        return true;
    }

    it_software(std::string pid, unsigned int pprice, std::string pname, std::string pname_plural,
                std::string pdes, char psym, nc_color pcolor, std::string pm1, std::string pm2,
                unsigned int pvolume, unsigned int pweight, signed int pmelee_dam,
                signed int pmelee_cut, signed int pm_to_hit, software_type pswtype, int ppower)
        : itype(pid, pprice, pname, pname_plural, pdes, psym, pcolor, pm1, pm2, SOLID,
                pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit)
    {
        swtype = pswtype;
        power = ppower;
    }
};

#endif
