#include <vector>
#include <memory>
#include "game.h"
#include "sounds.h"
#include "vehicle.h"

void trap::load( JsonObject &jo )
{
    std::unique_ptr<trap> trap_ptr( new trap() );
    trap &t = *trap_ptr;

    if( jo.has_member( "drops" ) ) {
        JsonArray drops_list = jo.get_array( "drops" );
        while( drops_list.has_more() ) {
            t.components.push_back( drops_list.next_string() );
        }
    }
    t.name = jo.get_string( "name" );
    if( !t.name.empty() ) {
        t.name = _( t.name.c_str() );
    }
    t.id = jo.get_string( "id" );
    t.loadid = traplist.size();
    t.color = color_from_string( jo.get_string( "color" ) );
    t.sym = jo.get_string( "symbol" ).at( 0 );
    t.visibility = jo.get_int( "visibility" );
    t.avoidance = jo.get_int( "avoidance" );
    t.difficulty = jo.get_int( "difficulty" );
    t.act = trap_function_from_string( jo.get_string( "action" ) );
    t.benign = jo.get_bool( "benign", false );
    t.funnel_radius_mm = jo.get_int( "funnel_radius", 0 );
    t.trigger_weight = jo.get_int( "trigger_weight", -1 );

    trapmap[t.id] = t.loadid;
    traplist.push_back( &t );
    trap_ptr.release();
}

void trap::reset()
{
    for( auto & tptr : traplist ) {
        delete tptr;
    }
    traplist.clear();
    trapmap.clear();
}

std::vector <trap *> traplist;
std::map<std::string, int> trapmap;

trap_id trapfind(const std::string id)
{
    if( trapmap.find(id) == trapmap.end() ) {
        popup("Can't find trap %s", id.c_str());
        return 0;
    }
    return traplist[trapmap[id]]->loadid;
}

bool trap::detect_trap( const tripoint &pos, const player &p ) const
{
    // Some decisions are based around:
    // * Starting, and thus average perception, is 8.
    // * Buried landmines, the silent killer, has a visibility of 10.
    // * There will always be a distance malus of 1 unless you're on top of the trap.
    // * ...and an average character should at least have a minor chance of
    //   noticing a buried landmine if standing right next to it.
    // Effective Perception...
    return (p.per_cur - (p.encumb(bp_eyes) / 10)) +
           // ...small bonus from stimulants...
           (p.stim > 10 ? rng(1, 2) : 0) +
           // ...bonus from trap skill...
           (p.get_skill_level("traps") * 2) +
           // ...luck, might be good, might be bad...
           rng(-4, 4) -
           // ...malus if we are tired...
           (p.has_effect("lack_sleep") ? rng(1, 5) : 0) -
           // ...malus farther we are from trap...
           rl_dist( p.pos(), point( pos.x, pos.y) ) +
           // Police are trained to notice Something Wrong.
           (p.has_trait("PROF_POLICE") ? 1 : 0) +
           (p.has_trait("PROF_PD_DET") ? 2 : 0) >
           // ...must all be greater than the trap visibility.
           visibility;
}

// Whether or not, in the current state, the player can see the trap.
bool trap::can_see( const tripoint &pos, const player &p ) const
{
    if( is_null() ) {
        // There is no trap at all, so logically one can not see it.
        return false;
    }
    return visibility < 0 || p.knows_trap( pos );
}

void trap::trigger( const tripoint &pos, Creature *creature ) const
{
    if( act != nullptr ) {
        trapfunc f;
        (f.*act)( creature, pos.x, pos.y );
    }
}

bool trap::is_null() const
{
    return loadid == tr_null;
}

bool trap::triggered_by_item( const item &itm ) const
{
    return !is_null() && itm.weight() >= trigger_weight;
}

