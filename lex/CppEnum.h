#pragma once

#include "FullyQualifiedId.h"

#include <list>
#include <string>

class Parser;
class Exporter;
class Cursor;

class CppEnum
{
    private:
        FullyQualifiedId full_name_;
        std::list<std::string> values;

    public:
        CppEnum( Parser &p, const Cursor &cursor );
        CppEnum( const CppEnum & ) = delete;

        std::string export_( Exporter &p ) const;

        FullyQualifiedId full_name() const;
};
