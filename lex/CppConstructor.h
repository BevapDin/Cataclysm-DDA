#pragma once

#include "CppCallable.h"

#include <string>
#include <list>

class CppClass;
class Parser;
class Exporter;

class CppConstructor : public CppCallable
{
    private:

    public:
        CppConstructor( CppClass &p, const Cursor &c );
        CppConstructor( const CppConstructor & ) = delete;

        std::list<std::string> export_( Exporter &p ) const;

        bool is_public() const;
};
