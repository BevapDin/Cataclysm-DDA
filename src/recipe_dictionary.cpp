#include "recipe_dictionary.h"
#include "crafting.h"

#include <algorithm> //std::remove

using itype_id = std::string; // From itype.h

recipe_dictionary recipe_dict;

void recipe_dictionary::add( recipe rec_ )
{
    recipes.push_back( std::move( rec_ )  );
    recipe * const rec = &recipes.back();
    add_to_component_lookup( rec );
    by_name[rec->ident()] = rec;
    by_category[rec->cat].push_back( rec );
}

void recipe_dictionary::delete_if( const std::function<bool( recipe & )> &pred )
{
    for( auto iter = recipes.begin(); iter != recipes.end(); ) {
        const auto old_iter = iter;
        recipe &r = *iter;
        // Already moving to the next, so we can erase the recipe without invalidating `iter`.
        ++iter;
        if( pred( r ) ) {
            remove_from_component_lookup( &r );
            by_name.erase( r.ident() );
            // Terse name for category vector since it's repeated so many times.
            auto &cat_vec = by_category[r.cat];
            cat_vec.erase( std::remove( cat_vec.begin(), cat_vec.end(), &r ), cat_vec.end() );
            recipes.erase( old_iter );
        }
    }
}

void recipe_dictionary::add_to_component_lookup( const recipe *r )
{
    std::unordered_set<itype_id> counted;
    for( const auto &comp_choices : r->requirements.get_components() ) {
        for( const item_comp &comp : comp_choices ) {
            if( counted.count( comp.type ) ) {
                continue;
            }
            counted.insert( comp.type );
            by_component[comp.type].push_back( r );
        }
    }
}

void recipe_dictionary::remove_from_component_lookup( const recipe *r )
{
    for( auto &map_item : by_component ) {
        std::vector<const recipe *> &rlist = map_item.second;
        rlist.erase( std::remove( rlist.begin(), rlist.end(), r ), rlist.end() );
    }
}

void recipe_dictionary::reset()
{
    by_component.clear();
    by_name.clear();
    by_category.clear();
    recipes.clear();
}

const std::vector<const recipe *> &recipe_dictionary::in_category( const std::string &cat )
{
    return by_category[cat];
}

const std::vector<const recipe *> &recipe_dictionary::of_component( const itype_id &id )
{
    return by_component[id];
}

void recipe_dictionary::load( JsonObject &jsobj )
{
    recipe rec;
    rec.load( jsobj );

    if( jsobj.get_bool( "override", false ) ) {
        // Remove the existing recipe of the same ident (if any).
        delete_if( [&]( const recipe &existing_rec ) {
            return existing_rec.ident() == rec.ident();
        } );

    } else if( operator[]( rec.ident() ) != nullptr ) {
        // Not overriding: throw if there is an existing recipe.
        jsobj.throw_error(
            std::string( "Recipe name collision (set a unique value for the id_suffix field to fix): " ) +
            rec.result, "result" );
    }

    add( std::move( rec ) );
}

void recipe_dictionary::finalize()
{
    for( auto &r : recipes ) {
        r.finalize();
    }
}

void recipe_dictionary::check_consistency() const
{
    for( auto &r : recipes ) {
        r.check_consistency();
    }
}

const recipe *recipe_dictionary::get_disassemble_recipe( const itype_id &type ) const
{
    for( auto &cur_recipe : recipes ) {
        if( type == cur_recipe.result && cur_recipe.reversible ) {
            return &cur_recipe;
        }
    }
    // no matching disassemble recipe found.
    return nullptr;
}
