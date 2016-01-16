#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "mutation.h"
#include "text_snippets.h"
#include "rng.h"
#include "translations.h"
#include "skill.h"
#include "addiction.h"
#include "pldata.h"
#include "itype.h"
#include "generic_factory.h"

namespace {
generic_factory<profession> profession_factory( "profession", "ident" );
}

template<>
const profession &string_id<profession>::obj() const
{
    return profession_factory.obj( *this );
}

template<>
bool string_id<profession>::is_valid() const
{
    return profession_factory.is_valid( *this );
}

const string_id<profession> generic_profession_id( "unemployed" );

profession::profession()
    : id(), _name_male("null"), _name_female("null"),
      _description_male("null"), _description_female("null"), _point_cost(0)
{
}

void profession::load_profession(JsonObject &jsobj)
{
    profession_factory.load( jsobj );
}

bool operator==( const addiction &a, const addiction &b )
{
    // Comparing only the type on purpose, so a mod can remove an
    // addiction by its id, ignoring the addiction level.
    return a.type == b.type;
}

bool operator==( const profession::itypedec &a, const profession::itypedec &b )
{
    // Comparing only the type on purpose, so a mod can remove an
    // item by its id, ignoring the snippet id.
    return a.type_id == b.type_id;
}

namespace {
class skilllevel_reader : public generic_typed_reader<std::pair<skill_id, int>> {
    private:
        std::pair<skill_id, int> get_next( JsonIn &jin ) const override {
            JsonObject jo = jin.get_object();
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
};

class addiction_reader : public generic_typed_reader<addiction> {
    private:
        addiction get_next( JsonIn &jin ) const override {
            JsonObject jo = jin.get_object();
            return addiction( addiction_type( jo.get_string( "type" ) ), jo.get_int( "intensity" ) );
        }
};

class item_reader : public generic_typed_reader<profession::itypedec> {
    private:
        profession::itypedec get_next( JsonIn &jin ) const override {
            // either a plain item type id string, or an array with item type id
            // and as second entry the item description.
            if( jin.test_array() ) {
                JsonArray arr = jin.get_array();
                const auto id = arr.get_string( 0 );
                const auto snippet = _( arr.get_string( 1 ).c_str() );
                return profession::itypedec( id, snippet );
            } else {
                return profession::itypedec( jin.get_string(), "" );
            }
        }
};
} // namespace

void profession::load( JsonObject &jo )
{
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

    mandatory( jo, was_loaded, "points", _point_cost );

    if( !was_loaded || jo.has_member( "items" ) ) {
        JsonObject items_obj = jo.get_object( "items" );
        optional( items_obj, was_loaded, "both", _starting_items, item_reader{} );
        optional( items_obj, was_loaded, "male", _starting_items_male, item_reader{} );
        optional( items_obj, was_loaded, "female", _starting_items_female, item_reader{} );
    }

    optional( jo, was_loaded, "skills", _starting_skills, skilllevel_reader{} );
    optional( jo, was_loaded, "addictions", _starting_addictions, addiction_reader{} );
    // TODO: use string_id<bionic_type> or so
    optional( jo, was_loaded, "CBMs", _starting_CBMs, auto_flags_reader<> {} );
    // TODO: use string_id<mutation_branch> or so
    optional( jo, was_loaded, "traits", _starting_traits, auto_flags_reader<> {} );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
}

const profession *profession::generic()
{
    return &generic_profession_id.obj();
}

// Strategy: a third of the time, return the generic profession.  Otherwise, return a profession,
// weighting 0 cost professions more likely--the weight of a profession with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const profession *profession::weighted_random()
{
    if (one_in(3)) {
        return generic();
    }
    while( true ) {
        const auto &pair = random_entry( profession_factory.all_ref() );
        const profession &prof = pair.second;
        if( x_in_y( 2, abs( prof.point_cost() ) + 2 ) && !prof.has_flag( "SCEN_ONLY" ) ) {
            return &prof;
        }  // else reroll in the while loop.
    }
}

std::vector<const profession*> profession::get_all()
{
    return profession_factory.get_all();
}

void profession::reset()
{
    profession_factory.reset();
}

void profession::check_definitions()
{
    for( const auto &p : profession_factory.all_ref() ) {
        p.second.check_definition();
    }
}

void profession::check_item_definitions( const itypedecvec &items ) const
{
    for( auto & itd : items ) {
        if( !item::type_is_defined( itd.type_id ) ) {
            debugmsg( "profession %s: item %s does not exist", id.c_str() , itd.type_id.c_str() );
        } else if( !itd.snippet_id.empty() ) {
            const itype *type = item::find_type( itd.type_id );
            if( type->snippet_category.empty() ) {
                debugmsg( "profession %s: item %s has no snippet category - no description can be set",
                          id.c_str(), itd.type_id.c_str() );
            } else {
                const int hash = SNIPPET.get_snippet_by_id( itd.snippet_id );
                if( SNIPPET.get( hash ).empty() ) {
                    debugmsg( "profession %s: snippet id %s for item %s is not contained in snippet category %s",
                              id.c_str(), itd.snippet_id.c_str(), itd.type_id.c_str(), type->snippet_category.c_str() );
                }
            }
        }
    }
}

void profession::check_definition() const
{
    check_item_definitions( _starting_items );
    check_item_definitions( _starting_items_female );
    check_item_definitions( _starting_items_male );
    for (auto const &a : _starting_CBMs) {
        if (!is_valid_bionic(a)) {
            debugmsg("bionic %s for profession %s does not exist", a.c_str(), id.c_str());
        }
    }

    for( auto &t : _starting_traits ) {
        if( !mutation_branch::has( t ) ) {
            debugmsg( "trait %s for profession %s does not exist", t.c_str(), id.c_str() );
        }
    }

    for( const auto &elem : _starting_skills ) {
        if( !elem.first.is_valid() ) {
            debugmsg( "skill %s for profession %s does not exist", elem.first.c_str(), id.c_str() );
        }
    }
}

bool profession::has_initialized()
{
    return generic_profession_id.is_valid();
}

void profession::add_CBM(std::string CBM)
{
    _starting_CBMs.push_back(CBM);
}

void profession::add_trait(std::string trait)
{
    _starting_traits.push_back(trait);
}

void profession::add_addiction(add_type type, int intensity)
{
    _starting_addictions.push_back(addiction(type, intensity));
}
void profession::add_skill(const skill_id &skill_name, const int level)
{
    _starting_skills.push_back(StartingSkill(skill_name, level));
}

const string_id<profession> &profession::ident() const
{
    return id;
}

std::string profession::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    } else {
        return _name_female;
    }
}

std::string profession::description(bool male) const
{
    if(male) {
        return _description_male;
    } else {
        return _description_female;
    }
}

signed int profession::point_cost() const
{
    return _point_cost;
}

profession::itypedecvec profession::items(bool male) const
{
    auto result = _starting_items;
    const auto &gender_items = male ? _starting_items_male : _starting_items_female;
    result.insert( result.begin(), gender_items.begin(), gender_items.end() );
    return result;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}

std::vector<std::string> profession::CBMs() const
{
    return _starting_CBMs;
}

std::vector<std::string> profession::traits() const
{
    return _starting_traits;
}

const profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}

bool profession::can_pick(player *u, int points) const
{
    if (point_cost() - u->prof->point_cost() > points) {
        return false;
    }

    return true;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