void trap::triggered_by_wheel( vehicle &veh, int const part, tripoint const & pos,
                              std::function<void(int)> const wheel_damage_func ) const
{
    int noise = 0;
    std::string snd;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    int part_damage = 0;

    if( id == "tr_bubblewrap" ) {
        noise = 18;
        snd = _( "Pop!" );
    } else if( id == "tr_beartrap" || id == "tr_beartrap_buried" ) {
        noise = 8;
        snd = _( "SNAP!" );
        part_damage = 300;
        g->m.remove_trap( pos );
        g->m.spawn_item( pos.x, pos.y, "beartrap" );
    } else if( id == "tr_nailboard" || id == "tr_caltrops" ) {
        part_damage = 300;
    } else if( id == "tr_blade" ) {
        noise = 1;
        snd = _( "Swinnng!" );
        part_damage = 300;
    } else if( id == "tr_crossbow" ) {
        chance = 30;
        noise = 1;
        snd = _( "Clank!" );
        part_damage = 300;
        g->m.remove_trap( pos );
        g->m.spawn_item( pos.x, pos.y, "crossbow" );
        g->m.spawn_item( pos.x, pos.y, "string_6" );
        if( !one_in( 10 ) ) {
            g->m.spawn_item( pos.x, pos.y, "bolt_steel" );
        }
    } else if( id == "tr_shotgun_2" || id == "tr_shotgun_1" ) {
        noise = 60;
        snd = _( "Bang!" );
        chance = 70;
        part_damage = 300;
        if( id == "tr_shotgun_2" ) {
            g->m.trap_set( pos, "tr_shotgun_1" );
        } else {
            g->m.remove_trap( pos );
            g->m.spawn_item( pos.x, pos.y, "shotgun_sawn" );
            g->m.spawn_item( pos.x, pos.y, "string_6" );
        }
    } else if( id == "tr_landmine_buried" || id == "tr_landmine" ) {
        expl = 10;
        shrap = 8;
        g->m.remove_trap( pos );
        part_damage = 1000;
    } else if( id == "tr_boobytrap" ) {
        expl = 18;
        shrap = 12;
        part_damage = 1000;
    } else if( id == "tr_dissector" ) {
        noise = 10;
        snd = _( "BRZZZAP!" );
        part_damage = 500;
    } else if( id == "tr_sinkhole" || id == "tr_pit" || id == "tr_spike_pit" || id == "tr_ledge" || id == "tr_glass_pit" ) {
        part_damage = 500;
    } else {
        return;
    }

    if( noise > 0 ) {
        sounds::sound( pos.x, pos.y, noise, snd );
    }
    if( g->u.sees( pos.x, pos.y ) ) {
        if( g->u.knows_trap( pos ) ) {
            add_msg( m_bad, _( "The %s's %s runs over %s." ), veh.name.c_str(),
            veh.part_info( part ).name.c_str(), name.c_str() );
        } else {
            add_msg( m_bad, _( "The %s's %s runs over something." ), veh.name.c_str(),
                     veh.part_info( part ).name.c_str() );
        }
    }
    if( part_damage != 0 && chance < rng( 1, 100 ) ) {
        wheel_damage_func( part_damage );
    }
    if( expl > 0 ) {
        g->explosion( pos.x, pos.y, expl, shrap, false );
    }
}

bool trap::is_funnel() const
{
    return !is_null() && funnel_radius_mm > 0;
}

bool trap::is_3x3_trap() const
{
    // TODO make this a json flag, implement more 3x3 traps.
    return id == "tr_engine";
}

void trap::on_disarmed( const tripoint &p ) const
{
    map &m = g->m;
    for( auto & i : components ) {
        m.spawn_item( p.x, p.y, i, 1, 1 );
    }
    // TODO: make this a flag, or include it into the components.
    if( id == "tr_shotgun_1" || id == "tr_shotgun_2" ) {
        m.spawn_item( p.x, p.y, "shot_00", 1, 2 );
    }
    if( is_3x3_trap() ) {
        for( int i = -1; i <= 1; i++ ) {
            for( int j = -1; j <= 1; j++ ) {
                m.remove_trap( tripoint( p.x + i, p.y + j, p.z ) );
            }
        }
    } else {
        m.remove_trap( p );
    }
}

