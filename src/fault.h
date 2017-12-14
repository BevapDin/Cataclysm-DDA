#pragma once
#ifndef FAULT_H
#define FAULT_H

#include "translatable_text.h"
#include "string_id.h"

#include <string>
#include <map>

class JsonObject;

class fault;
using fault_id = string_id<fault>;

class Skill;
using skill_id = string_id<Skill>;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

class fault
{
    public:
        fault() : id_( fault_id( "null" ) ) {}

        const fault_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == fault_id( "null" );
        }

        const translatable_text &name() const {
            return name_;
        }

        const translatable_text &description() const {
            return description_;
        }

        int time() const {
            return time_;
        }

        const std::map<skill_id, int> &skills() const {
            return skills_;
        }

        const requirement_data &requirements() const {
            return requirements_.obj();
        }

        /** Load fault from JSON definition */
        static void load_fault( JsonObject &jo );

        /** Get all currently loaded faults */
        static const std::map<fault_id, fault> &all();

        /** Clear all loaded faults (invalidating any pointers) */
        static void reset();

        /** Checks all loaded from JSON are valid */
        static void check_consistency();

    private:
        fault_id id_;
        translatable_text name_;
        translatable_text description_;
        int time_;
        std::map<skill_id, int> skills_;
        requirement_id requirements_;
};

#endif
