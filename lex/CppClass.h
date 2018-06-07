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

class CppClass
{
    private:
        Cursor cursor_;

        std::list<CppFunction> functions;
        std::list<CppAttribute> attributes;
        std::list<CppConstructor> constructors;
        std::list<std::reference_wrapper<const CppClass>> parents;

        bool has_equal = false;

        bool has_non_const_overload( const CppFunction &func ) const;

        void gather_parent( Exporter &p, std::vector<std::reference_wrapper<const CppFunction>> &functions, std::vector<std::reference_wrapper<const CppAttribute>> &attributes ) const;

        std::vector<std::string> get_headers() const;

    public:
        CppClass( Parser &p, const Cursor &cursor );
        CppClass( const CppClass & ) = delete;

        ~CppClass();

        std::string export_( Exporter &p ) const;

        std::string cpp_name() const;
        std::string full_name() const;
};
