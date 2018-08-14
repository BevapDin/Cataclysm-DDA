#pragma once

#include "Cursor.h"
#include "optional.h"

#include <list>
#include <string>

class Parser;
class Exporter;

class CppEnum
{
    private:
        Cursor cursor_;
        std::list<std::string> values;

    public:
        CppEnum( Parser &p, const Cursor &cursor );
        CppEnum( const CppEnum & ) = delete;

        std::string export_( Exporter &p ) const;

        FullyQualifiedId full_name() const;
        std::string forward_declaration() const;
        cata::optional<std::string> underlying_cpp_type() const;
        bool is_enum_class() const;
};
