#pragma once

#include "clang-c/Index.h"

#include <string>
#include <vector>
#include <functional>

class Type;
class FullyQualifiedId;
class TranslationUnit;
class TokenRange;

/**
 * Simple wrapper over @ref CXCursor.
 */
class Cursor
{
    private:
        CXCursor cursor_;
        std::reference_wrapper<const TranslationUnit> tu;

    public:
        Cursor( const CXCursor cursor, const TranslationUnit &tu ) : cursor_( cursor ), tu( tu ) { }

        using VisitorType = std::function<CXChildVisitResult( const Cursor &, const Cursor &)>;
        void visit_children( const VisitorType &func ) const;

        std::string spelling() const;

        CXCursorKind kind() const {
            return cursor_.kind;
        }

        Type type() const;
        Type get_result_type() const;
        Type get_underlying_type() const;

        bool has_default_value() const;
        bool is_static_method() const;
        bool is_const_method() const;
        bool is_public() const;
        bool is_definition() const;

        std::string raw_comment() const;

        std::string location() const;
        std::string location_path() const;
        std::string location_file() const;

        std::vector<Cursor> get_arguments() const;
        std::vector<Type> get_template_arguments() const;

        Cursor get_definition() const;
        Cursor get_semantic_parent() const;

        FullyQualifiedId fully_qualifid() const;

        void dump( const std::string &msg ) const;

        bool operator==( const Cursor &rhs ) const;
        bool operator!=( const Cursor &rhs ) const { return !operator==(rhs); }

        TokenRange tokens() const;
};

std::string str( const CXCursorKind &c );
