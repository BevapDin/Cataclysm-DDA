#pragma once
#ifndef IUSE_H
#define IUSE_H

#include "enums.h"

#include <map>
#include <string>
#include <vector>
#include <memory>

class item;
class player;
class JsonObject;
class MonsterGenerator;

template<typename T> class ret_val;

struct iteminfo;
typedef std::string itype_id;
struct tripoint;

class iuse_invocation_data
{
    private:
        using item_class = ::item;
        player *p;
        item_class *it;
        bool t;
        //@todo change to item_location
        tripoint pos;

    public:
        iuse_invocation_data( player *const p, item_class &it, const bool t, const tripoint &pos ) : p( p ),
            it( &it ), t( t ), pos( pos ) {}

        ///@returns `*p`
        ///@throws if @ref p is null.
        player &user() const;
        ///@returns @ref p - may return a null pointer.
        player *user_ptr() const {
            return p;
        }
        bool has_user() const {
            return p != nullptr;
        }
        item_class &item() const {
            return *it;
        }
        bool is_active() const {
            return t;
        }
        //@todo change to item_location
        const tripoint &position() const {
            return pos;
        }
        ///@returns `it->type->charges_to_use()` - intended to be used as result
        ///of the iuse function.
        int charges_to_use() const;
};

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
public:
// FOOD AND DRUGS (ADMINISTRATION)
    int sewage              ( const iuse_invocation_data &data);
    int honeycomb           ( const iuse_invocation_data &data);
    int royal_jelly         ( const iuse_invocation_data &data);
    int completefirstaid    ( const iuse_invocation_data &data);
    int disinfectant        ( const iuse_invocation_data &data);
    int caff                ( const iuse_invocation_data &data);
    int atomic_caff         ( const iuse_invocation_data &data);
    int alcohol_weak        ( const iuse_invocation_data &data);
    int alcohol_medium      ( const iuse_invocation_data &data);
    int alcohol_strong      ( const iuse_invocation_data &data);
    int xanax               ( const iuse_invocation_data &data);
    int smoking             ( const iuse_invocation_data &data);
    int smoking_pipe        ( const iuse_invocation_data &data);
    int ecig                ( const iuse_invocation_data &data);
    int antibiotic          ( const iuse_invocation_data &data);
    int eyedrops            ( const iuse_invocation_data &data);
    int fungicide           ( const iuse_invocation_data &data);
    int antifungal          ( const iuse_invocation_data &data);
    int antiparasitic       ( const iuse_invocation_data &data);
    int anticonvulsant      ( const iuse_invocation_data &data);
    int weed_brownie        ( const iuse_invocation_data &data);
    int coke                ( const iuse_invocation_data &data);
    int meth                ( const iuse_invocation_data &data);
    int vaccine             ( const iuse_invocation_data &data);
    int flu_vaccine         ( const iuse_invocation_data &data);
    int poison              ( const iuse_invocation_data &data);
    int fun_hallu           ( const iuse_invocation_data &data);
    int meditate            ( const iuse_invocation_data &data);
    int thorazine           ( const iuse_invocation_data &data);
    int prozac              ( const iuse_invocation_data &data);
    int sleep               ( const iuse_invocation_data &data);
    int datura              ( const iuse_invocation_data &data);
    int flumed              ( const iuse_invocation_data &data);
    int flusleep            ( const iuse_invocation_data &data);
    int inhaler             ( const iuse_invocation_data &data);
    int blech               ( const iuse_invocation_data &data);
    int plantblech          ( const iuse_invocation_data &data);
    int chew                ( const iuse_invocation_data &data);
    int mutagen             ( const iuse_invocation_data &data);
    int mut_iv              ( const iuse_invocation_data &data);
    int purifier            ( const iuse_invocation_data &data);
    int purify_iv           ( const iuse_invocation_data &data);
    int marloss             ( const iuse_invocation_data &data);
    int marloss_seed        ( const iuse_invocation_data &data);
    int marloss_gel         ( const iuse_invocation_data &data);
    int mycus               ( const iuse_invocation_data &data);
    int dogfood             ( const iuse_invocation_data &data);
    int catfood             ( const iuse_invocation_data &data);
    int feedcattle          ( const iuse_invocation_data &data);
