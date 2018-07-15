#pragma once

#include "FullyQualifiedId.h"

#include <string>
#include <memory>
#include <set>
#include <map>

class MultiMatcher;
class Cursor;
class Type;
class CppClass;
class Parser;

bool valid_cpp_identifer( const std::string &ident );

class Exporter
{
    private:
        friend int main( int argc, const char *argv[] );
        friend class Parser;
        /**
         * All the C++ types that should be exported to Lua. This should contain the
         * real type, not a type alias. It can contain enumeration or class types.
         * Enumerations are automatically added to types_exported_by_value. Class types
         * must be added manually to types_exported_by_value or types_exported_by_reference
         * or to both.
         */
        std::set<FullyQualifiedId> types_to_export;
        /**
         * C++ types that should be exported by-value. They must support copy-construction
         * and copy-assignment. Use the plain C++ name of their type. Note: enumerations
         * and string_id and int_id are automatically exported as value, they don't need to
         * be listed here.
         */
        std::set<FullyQualifiedId> types_exported_by_value;
        /**
         * C++ types that should be exported by-reference. They don't need to support
         * copy-construction nor copy-assignment. Types may be exported by-value and
         * by-reference.
         */
        std::set<FullyQualifiedId> types_exported_by_reference;

        std::map<std::string, std::string> generic_types;

        // key is the string_id typedef name (e.g. "mtype_id") and value is the
        // name of the objects it identifiers (e.g. "mtype").
        std::map<std::string, FullyQualifiedId> string_ids;
        std::map<std::string, FullyQualifiedId> int_ids;

        std::unique_ptr<MultiMatcher> readonly_identifiers;
        std::unique_ptr<MultiMatcher> blocked_identifiers;
        std::unique_ptr<MultiMatcher> ignore_result_of_those;
        std::unique_ptr<MultiMatcher> is_readonly_identifiers;

        std::string export_all_in_path_;

    public:
        Exporter();
        ~Exporter();

        void export_all_in( const std::string &path );

        bool export_comments = false;

        Exporter( const Exporter & ) = delete;

        bool is_blocked( const FullyQualifiedId &name ) const;
        bool is_readonly( const FullyQualifiedId &name ) const;
        bool ignore_result_of( const FullyQualifiedId &name ) const;

        /**
         * Translate the given type to a string that is usable in Lua as name for the
         * given type.
         * Note that usage (member/global variable/static member vs argument vs return type)
         * can affect this.
         * @throws Errors when the type is not available in Lua.
         */
        ///@{
        std::string translate_member_type( const Type &t ) const;
        std::string translate_argument_type( const Type &t ) const;
        std::string translate_result_type( const Type &t ) const;
        ///@}

        std::string get_header_for_argument( const Type &t ) const;

        void add_export_for_string_id( const std::string &id_name, const FullyQualifiedId &full_name );
        void add_export_by_value( const FullyQualifiedId &full_name );
        void add_export_by_reference( const FullyQualifiedId &full_name );
        void add_export_by_value_and_reference( const FullyQualifiedId &full_name );
        void add_export_enumeration( const FullyQualifiedId &full_name );

        FullyQualifiedId derived_class( const Type &t ) const;

        bool export_enabled( const FullyQualifiedId name ) const;
        bool export_enabled( const Type &name, const std::string &path ) const;

        bool export_by_value( const FullyQualifiedId &name ) const;
        bool export_by_value( const Type &name ) const;
        bool export_by_reference( const FullyQualifiedId &name ) const;
        bool export_by_reference( const Type &name ) const;

        bool add_id_typedef( const Cursor &cursor, const std::string &id_type,
                             std::map<std::string, FullyQualifiedId> &ids_map );

        bool register_id_typedef( const Cursor &cursor );

        /**
         * Given a C++ identifier, returns valid Lua identifier that should
         * be used in place of this one.
         * Usually this just returns the input
         * as is. Only when the input does not form a valid identifier in Lua
         * (e.g. `end`), it returns something else.
         */
        std::string translate_identifier( const std::string &name ) const;

        std::string get_string_id_for( const FullyQualifiedId &name ) const {
            for( const auto &elem : string_ids ) {
                if( elem.second == name ) {
                    return elem.first;
                }
            }
            return std::string();
        }
        std::string get_int_id_for( const FullyQualifiedId &name ) const {
            for( const auto &elem : int_ids ) {
                if( elem.second == name ) {
                    return elem.first;
                }
            }
            return std::string();
        }

        /**
         * Convert an arbitrary string to be usable as content of a Lua string.
         * (Does not add string quotation marks).
         */
        std::string escape_to_lua_string( const std::string &text ) const;

        /**
         * Returns the build-in Lua type of the given C++ type, if there is one. It returns
         * an empty string if there is no matching build-in type.
         */
        ///@{
        std::string build_in_lua_type( const std::string &t ) const;
        std::string build_in_lua_type( const Type &t ) const;
        ///@}

        std::string register_std_container( const std::string name, const Type &t );
        std::string register_std_iterator( const std::string &name, const Type &t );
        std::string register_generic( const Type &t );

        void export_( const Parser &parser, const std::string &lua_file );

        void debug_message( const std::string &message ) const;
        void info_message( const std::string &message ) const;
        void error_message( const std::string &message ) const;

        std::string lua_name( const FullyQualifiedId &full_name ) const;
};
