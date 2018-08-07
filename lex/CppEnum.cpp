#include "CppEnum.h"

#include "Cursor.h"
#include "Parser.h"
#include "Exporter.h"

CppEnum::CppEnum( Parser &p, const Cursor &cursor ) : full_name_( cursor.fully_qualifid() )
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

std::string CppEnum::export_( Exporter &p ) const
{
    static const std::string tab( 4, ' ' );
    p.debug_message( "Exporting enum " + full_name() );
    const std::string lua_name = p.lua_name( full_name() );
    std::string r;
    r = r + "enums[\"" + lua_name + "\"] = {\n";
    if( lua_name != full_name().as_string() ) {
        r = r + tab + "cpp_name = \"" + Exporter::escape_to_lua_string( full_name().as_string() ) + "\",\n";
    }
    r = r + tab + "values = {\n";
    for( const std::string &a : values ) {
        r = r + tab + "    \"" + a + "\",\n";
    }
    r = r + tab + "}";
    r = r + "}";
    return r;
}

FullyQualifiedId CppEnum::full_name() const
{
    return full_name_;
}
