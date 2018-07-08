#include "CppAttribute.h"

#include "CppClass.h"
#include "Cursor.h"
#include "exceptions.h"
#include "Exporter.h"

CppAttribute::CppAttribute( CppClass &p, const Cursor &c ) : parent_name_( p.full_name() ),
    cpp_name_( c.spelling() ), const_qualified_( c.type().is_const_qualified() ),
    public_( c.is_public() ), type_( c.type() )
{
}

std::string CppAttribute::full_name() const
{
    return parent_name_ + "::" + cpp_name();
}

std::string CppAttribute::cpp_name() const
{
    return cpp_name_;
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
        const std::string lua_name = p.translate_identifier( cpp_name() );
        line = line + lua_name + " = { ";
        line = line + "type = " + p.translate_member_type( type_ );
        if( !is_const() && !p.is_readonly( full_name() ) ) {
            line = line + ", writable = true";
        }
        if( lua_name != cpp_name() ) {
            line = line + "cpp_name = \"" + cpp_name() + "\", ";
        }
        line = line + " }";
        return std::list<std::string> { { line } };
    } catch( const TypeTranslationError &e ) {
        return std::list<std::string> { { "-- " + full_name() + " ignored because: " + e.what() } };
    } catch( const SkippedObjectError &e ) {
        return std::list<std::string> { { "-- " + full_name() + " ignored because: " + e.what() } };
    }
}
