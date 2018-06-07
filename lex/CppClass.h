#pragma once

#include "Cursor.h"

#include <string>
#include <list>
#include <set>
#include <functional>

class CppFunction;
class CppAttribute;
class CppConstructor;
class Parser;
class Exporter;

class CppClass
{
    private:
        std::string full_name_;
        std::string cpp_name_;

        std::list<CppFunction> functions;
        std::list<CppAttribute> attributes;
        std::list<CppConstructor> constructors;
        std::list<std::reference_wrapper<const CppClass>> parents;
        std::set<std::string> headers_;

        bool has_equal = false;
        std::string source_file_;

        bool has_non_const_overload( const CppFunction &func ) const;

        void gather_parent( Exporter &p, std::vector<const CppFunction *> &functions,
                            std::vector<const CppAttribute *> &attributes ) const;

    public:
        CppClass( Parser &p, const Cursor &cursor );
        CppClass( const std::string &ns, const std::string &n );
        CppClass( const CppClass & ) = delete;

        ~CppClass();

        std::string export_( Exporter &p ) const;

        std::string cpp_name() const;
        std::string full_name() const;
};
