#include "CppEnum.h"

#include "Cursor.h"
#include "Parser.h"
#include "Exporter.h"

CppEnum::CppEnum( Parser &p, const Cursor &cursor ) : full_name_( cursor.fully_qualifid() ), cpp_name_( cursor.spelling() )
{
    p.debug_message( "Parsing enum " + full_name() + " at " + cursor.location() );
    cursor.visit_children( [this]( const Cursor & c, const Cursor &/*parent*/ ) {
        if( c.kind() == CXCursor_EnumConstantDecl ) {
            values.emplace_back( c.spelling() );
            return CXChildVisit_Continue;
        }
        c.dump( "Cursor kind not handled upon parsing enumeration definition" );
        return CXChildVisit_Continue;
    } );
}

CppEnum::CppEnum( const std::string &ns, const std::string &n ) : full_name_( Parser::fully_qualifid(ns, n)), cpp_name_(n)
{ }

std::string CppEnum::export_( Exporter &p ) const
{
    p.debug_message( "Exporting enum " + cpp_name() );
    std::string r;
    //@todo lua name
    r = r + "    " + cpp_name() + " = {\n";
    for( const std::string &a : values ) {
        r = r + "        \"" + a + "\",\n";
    }
    r = r + "    }";
    return r;
}

std::string CppEnum::cpp_name() const
{
    return cpp_name_;
}

std::string CppEnum::full_name() const
{
    return full_name_;
}