// TOOLS
    int sew_advanced        ( const iuse_invocation_data &data);
    int scissors            ( const iuse_invocation_data &data);
    int extinguisher        ( const iuse_invocation_data &data);
    int hammer              ( const iuse_invocation_data &data);
    int water_purifier      ( const iuse_invocation_data &data);
    int directional_antenna ( const iuse_invocation_data &data);
    int radio_off           ( const iuse_invocation_data &data);
    int radio_on            ( const iuse_invocation_data &data);
    int noise_emitter_off   ( const iuse_invocation_data &data);
    int noise_emitter_on    ( const iuse_invocation_data &data);
    int ma_manual           ( const iuse_invocation_data &data);
    int crowbar             ( const iuse_invocation_data &data);
    int makemound           ( const iuse_invocation_data &data);
    int dig                 ( const iuse_invocation_data &data);
    int siphon              ( const iuse_invocation_data &data);
    int chainsaw_off        ( const iuse_invocation_data &data);
    int chainsaw_on         ( const iuse_invocation_data &data);
    int elec_chainsaw_off   ( const iuse_invocation_data &data);
    int elec_chainsaw_on    ( const iuse_invocation_data &data);
    int cs_lajatang_off     ( const iuse_invocation_data &data);
    int cs_lajatang_on      ( const iuse_invocation_data &data);
    int carver_off          ( const iuse_invocation_data &data);
    int carver_on           ( const iuse_invocation_data &data);
    int trimmer_off         ( const iuse_invocation_data &data);
    int trimmer_on          ( const iuse_invocation_data &data);
    int circsaw_on          ( const iuse_invocation_data &data);
    int combatsaw_off       ( const iuse_invocation_data &data);
    int combatsaw_on        ( const iuse_invocation_data &data);
    int jackhammer          ( const iuse_invocation_data &data);
    int pickaxe             ( const iuse_invocation_data &data);
    int geiger              ( const iuse_invocation_data &data);
    int teleport            ( const iuse_invocation_data &data);
    int can_goo             ( const iuse_invocation_data &data);
    int throwable_extinguisher_act( const iuse_invocation_data &data);
    int capture_monster_act ( const iuse_invocation_data &data);
    int pipebomb_act        ( const iuse_invocation_data &data);
    int granade             ( const iuse_invocation_data &data);
    int granade_act         ( const iuse_invocation_data &data);
    int c4                  ( const iuse_invocation_data &data);
    int arrow_flamable      ( const iuse_invocation_data &data);
    int acidbomb_act        ( const iuse_invocation_data &data);
    int grenade_inc_act     ( const iuse_invocation_data &data);
    int molotov_lit         ( const iuse_invocation_data &data);
    int firecracker_pack    ( const iuse_invocation_data &data);
    int firecracker_pack_act( const iuse_invocation_data &data);
    int firecracker         ( const iuse_invocation_data &data);
    int firecracker_act     ( const iuse_invocation_data &data);
    int mininuke            ( const iuse_invocation_data &data);
    int pheromone           ( const iuse_invocation_data &data);
    int portal              ( const iuse_invocation_data &data);
    int UPS_off             ( const iuse_invocation_data &data);
    int UPS_on              ( const iuse_invocation_data &data);
    int adv_UPS_off         ( const iuse_invocation_data &data);
    int adv_UPS_on          ( const iuse_invocation_data &data);
    int tazer               ( const iuse_invocation_data &data);
    int tazer2              ( const iuse_invocation_data &data);
    int shocktonfa_off      ( const iuse_invocation_data &data);
    int shocktonfa_on       ( const iuse_invocation_data &data);
    int mp3                 ( const iuse_invocation_data &data);
    int mp3_on              ( const iuse_invocation_data &data);
    int portable_game       ( const iuse_invocation_data &data);
    int vibe                ( const iuse_invocation_data &data);
    int vortex              ( const iuse_invocation_data &data);
    int dog_whistle         ( const iuse_invocation_data &data);
    int blood_draw          ( const iuse_invocation_data &data);
    static void cut_log_into_planks(player *);
    int lumber              ( const iuse_invocation_data &data);
    int chop_tree           ( const iuse_invocation_data &data);
    int chop_logs           ( const iuse_invocation_data &data);
    int oxytorch            ( const iuse_invocation_data &data);
    int hacksaw             ( const iuse_invocation_data &data);
    int portable_structure  ( const iuse_invocation_data &data);
    int tent                ( const iuse_invocation_data &data);
    int large_tent          ( const iuse_invocation_data &data);
    int shelter             ( const iuse_invocation_data &data);
    int torch_lit           ( const iuse_invocation_data &data);
    int battletorch_lit     ( const iuse_invocation_data &data);
    int boltcutters         ( const iuse_invocation_data &data);
    int mop                 ( const iuse_invocation_data &data);
    int spray_can           ( const iuse_invocation_data &data);
    int heatpack            ( const iuse_invocation_data &data);
    int heat_food           ( const iuse_invocation_data &data);
    int hotplate            ( const iuse_invocation_data &data);
    int towel               ( const iuse_invocation_data &data);
    int unfold_generic      ( const iuse_invocation_data &data);
    int adrenaline_injector ( const iuse_invocation_data &data);
    int jet_injector        ( const iuse_invocation_data &data);
    int stimpack            ( const iuse_invocation_data &data);
    int contacts            ( const iuse_invocation_data &data);
    int talking_doll        ( const iuse_invocation_data &data);
    int bell                ( const iuse_invocation_data &data);
    int seed                ( const iuse_invocation_data &data);
    int oxygen_bottle       ( const iuse_invocation_data &data);
    int radio_mod           ( const iuse_invocation_data &data);
    int remove_all_mods     ( const iuse_invocation_data &data);
    int fishing_rod         ( const iuse_invocation_data &data);
    int fish_trap           ( const iuse_invocation_data &data);
    int gun_repair          ( const iuse_invocation_data &data);
    int gunmod_attach       ( const iuse_invocation_data &data);
    int toolmod_attach      ( const iuse_invocation_data &data);
    int misc_repair         ( const iuse_invocation_data &data);
    int rm13armor_off       ( const iuse_invocation_data &data);
    int rm13armor_on        ( const iuse_invocation_data &data);
    int unpack_item         ( const iuse_invocation_data &data);
    int pack_item           ( const iuse_invocation_data &data);
    int radglove            ( const iuse_invocation_data &data);
    int robotcontrol        ( const iuse_invocation_data &data);
    int einktabletpc        ( const iuse_invocation_data &data);
    int camera              ( const iuse_invocation_data &data);
    int ehandcuffs          ( const iuse_invocation_data &data);
    int cable_attach        ( const iuse_invocation_data &data);
    int shavekit            ( const iuse_invocation_data &data);
    int hairkit             ( const iuse_invocation_data &data);
    int weather_tool        ( const iuse_invocation_data &data);
    int ladder              ( const iuse_invocation_data &data);
    int washclothes         ( const iuse_invocation_data &data);

