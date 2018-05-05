#pragma once
#ifndef VITAMIN_H
#define VITAMIN_H

#include "string_id.h"
#include "optional.h"

#include <string>
#include <map>
#include <utility>
#include <vector>

class JsonObject;
class vitamin;
using vitamin_id = string_id<vitamin>;
class effect_type;
using efftype_id = string_id<effect_type>;

class vitamin
{
    private:
        friend vitamin_id;

        vitamin() = default;

    public:
        const vitamin_id &id() const {
            return id_;
        }

        const std::string &name() const {
            return name_;
        }

        /** Disease effect with increasing intensity proportional to vitamin deficiency */
        const efftype_id &deficiency() const {
            return deficiency_;
        }

        /** Disease effect with increasing intensity proportional to vitamin excess */
        const cata::optional<efftype_id> &excess() const {
            return excess_;
        }

        /** Lower bound for deficiency of this vitamin */
        int min() const {
            return min_;
        }

        /** Upper bound for any accumulation of this vitamin */
        int max() const {
            return max_;
        }

        /**
         * Usage rate of vitamin (turns to consume unit)
         * Lower bound is zero whereby vitamin is not required (but may still accumulate)
         * If unspecified in JSON a default value of 60 minutes is used
         */
        int rate() const {
            return rate_;
        }

        /** Get intensity of deficiency or zero if not deficient for specified qty */
        int severity( int qty ) const;

        /** Load vitamin from JSON definition */
        static void load_vitamin( JsonObject &jo );

        /** Get all currently loaded vitamins */
        static const std::map<vitamin_id, vitamin> &all();

        /** Check consistency of all loaded vitamins */
        static void check_consistency();

        /** Clear all loaded vitamins (invalidating any pointers) */
        static void reset();

    private:
        vitamin_id id_;
        std::string name_;
        efftype_id deficiency_;
        cata::optional<efftype_id> excess_;
        int min_;
        int max_;
        int rate_;
        std::vector<std::pair<int, int>> disease_;
};

#endif
