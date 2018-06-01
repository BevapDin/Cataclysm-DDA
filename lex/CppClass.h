#pragma once

#include "Cursor.h"

#include <string>
#include <list>
#include <vector>
#include <functional>

class CppFunction;
class CppAttribute;
class CppConstructor;
class Parser;
class Exporter;
class FullyQualifiedId;

class CppClass
{
    private:
        Cursor cursor_;

        std::list<CppFunction> functions;
        std::list<CppAttribute> attributes;
        std::list<CppConstructor> constructors;
        std::list<std::reference_wrapper<const CppClass>> parents;

        bool has_non_const_overload( const CppFunction &func ) const;

        void gather_parent( Exporter &p, std::vector<std::reference_wrapper<const CppFunction>> &functions, std::vector<std::reference_wrapper<const CppAttribute>> &attributes ) const;

        std::vector<std::string> get_headers() const;

    public:
        CppClass( Parser &p, const Cursor &cursor );
        CppClass( const CppClass & ) = delete;

        ~CppClass();

        std::string export_( Exporter &p ) const;

        FullyQualifiedId full_name() const;

        bool has_equal() const;
};
