#include <iostream>
#include <fstream>
#include <sstream>

#include "scenario.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "start_location.h"
#include "game.h"
#include "map.h"
#include "translations.h"
#include "pldata.h"
#include "addiction.h"
#include "skill.h"
#include "profession.h"
#include "mutation.h"
#include "mapgen.h"
#include "generic_factory.h"

namespace {
generic_factory<scenario> scenario_factory( "scenario", "ident" );
}

template<>
const scenario &string_id<scenario>::obj() const
{
    return scenario_factory.obj( *this );
}

template<>
bool string_id<scenario>::is_valid() const
{
    return scenario_factory.is_valid( *this );
}

const string_id<scenario> generic_scenario_id( "evacuee" );

scenario::scenario()
   : id(""), _name_male("null"), _name_female("null"),
     _description_male("null"), _description_female("null")
{
}

void scenario::load_scenario(JsonObject &jsobj)
{
    scenario_factory.load( jsobj );
}

void scenario::load( JsonObject &jo )
{
    // TODO: loading the names and items is identical with the code in profession.cpp,
    // maybe combine them somehow. Mabye make scenario a subtype of profession? Or the other way round?

    //If the "name" is an object then we have to deal with gender-specific titles,
    if( jo.has_object( "name" ) ) {
        JsonObject name_obj = jo.get_object( "name" );
        _name_male = pgettext( "profession_male", name_obj.get_string( "male" ).c_str() );
        _name_female = pgettext( "profession_female", name_obj.get_string( "female" ).c_str() );
    } else if( jo.has_string( "name" ) ) {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jo.get_string( "name" );
        _name_female = pgettext( "profession_female", name.c_str() );
        _name_male = pgettext( "profession_male", name.c_str() );
    } else if( !was_loaded ) {
        jo.throw_error( "missing mandatory member \"name\"" );
    }

    if( !was_loaded || jo.has_object( "description" ) ) {
        const std::string desc = jo.get_string( "description" );
        _description_male = pgettext( "prof_desc_male", desc.c_str() );
        _description_female = pgettext( "prof_desc_female", desc.c_str() );
    }

    mandatory( jo, was_loaded, "start_name", _start_name, translated_string_reader );
    mandatory( jo, was_loaded, "points", _point_cost );

    if( !was_loaded || jo.has_member( "items" ) ) {
        JsonObject items_obj = jo.get_object( "items" );
        optional( items_obj, was_loaded, "both", _starting_items, auto_flags_reader<> {} );
        optional( items_obj, was_loaded, "male", _starting_items_male, auto_flags_reader<> {} );
        optional( items_obj, was_loaded, "female", _starting_items_female, auto_flags_reader<> {} );
    }

    optional( jo, was_loaded, "professions", _allowed_professions,
              auto_flags_reader<string_id<profession>> {} );
    optional( jo, was_loaded, "traits", _allowed_traits, auto_flags_reader<> {} );
    optional( jo, was_loaded, "forced_traits", _forced_traits, auto_flags_reader<> {} );
    optional( jo, was_loaded, "forbidden_traits", _forbidden_traits, auto_flags_reader<> {} );
    mandatory( jo, was_loaded, "allowed_locs", _allowed_locs, auto_flags_reader<start_location_id> {} );
    if( _allowed_locs.empty() ) {
        jo.throw_error( "at least one starting location (member \"allowed_locs\") must be defined" );
    }

    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "map_special", _map_special, "mx_null" );
}

const scenario *scenario::generic()
{
    return &generic_scenario_id.obj();
}

// Strategy: a third of the time, return the generic scenario.  Otherwise, return a scenario,
// weighting 0 cost scenario more likely--the weight of a scenario with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const scenario *scenario::weighted_random()
{
    if (one_in(3)) {
        return generic();
    }
    while( true ) {
        const auto &pair = random_entry( scenario_factory.all_ref() );
        const scenario &scen = pair.second;
        if( x_in_y( 2, abs( scen.point_cost() ) + 2 ) ) {
            return &scen;
        }
    }
}

