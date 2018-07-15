#pragma once

#include "Cursor.h"
#include "FullyQualifiedId.h"

#include <string>

class Exporter;

class CppVariable
{
    private:
        Cursor cursor_;

    public:
        CppVariable( const Cursor &c );
        CppVariable( const CppVariable & ) = delete;

        std::string export_( Exporter &p ) const;

        Type type() const;
        /// Fully qualified name in C++.
        FullyQualifiedId full_name() const;
        /// Whether it's declared as const.
        bool is_const() const;
};
