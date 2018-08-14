#pragma once

#include "Cursor.h"

#include <list>
#include <string>

class Parser;
class Exporter;

class CppEnum
{
    private:
        Cursor cursor;
        std::list<std::string> values;

    public:
        CppEnum( Parser &p, const Cursor &cursor );
        CppEnum( const CppEnum & ) = delete;

        std::string export_( Exporter &p ) const;

        FullyQualifiedId full_name() const;
};
