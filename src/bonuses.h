#pragma once
#ifndef BONUSES_H
#define BONUSES_H

#include <map>
#include <vector>

enum damage_type : int;

class Character;
class JsonObject;
class JsonArray;

class stat_accessor {
    protected:
        stat_accessor() = default;

    public:
        virtual ~stat_accessor() = default;

        virtual int get( const Character &u ) const = 0;
        virtual void set( Character &u, int value ) const = 0;

        void mod( Character &u, const int delta ) const {
            return set( u, get( u ) + delta );
        }
};

namespace stats
{

extern const stat_accessor &strength;
extern const stat_accessor &dexterity;
extern const stat_accessor &intelligence;
extern const stat_accessor &perception;

extern const stat_accessor &hit;
extern const stat_accessor &dodge;
extern const stat_accessor &block;
extern const stat_accessor &speed;
extern const stat_accessor &move_cost;
extern const stat_accessor &damage;
extern const stat_accessor &armor;
extern const stat_accessor &armor_penetration;
extern const stat_accessor &target_armor_multiplier;

} // namespace stat

// We'll be indexing bonuses with this
struct affected_type {
    public:
        affected_type( const stat_accessor &s );
        affected_type( const stat_accessor &s, damage_type t );
        bool operator<( const affected_type & ) const;
        bool operator==( const affected_type & ) const;

        const stat_accessor &get_stat() const {
            return stat;
        }

        damage_type get_damage_type() const {
            return type;
        }

    private:
        const stat_accessor &stat;
        damage_type type;
};

// This is the bonus we are indexing
struct effect_scaling {
    const stat_accessor &stat;
    float scale;

    float get( const Character &u ) const;

    effect_scaling( JsonArray &jarr );
};

class bonus_container
{
    public:
        bonus_container();
        void load( JsonObject &jo );
        void load( JsonArray &jo, bool mult );

        float get_flat( const Character &u, const stat_accessor &stat, damage_type type ) const;
        float get_flat( const Character &u, const stat_accessor &stat ) const;

        float get_mult( const Character &u, const stat_accessor &stat, damage_type type ) const;
        float get_mult( const Character &u, const stat_accessor &stat ) const;

    private:
        using bonus_map = std::map<affected_type, std::vector<effect_scaling>>;
        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_map bonuses_flat;
        bonus_map bonuses_mult;
};

#endif
