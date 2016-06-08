#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include "string_id.h"

#include <string>
#include <vector>
#include <map>
#include <list>
#include <functional>

struct recipe;
using itype_id = std::string; // From itype.h
using recipe_id = string_id<recipe>;

class JsonObject;

/**
*   Repository class for recipes.
*
*   This class is aimed at making (fast) recipe lookups easier from the outside.
*/
class recipe_dictionary
{
    public:
        void add( recipe rec );

        /** Returns a list of recipes in the 'cat' category */
        const std::vector<const recipe *> &in_category( const std::string &cat );
        /** Returns a list of recipes in which the component with itype_id 'id' can be used */
        const std::vector<const recipe *> &of_component( const itype_id &id );

        /**
         * Lookup and return the recipe of the given ident.
         * Returns nullptr if there is no recipe with the given ident.s
         */
        const recipe *operator[]( const recipe_id &rec_name ) const {
            const auto iter = by_name.find( rec_name );
            return iter == by_name.end() ? nullptr : iter->second;
        }
        size_t size() const {
            return recipes.size();
        }

        /** Allows for iteration over all recipes like: 'for( recipe &r : recipe_dict )'. */
        std::list<recipe>::const_iterator begin() const {
            return recipes.begin();
        }
        std::list<recipe>::const_iterator end() const {
            return recipes.end();
        }

        /**
         * Goes over all recipes and calls the predicate, if it returns true, the recipe
         * is removed *and* deleted.
         */
        void delete_if( const std::function<bool( recipe & )> &pred );

        void load( JsonObject &jsobj );
        void finalize();
        void check_consistency() const;
        void reset();

    private:
        // A list because adding and removing entries does not change the memory location
        // of the other entries. Pointers to entries stay therefor valid.
        std::list<recipe> recipes;

        std::map<const std::string, std::vector<const recipe *>> by_category;
        std::map<const itype_id, std::vector<const recipe *>> by_component;

        std::map<recipe_id, const recipe *> by_name;

        /** Maps a component to a list of recipes. So we can look up what we can make with an item */
        void add_to_component_lookup( const recipe *r );
        void remove_from_component_lookup( const recipe *r );
};

extern recipe_dictionary recipe_dict;

#endif
