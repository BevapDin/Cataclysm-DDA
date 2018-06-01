#pragma once

#include "Type.h"
#include "FullyQualifiedId.h"

#include <string>

class CppClass;
class Cursor;
class Exporter;

class CppAttribute
{
    private:
        FullyQualifiedId parent_name_;
        FullyQualifiedId full_name_;
        bool const_qualified_;
        bool public_;

        Type type_;

    public:
        CppAttribute( CppClass &p, const Cursor &c );
        CppAttribute( const CppAttribute & ) = delete;

        std::string export_( Exporter &p ) const;

        /// Fully qualified name in C++.
        FullyQualifiedId full_name() const;
        /// Whether it's declared as const member.
        bool is_const() const;
        /// Whether it's declared as public member.
        bool is_public() const;
};
