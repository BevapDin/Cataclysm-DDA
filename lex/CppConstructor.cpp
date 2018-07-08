#include "CppConstructor.h"

#include "CppClass.h"
#include "common.h"

CppConstructor::CppConstructor( CppClass &/*p*/, const Cursor &c ) : CppCallable( c )
{
    //@todo handle delete attribute
}

std::list<std::string> CppConstructor::export_( Exporter &p ) const
{
    return export_cb( p, [&]( const std::string & args ) {
        return args;
    } );
}

bool CppConstructor::is_public() const
{
    return cursor.is_public();
}
