#pragma once

#include "Type.h"

#include <string>
#include <list>

class CppClass;
class Cursor;
class Exporter;

class CppAttribute
{
    private:
        std::string parent_name_;
        std::string cpp_name_;
        bool const_qualified_;
        bool public_;

        Type type_;

    public:
        CppAttribute( CppClass &p, const Cursor &c );
        CppAttribute( const CppAttribute & ) = delete;

        std::list<std::string> export_( Exporter &p ) const;

        /// Fully qualified name in C++.
        std::string full_name() const;
        /// Simple name as it appears in C++.
        std::string cpp_name() const;
        /// Whether it's declared as const member.
        bool is_const() const;
        /// Whether it's declared as public member.
        bool is_public() const;
};
