#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "optional.h"

/**
 * A set of flags, which can be strings or enumeration values.
 * The class allows using either, it does keep the consistency (setting
 * a flag via an enumeration value also sets the matching string flag).
 * This allows using hard coded enumeration values for performance
 * and dynamic flags that are set, unset and checked via JSON.
 */
template<typename enum_type>
class generic_flag_set
{
    private:
        std::unordered_set<enum_type> enum_flags;
        std::unordered_set<std::string> string_flags;

        // To be implemented for each enum_type
        static const std::unordered_map<std::string, enum_type> string_to_enum_map;

    public:
        static cata::optional<enum_type> string_to_enum( const std::string &flag ) {
            const auto &map = string_to_enum_map;
            const auto iter = map.find( flag );
            if( iter != map.end() ) {
                return iter->second;
            }
            return cata::nullopt;
        }
        static cata::optional<std::string> enum_to_string( const enum_type flag ) {
            for( const auto &e : string_to_enum_map ) {
                if( e.second == flag ) {
                    return e.first;
                }
            }
            return cata::nullopt;
        }

        void clear() {
            enum_flags.clear();
            string_flags.clear();
        }

        bool has( const enum_type flag ) const {
            return enum_flags.count( flag ) > 0;
        }
        bool has( const std::string &flag ) const {
            return string_flags.count( flag ) > 0;
        }

        void set( const enum_type flag ) {
            enum_flags.insert( flag );
            if( const auto maybe_flag = enum_to_string( flag ) ) {
                set( *maybe_flag );
            }
        }
        void set( const std::string &flag ) {
            string_flags.insert( flag );
            if( const auto maybe_flag = string_to_enum( flag ) ) {
                set( *maybe_flag );
            }
        }

        void unset( const enum_type flag ) {
            enum_flags.erase( flag );
            if( const auto maybe_flag = enum_to_string( flag ) ) {
                unset( *maybe_flag );
            }
        }
        void unset( const std::string &flag ) {
            string_flags.erase( flag );
            if( const auto maybe_flag = string_to_enum( flag ) ) {
                unset( *maybe_flag );
            }
        }
};