// MACGUFFINS

    int radiocar( const iuse_invocation_data &data);
    int radiocaron( const iuse_invocation_data &data);
    int radiocontrol( const iuse_invocation_data &data);

    int multicooker( const iuse_invocation_data &data);

    int remoteveh( const iuse_invocation_data &data);

// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            ( const iuse_invocation_data &data);

    // Helper for listening to music, might deserve a better home, but not sure where.
    static void play_music( player *p, const tripoint &source, int volume, int max_morale );

    // Helper for handling pesky wannabe-artists
    static int handle_ground_graffiti( player *p, item *it, const std::string prefix );

};


typedef int ( iuse::*use_function_pointer )( const iuse_invocation_data & );

class iuse_actor {

protected:
    iuse_actor( const std::string& type, long cost = -1 ) : type( type ), cost( cost ) {}

public:
    /**
     * The type of the action. It's not translated. Different iuse_actor instances may have the
     * same type, but different data.
     */
    const std::string type;

    /** Units of ammo required per invocation (or use value from base item if negative) */
    long cost;

    virtual ~iuse_actor() = default;
    virtual void load( JsonObject &jo ) = 0;
    virtual long use( const iuse_invocation_data &data ) const = 0;
    virtual ret_val<bool> can_use( const iuse_invocation_data &data ) const;
    virtual void info( const item &, std::vector<iteminfo> & ) const {};
    /**
     * Returns a deep copy of this object. Example implementation:
     * \code
     * class my_iuse_actor {
     *     iuse_actor *clone() const override {
     *         return new my_iuse_actor( *this );
     *     }
     * };
     * \endcode
     * The returned value should behave like the original item and must have the same type.
     */
    virtual iuse_actor *clone() const = 0;
    /**
     * Returns whether the actor is valid (exists in the generator).
     */
    virtual bool is_valid() const;
    /**
     * Returns the translated name of the action. It is used for the item action menu.
     */
    virtual std::string get_name() const;
    /**
     * Finalizes the actor. Must be called after all items are loaded.
     */
    virtual void finalize( const itype_id &/*my_item_type*/ ) { }
};

struct use_function {
protected:
    std::unique_ptr<iuse_actor> actor;

public:
    use_function() = default;
    use_function( const std::string &type, use_function_pointer f );
    use_function( iuse_actor *f ) : actor( f ) {}
    use_function( use_function && ) = default;
    use_function( const use_function &other );

    ~use_function() = default;

    long call( const iuse_invocation_data &data ) const;
    ret_val<bool> can_call( const iuse_invocation_data &data ) const;

    iuse_actor *get_actor_ptr() const
    {
        return actor.get();
    }

    explicit operator bool() const {
        return actor.get() != nullptr;
    }

    /** @return See @ref iuse_actor::type */
    std::string get_type() const;
    /** @return See @ref iuse_actor::get_name */
    std::string get_name() const;
    /** @return Used by @ref item::info to get description of the actor */
    void dump_info( const item &, std::vector<iteminfo> & ) const;

    use_function &operator=( iuse_actor *f );
    use_function &operator=( use_function && ) = default;
    use_function &operator=( const use_function &other );
};

#endif
