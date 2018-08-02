#pragma once

#include "Index.h"

#include "clang-c/Index.h"
#include "FullyQualifiedId.h"

#include <string>
#include <vector>
#include <memory>
#include <set>
#include <list>
#include <map>

class TranslationUnit;
class MultiMatcher;
class Cursor;
class Type;
class CppEnum;
class CppClass;
class CppFreeFunction;
class Exporter;

class Parser
{
    private:
        friend class Exporter;

        Exporter &exporter;

        Index index;
        std::list<TranslationUnit> tus;

        std::set<std::string> skipped_entities;

        std::list<CppEnum> enums;
        std::list<CppClass> classes;
        std::list<CppFreeFunction> functions;

        template<typename C>
        static const typename C::value_type *get_from( const FullyQualifiedId &what, const C &where ) {
            for( const auto &existing : where ) {
                if( existing.full_name() == what ) {
                    return &existing;
                }
            }
            return nullptr;
        }
        template<typename C>
        static bool contains( const FullyQualifiedId &what, const C &where ) {
            return get_from( what, where ) != nullptr;
        }

    public:
        static const FullyQualifiedId cpp_standard_namespace;

        Parser( Exporter &exporter );
        ~Parser();

        Parser( const Parser & ) = delete;

        const CppClass *get_class( const FullyQualifiedId &full_name ) const;
        bool contains_class( const FullyQualifiedId &full_name ) const;
        CppClass &add_class( const Cursor &c );
        CppClass &get_or_add_class( const Cursor &c );

        const CppEnum *get_enum( const FullyQualifiedId &full_name ) const;
        bool contains_enum( const FullyQualifiedId &full_name ) const;

        void skipped( const std::string &what, const FullyQualifiedId &name, const std::string &why );

        void parse( const std::string &header );
        void parse( const Cursor &cursor );
        void parse_typedef( const Cursor &cursor);
        void parse_class( const Cursor &cursor );
        void parse_enum( const Cursor &cursor );
        void parse_function( const Cursor &cursor );
        void parse_union( const Cursor &cursor );
        void parse_namespace( const Cursor &cursor );

        void debug_message( const std::string &message ) const;
        void info_message( const std::string &message ) const;
        void error_message( const std::string &message ) const;
};
