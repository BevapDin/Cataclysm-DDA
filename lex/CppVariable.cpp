#include "CppVariable.h"

#include "Type.h"
#include "Exporter.h"

CppVariable::CppVariable( const Cursor &c ) : cursor_( c )
{
}

FullyQualifiedId CppVariable::full_name() const
{
    return cursor_.fully_qualifid();
}

bool CppVariable::is_const() const
{
    return type().is_const_qualified();
}

Type CppVariable::type() const {
    return cursor_.type();
}

std::string CppVariable::export_( Exporter &p ) const
{
    std::string line;
    const std::string lua_name = p.lua_name( full_name() );
    line = line + lua_name + " = { ";
    if( lua_name != full_name().as_string() ) {
        line = line + "cpp_name = \"" + full_name() + "\", ";
    }
    line = line + "type = " + p.translate_member_type( type() );
    if( !is_const() && !p.is_readonly( full_name() ) ) {
        line = line + ", writable = true";
    }
    line = line + " }";
    return line;
}
