#pragma once

#include "FullyQualifiedId.h"

#include <string>
#include <memory>
#include <set>
#include <map>

#include "optional.h"

class MultiMatcher;
class Cursor;
class Type;
class CppClass;
class CppAttribute;
class CppFunction;
class CppConstructor;
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
         * Enumerations are automatically added to types_exported_by_value.
         * Key is the actual type name as it appears in C++, value is the name it will
         * have in Lua.
         */
        std::map<FullyQualifiedId, std::string> types_to_export;

        // key is the string_id typedef name (e.g. "mtype_id") and value is the
        // name of the objects it identifiers (e.g. "mtype").
        std::map<std::string, FullyQualifiedId> string_ids;
        std::map<std::string, FullyQualifiedId> int_ids;

        std::unique_ptr<MultiMatcher> readonly_identifiers;
        std::unique_ptr<MultiMatcher> blocked_identifiers;
        std::unique_ptr<MultiMatcher> ignore_result_of_those;

    public:
        Exporter();
        ~Exporter();

        Exporter( const Exporter & ) = delete;

        bool is_blocked( const CppConstructor &obj ) const;
        bool is_blocked( const CppFunction &obj ) const;
        bool is_blocked( const CppAttribute &obj ) const;

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

        cata::optional<std::string> get_header_for_argument( const Type &t ) const;

        void add_export_for_string_id( const std::string &id_name, const FullyQualifiedId &full_name );
        void add_export( const FullyQualifiedId &full_name );
        void add_export( const FullyQualifiedId &full_name, const std::string &lua_name );

        FullyQualifiedId derived_class( const Type &t ) const;

        bool export_enabled( const FullyQualifiedId name ) const;
        bool export_enabled( const Type &name, const std::string &path ) const;
        bool export_enabled( const Type &name ) const;

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

        cata::optional<std::string> get_string_id_for( const FullyQualifiedId &name ) const {
            for( const auto &elem : string_ids ) {
                if( elem.second == name ) {
                    return elem.first;
                }
            }
            return {};
        }
        cata::optional<std::string> get_int_id_for( const FullyQualifiedId &name ) const {
            for( const auto &elem : int_ids ) {
                if( elem.second == name ) {
                    return elem.first;
                }
            }
            return {};
        }

        /**
         * Convert an arbitrary string to be usable as content of a Lua string.
         * (Does not add string quotation marks).
         */
        static std::string escape_to_lua_string( const std::string &text );

        /**
         * Returns the build-in Lua type of the given C++ type, if there is one.
         */
        ///@{
        cata::optional<std::string> build_in_lua_type( const std::string &t ) const;
        cata::optional<std::string> build_in_lua_type( const Type &t ) const;
        ///@}

        void export_( const Parser &parser, const std::string &lua_file );

        void debug_message( const std::string &message ) const;
        void info_message( const std::string &message ) const;
        void error_message( const std::string &message ) const;

        std::string lua_name( const FullyQualifiedId &full_name ) const;
};
