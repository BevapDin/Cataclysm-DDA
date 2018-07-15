#include "CppAttribute.h"

#include "CppClass.h"
#include "Cursor.h"
#include "exceptions.h"
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

std::list<std::string> CppAttribute::export_( Exporter &p ) const
{
    try {
        if( p.is_blocked( full_name() ) ) {
            return std::list<std::string> { { "-- " + full_name() + " skipped because it's blocked" } };
        }

        std::string line;
        const std::string lua_name = p.lua_name( full_name() );
        line = line + lua_name + " = { ";
        line = line + "type = " + p.translate_member_type( type_ );
        if( !is_const() && !p.is_readonly( full_name() ) ) {
            line = line + ", writable = true";
        }
        if( lua_name != full_name().as_string() ) {
            line = line + "cpp_name = \"" + full_name() + "\", ";
        }
        line = line + " }";
        return std::list<std::string> { { line } };
    } catch( const TypeTranslationError &e ) {
        return std::list<std::string> { { "-- " + full_name() + " ignored because: " + e.what() } };
    } catch( const SkippedObjectError &e ) {
        return std::list<std::string> { { "-- " + full_name() + " ignored because: " + e.what() } };
    }
}
