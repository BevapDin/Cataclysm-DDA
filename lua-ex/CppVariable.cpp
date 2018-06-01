#include "CppVariable.h"

#include "Type.h"
#include "exceptions.h"
#include "Exporter.h"

CppVariable::CppVariable( const Cursor &c ) : cursor_( c )
{
}

std::string CppVariable::full_name() const
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
    try {
        if( p.is_blocked( full_name() ) ) {
            return "-- " + full_name() + " skipped because it's blocked";
        }

        std::string line;
        const std::string lua_name = p.translate_identifier( cursor_.spelling() );
        line = line + lua_name + " = { ";
        if( lua_name != cursor_.fully_qualifid() ) {
            line = line + "cpp_name = \"" + cursor_.fully_qualifid() + "\", ";
        }
        line = line + "type = " + p.translate_member_type( type() );
        if( !is_const() && !p.is_readonly( full_name() ) ) {
            line = line + ", writable = true";
        }
        line = line + " }";
        return line;
    } catch( const TypeTranslationError &e ) {
        return "-- " + full_name() + " ignored because: " + e.what();
    } catch( const SkippedObjectError &e ) {
        return "-- " + full_name() + " ignored because: " + e.what();
    }
}
