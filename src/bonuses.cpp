#include "bonuses.h"
#include "damage.h"
#include "json.h"
#include "character.h"
#include "debug.h"
#include <map>
#include <string>
#include <utility>
#include <sstream>
#include <vector>

#define dbg(x) DebugLog((DebugLevel)(x),D_MAIN) << __FILE__ << ":" << __LINE__ << ": "

bool needs_damage_type( const stat_accessor &as )
{
    return &as == &stats::damage || &as == &stats::armor ||
           &as == &stats::armor_penetration;
}

static const std::map<std::string, std::reference_wrapper<const stat_accessor>> stat_map = {{
        std::make_pair( "hit", stats::hit ),
        std::make_pair( "dodge", stats::dodge ),
        std::make_pair( "block", stats::block ),
        std::make_pair( "speed", stats::speed ),
        std::make_pair( "movecost", stats::move_cost ),
        std::make_pair( "damage", stats::damage ),
        std::make_pair( "armor", stats::armor ),
        std::make_pair( "arpen", stats::armor_penetration ),
        std::make_pair( "target_armor_multiplier", stats::target_armor_multiplier ),
        std::make_pair( "str", stats::strength ),
        std::make_pair( "dex", stats::dexterity ),
        std::make_pair( "int", stats::intelligence ),
        std::make_pair( "per", stats::perception ),
    }
};

static const stat_accessor &stat_from_string( const std::string &s )
{
    const auto &iter = stat_map.find( s );
    if( iter == stat_map.end() ) {
        throw std::runtime_error("invalid stat: " + s);
    }

    return iter->second;
}

bonus_container::bonus_container()
{
}

effect_scaling::effect_scaling( JsonArray &jarr )
: stat( stat_from_string( jarr.next_string() ) )
, scale( jarr.next_float() )
{
}

void bonus_container::load( JsonObject &jo )
{
    if( jo.has_array( "flat_bonuses" ) ) {
        JsonArray jarr = jo.get_array( "flat_bonuses" );
        load( jarr, false );
    }

    if( jo.has_array( "mult_bonuses" ) ) {
        JsonArray jarr = jo.get_array( "mult_bonuses" );
        load( jarr, true );
    }
}

void bonus_container::load( JsonArray &jarr, bool mult )
{
    while( jarr.has_more() ) {
        JsonArray qualifiers = jarr.next_array();

        damage_type dt = DT_NULL;
        const stat_accessor &as = stat_from_string( qualifiers.next_string() );

        if( needs_damage_type( as ) ) {
            const std::string damage_string = qualifiers.next_string();
            dt = dt_by_name( damage_string );
            if( dt == DT_NULL ) {
                jarr.throw_error( "Invalid damage type" );
            }
        }

        effect_scaling es( qualifiers );
        affected_type at( as, dt );
        // Are we changing mults or flats?
        auto &selected = mult ? bonuses_mult : bonuses_flat;
        selected[at].push_back( es );
    }
}

affected_type::affected_type( const stat_accessor &s )
: stat( s )
{
}

affected_type::affected_type( const stat_accessor &s, damage_type t )
: stat( s )
{
    if( needs_damage_type( s ) ) {
        type = t;
    } else {
        type = DT_NULL;
    }
}

bool affected_type::operator<( const affected_type &other ) const
{
    return &stat < &other.stat || ( &stat == &other.stat && type < other.type );
}
bool affected_type::operator==( const affected_type &other ) const
{
    // NOTE: Constructor has to ensure type is set to NULL for some stats
    return &stat == &other.stat && type == other.type;
}

float bonus_container::get_flat( const Character &u, const stat_accessor &stat, damage_type dt ) const
{
    affected_type type( stat, dt );
    const auto &iter = bonuses_flat.find( type );
    if( iter == bonuses_flat.end() ) {
        return 0.0f;
    }

    float ret = 0.0f;
    for( const auto &es : iter->second ) {
        ret += es.get( u );
    }

    return ret;
}
float bonus_container::get_flat( const Character &u, const stat_accessor &stat ) const
{
    return get_flat( u, stat, DT_NULL );
}

float bonus_container::get_mult( const Character &u, const stat_accessor &stat, damage_type dt ) const
{
    affected_type type( stat, dt );
    const auto &iter = bonuses_mult.find( type );
    if( iter == bonuses_mult.end() ) {
        return 1.0f;
    }

    float ret = 1.0f;
    for( const auto &es : iter->second ) {
        ret *= es.get( u );
    }

    // Currently all relevant effects require non-negative values
    return std::max( 0.0f, ret );
}
float bonus_container::get_mult( const Character &u, const stat_accessor &stat ) const
{
    return get_mult( u, stat, DT_NULL );
}

float effect_scaling::get( const Character &u ) const
{
    return scale * stat.get( u );
}