std::vector<const scenario*> scenario::get_all()
{
    return scenario_factory.get_all();
}

void scenario::reset()
{
    scenario_factory.reset();
}

void scenario::check_definitions()
{
    for( const auto &p : scenario_factory.all_ref() ) {
        p.second.check_definition();
    }
}

void check_traits( const std::set<std::string> &traits, const string_id<scenario> &ident )
{
    for( auto &t : traits ) {
        if( !mutation_branch::has( t ) ) {
            debugmsg( "trait %s for scenario %s does not exist", t.c_str(), ident.c_str() );
        }
    }
}

void check_items( const std::vector<std::string> &items, const string_id<scenario> &ident )
{
    for( auto &i : items ) {
        if( !item::type_is_defined( i ) ) {
            debugmsg( "item %s for scenario %s does not exist", i.c_str(), ident.c_str() );
    }
        }
    }

void scenario::check_definition() const
{
    check_items( _starting_items, id );
    check_items( _starting_items_female, id );
    check_items( _starting_items_male, id );
    for( auto &p : _allowed_professions ) {
        if( !p.is_valid() ) {
            debugmsg( "profession %s for scenario %s does not exist", p.c_str(), id.c_str() );
        }
    }
    for( auto &l : _allowed_locs ) {
        if( !l.is_valid() ) {
            debugmsg( "starting location %s for scenario %s does not exist", l.c_str(), id.c_str() );
        }
    }
    check_traits( _allowed_traits, id );
    check_traits( _forced_traits, id );
    check_traits( _forbidden_traits, id );
    MapExtras::get_function( _map_special ); // triggers a debug message upon invalid input
}

const string_id<scenario> &scenario::ident() const
{
    return id;
}

std::string scenario::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    }
    else {
        return _name_female;
    }
}

std::string scenario::description(bool male) const
{
    if(male) {
        return _description_male;
    }
    else {
        return _description_female;
    }
}

signed int scenario::point_cost() const
{
    return _point_cost;
}

start_location_id scenario::start_location() const
{
    return _allowed_locs.front();
}
start_location_id scenario::random_start_location() const
{
   return random_entry( _allowed_locs );
}
const profession* scenario::get_profession() const
{
    if( _allowed_professions.empty() ) {
        return profession::generic();
    } else {
        return &_allowed_professions.front().obj();
    }
}
const profession* scenario::random_profession() const
{
    if( _allowed_professions.empty() ) {
        return profession::generic();
    } else {
        return &random_entry( _allowed_professions ).obj();
    }
}
std::string scenario::start_name() const
{
    return _start_name;
}
int scenario::mission() const
{
    return _mission;
}
std::vector<std::string> scenario::items() const
{
    return _starting_items;
}

std::vector<std::string> scenario::items_male() const
{
    return _starting_items_male;
}

std::vector<std::string> scenario::items_female() const
{
    return _starting_items_female;
}
bool scenario::profquery( const string_id<profession> &proff ) const
{
    auto &vec = _allowed_professions;
    return std::find( vec.begin(), vec.end(), proff ) != vec.end();
}
bool scenario::traitquery(std::string trait) const
{
    return _allowed_traits.count(trait) != 0;
}
bool scenario::locked_traits(std::string trait) const
{
    return _forced_traits.count(trait) != 0;
}
bool scenario::forbidden_traits(std::string trait) const
{
    return _forbidden_traits.count(trait) != 0;
}
int scenario::profsize() const
{
    return _allowed_professions.size();
}
bool scenario::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}
bool scenario::allowed_start( const start_location_id &loc ) const
{
    auto &vec = _allowed_locs;
    return std::find( vec.begin(), vec.end(), loc ) != vec.end();
}
bool scenario::can_pick(int points) const
{
    if (point_cost() - g->scen->point_cost() > points) {
        return false;
    }

    return true;
}
bool scenario::has_map_special() const
{
    return _map_special != "mx_null";
}
const std::string& scenario::get_map_special() const
{
    return _map_special;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
