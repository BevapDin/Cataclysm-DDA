#pragma once
#ifndef FLAG_H
#define FLAG_H

#include <set>
#include <map>
#include <string>

class JsonObject;
template<typename TagType>
class json_flag;

/**
 * @tparam TagType A type to define where the flags are applied to, e.g. an
 * item flag and an item type flag are different classes.
 */
template<typename TagType>
class json_flag_manager {
    private:
        friend class DynamicDataLoader;

        std::map<std::string, json_flag<TagType>> all_;

    public:
        json_flag_manager() = default;

        /** Fetches flag definition (or null flag if not found) */
        const json_flag<TagType> &get( const std::string &id );

        /** Load flag definition from JSON */
        void load( JsonObject &jo );

        /** Check consistency of all loaded flags */
        void check_consistency();

        /** Clear all loaded flags (invalidating any pointers) */
        void reset();

};

template<typename TagType>
class json_flag
{
        friend json_flag_manager<TagType>;

    public:
        /** Get identifier of flag as specified in JSON */
        const std::string &id() const {
            return id_;
        }

        /** Get informative text for display in UI */
        const std::string &info() const {
            return info_;
        }

        /** Is flag inherited by base items from any attached items? */
        bool inherit() const {
            return inherit_;
        }

        /** Is this a valid (non-null) flag */
        operator bool() const {
            return !id_.empty();
        }

    private:
        const std::string id_;
        std::string info_;
        std::set<std::string> conflicts_;
        bool inherit_ = true;

        json_flag( const std::string &id = std::string() ) : id_( id ) {}
};

#endif
