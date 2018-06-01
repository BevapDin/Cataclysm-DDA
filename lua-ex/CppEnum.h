#pragma once

#include <list>
#include <string>

class Parser;
class Exporter;
class Cursor;

class CppEnum
{
    private:
        std::string full_name_;
        std::string cpp_name_;
        std::list<std::string> values;

    public:
        CppEnum( Parser &p, const Cursor &cursor );
        CppEnum( const std::string &ns, const std::string &n );
        CppEnum( const CppEnum & ) = delete;

        std::string export_( Exporter &p ) const;

        std::string cpp_name() const;
        std::string full_name() const;
};
