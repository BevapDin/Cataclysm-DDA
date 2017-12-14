#pragma once
#ifndef AMMO_H
#define AMMO_H

#include "translatable_text.h"

#include <string>

class JsonObject;

using itype_id = std::string;

class ammunition_type
{
        friend class DynamicDataLoader;
    public:
        ammunition_type() = default;
        explicit ammunition_type( translatable_text name ) : name_( std::move( name ) ) { }

        translatable_text name() const;

        itype_id const &default_ammotype() const {
            return default_ammotype_;
        }

    private:
        translatable_text name_;
        itype_id default_ammotype_;

        static void load_ammunition_type( JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

#endif