//////////////////////////
// convenient int-lookup names for hard-coded functions
trap_id
tr_null,
tr_bubblewrap,
tr_cot,
tr_brazier,
tr_funnel,
tr_makeshift_funnel,
tr_leather_funnel,
tr_rollmat,
tr_fur_rollmat,
tr_beartrap,
tr_beartrap_buried,
tr_nailboard,
tr_caltrops,
tr_tripwire,
tr_crossbow,
tr_shotgun_2,
tr_shotgun_1,
tr_engine,
tr_blade,
tr_light_snare,
tr_heavy_snare,
tr_landmine,
tr_landmine_buried,
tr_telepad,
tr_goo,
tr_dissector,
tr_sinkhole,
tr_pit,
tr_spike_pit,
tr_lava,
tr_portal,
tr_ledge,
tr_boobytrap,
tr_temple_flood,
tr_temple_toggle,
tr_glow,
tr_hum,
tr_shadow,
tr_drain,
tr_snake,
tr_glass_pit;

void trap::check_consistency()
{
    for( auto & tptr : traplist ) {
        for( auto & i : tptr->components ) {
            if( !item::type_is_defined( i ) ) {
                debugmsg( "trap %s has unknown item as component %s", tptr->id.c_str(), i.c_str() );
            }
        }
    }
}

void trap::finalize()
{
    tr_null = trapfind("tr_null");
    tr_bubblewrap = trapfind("tr_bubblewrap");
    tr_cot = trapfind("tr_cot");
    tr_brazier = trapfind("tr_brazier");
    tr_funnel = trapfind("tr_funnel");
    tr_makeshift_funnel = trapfind("tr_makeshift_funnel");
    tr_leather_funnel = trapfind("tr_leather_funnel");
    tr_rollmat = trapfind("tr_rollmat");
    tr_fur_rollmat = trapfind("tr_fur_rollmat");
    tr_beartrap = trapfind("tr_beartrap");
    tr_beartrap_buried = trapfind("tr_beartrap_buried");
    tr_nailboard = trapfind("tr_nailboard");
    tr_caltrops = trapfind("tr_caltrops");
    tr_tripwire = trapfind("tr_tripwire");
    tr_crossbow = trapfind("tr_crossbow");
    tr_shotgun_2 = trapfind("tr_shotgun_2");
    tr_shotgun_1 = trapfind("tr_shotgun_1");
    tr_engine = trapfind("tr_engine");
    tr_blade = trapfind("tr_blade");
    tr_light_snare = trapfind("tr_light_snare");
    tr_heavy_snare = trapfind("tr_heavy_snare");
    tr_landmine = trapfind("tr_landmine");
    tr_landmine_buried = trapfind("tr_landmine_buried");
    tr_telepad = trapfind("tr_telepad");
    tr_goo = trapfind("tr_goo");
    tr_dissector = trapfind("tr_dissector");
    tr_sinkhole = trapfind("tr_sinkhole");
    tr_pit = trapfind("tr_pit");
    tr_spike_pit = trapfind("tr_spike_pit");
    tr_lava = trapfind("tr_lava");
    tr_portal = trapfind("tr_portal");
    tr_ledge = trapfind("tr_ledge");
    tr_boobytrap = trapfind("tr_boobytrap");
    tr_temple_flood = trapfind("tr_temple_flood");
    tr_temple_toggle = trapfind("tr_temple_toggle");
    tr_glow = trapfind("tr_glow");
    tr_hum = trapfind("tr_hum");
    tr_shadow = trapfind("tr_shadow");
    tr_drain = trapfind("tr_drain");
    tr_snake = trapfind("tr_snake");
    tr_glass_pit = trapfind("tr_glass_pit");

    // Set ter_t.trap using ter_t.trap_id_str.
    for( auto &elem : terlist ) {
        if( elem.trap_id_str.empty() ) {
            elem.trap = tr_null;
        } else {
            elem.trap = trapfind( elem.trap_id_str );
        }
    }
}
