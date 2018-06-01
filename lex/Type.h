#pragma once

#include "clang-c/Index.h"

#include <string>
#include <functional>

class Cursor;
class TranslationUnit;

/**
 * Simple wrapper over @ref CXType.
 */
class Type
{
    private:
        CXType type_;
        std::reference_wrapper<const TranslationUnit> tu;

    public:
        Type( const CXType type, const TranslationUnit &tu ) : type_( type ), tu( tu ) { }

        std::string spelling() const;

        CXTypeKind kind() const {
            return type_.kind;
        }

        Type get_canonical_type() const;
        Type get_pointee() const;

        Cursor get_declaration() const ;

        bool operator==( const Type &rhs ) const;
        bool operator!=( const Type &rhs ) const {
            return !operator==( rhs );
        }
        bool is_const_qualified() const;

        void dump( const std::string &msg ) const;
};

std::string str( const CXTypeKind &t );
