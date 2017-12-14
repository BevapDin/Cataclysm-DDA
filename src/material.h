#pragma once
#ifndef MATERIALS_H
#define MATERIALS_H

#include <string>
#include <array>
#include <map>
#include <vector>

#include "translatable_text.h"
#include "game_constants.h"
#include "string_id.h"
#include "fire.h"

enum damage_type : int;
class material_type;
using material_id = string_id<material_type>;
using itype_id = std::string;
class JsonObject;
class vitamin;
using vitamin_id = string_id<vitamin>;

class material_type
{
    public:
        material_id id;
        bool was_loaded = false;

    private:
        translatable_text _name;
        itype_id _salvaged_into = itype_id( "null" ); // this material turns into this item when salvaged
        itype_id _repaired_with = itype_id( "null" ); // this material can be repaired with this item
        int _bash_resist = 0;                         // negative integers means susceptibility
        int _cut_resist = 0;
        int _acid_resist = 0;
        int _elec_resist = 0;
        int _fire_resist = 0;
        int _chip_resist = 0;                         // Resistance to physical damage of the item itself
        int _density = 1;                             // relative to "powder", which is 1
        bool _edible = false;
        bool _soft = false;

        translatable_text _bash_dmg_verb;
        translatable_text _cut_dmg_verb;
        std::vector<translatable_text> _dmg_adj;

        std::map<vitamin_id, double> _vitamins;

        std::array<mat_burn_data, MAX_FIELD_DENSITY> _burn_data;

    public:
        material_type();

        void load( JsonObject &jo, const std::string &src );
        void check() const;

        int dam_resist( damage_type damtype ) const;

        material_id ident() const;
        translatable_text name() const;
        itype_id salvaged_into() const;
        itype_id repaired_with() const;
        int bash_resist() const;
        int cut_resist() const;
        translatable_text bash_dmg_verb() const;
        translatable_text cut_dmg_verb() const;
        translatable_text dmg_adj( int damage ) const;
        int acid_resist() const;
        int elec_resist() const;
        int fire_resist() const;
        int chip_resist() const;
        int density() const;
        bool edible() const;
        bool soft() const;

        double vitamin( const vitamin_id &id ) const {
            auto iter = _vitamins.find( id );
            return iter != _vitamins.end() ? iter->second : 0;
        }

        const mat_burn_data &burn_data( size_t intensity ) const;
};

namespace materials
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

}

#endif
