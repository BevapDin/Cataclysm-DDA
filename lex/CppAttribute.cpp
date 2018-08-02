#include "CppAttribute.h"

#include "CppClass.h"
#include "Cursor.h"
#include "Exporter.h"

CppAttribute::CppAttribute( CppClass &p, const Cursor &c ) : parent_name_( p.full_name() ),
    full_name_( c.fully_qualifid() ), const_qualified_( c.type().is_const_qualified() ),
    public_( c.is_public() ), type_( c.type() )
{
}

FullyQualifiedId CppAttribute::full_name() const
{
    return full_name_;
}

bool CppAttribute::is_const() const
{
    return const_qualified_;
}

bool CppAttribute::is_public() const
{
    return public_;
}

std::string CppAttribute::export_( Exporter &p ) const
{
    std::string line;
    const std::string lua_name = p.lua_name( full_name() );
    line = line + lua_name + " = { ";
    line = line + "type = " + p.translate_member_type( type_ );
    if( !is_const() && !p.is_readonly( full_name() ) ) {
        line = line + ", writable = true";
    }
    // .back because full_name itself includes the name of the class
    if( lua_name != full_name().back() ) {
        line = line + ", cpp_name = \"" + full_name().back() + "\"";
    }
    line = line + " }";
    return line;
}
