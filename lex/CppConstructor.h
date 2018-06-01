#pragma once

#include "CppCallable.h"

#include <string>

class CppClass;
class Parser;
class Exporter;

class CppConstructor : public CppCallable
{
    private:

    public:
        CppConstructor( CppClass &p, const Cursor &c, size_t arg_count );
        CppConstructor( const CppConstructor & ) = delete;

        std::string export_( Exporter &p ) const;

        bool is_public() const;
};
