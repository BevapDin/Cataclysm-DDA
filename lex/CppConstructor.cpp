#include "CppConstructor.h"

#include "CppClass.h"
#include "common.h"

CppConstructor::CppConstructor( CppClass &/*p*/, const Cursor &c, const size_t arg_count ) : CppCallable( c )
{
    while( arguments_.size() > arg_count ) {
        arguments_.pop_back();
    }
    //@todo handle delete attribute
}

std::string CppConstructor::export_( Exporter &p ) const
{
    return export_arguments( p );
}

bool CppConstructor::is_public() const
{
    return cursor.is_public();
}
