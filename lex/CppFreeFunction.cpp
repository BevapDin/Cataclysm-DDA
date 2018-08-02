#include "CppFreeFunction.h"

#include "Exporter.h"
#include "Cursor.h"
#include "Type.h"
#include "exceptions.h"

CppFreeFunction::CppFreeFunction( const Cursor &c, const size_t arg_count ) : CppCallable( c )
{
    while( arguments_.size() > arg_count ) {
        arguments_.pop_back();
    }
}

CppFreeFunction::~CppFreeFunction() = default;

std::string CppFreeFunction::export_( Exporter &p ) const
{
    // @todo: add support for *some* operators
    if( is_operator() ) {
        if( operator_name().compare(0, 3, "\"\"_") == 0)  {
            throw SkippedObjectError( "user defined literal" );
        }
        throw SkippedObjectError( "operator" );
    }

    std::string line;
    line = line + "{ ";
    const std::string lua_name = p.lua_name( full_name() );
    line = line + "name = \"" + lua_name + "\", ";
    line = line + "rval = " + result_type_as_lua_string( p ) + ", ";
    if( lua_name != full_name().as_string() ) {
        line = line + "cpp_name = \"" + full_name() + "\", ";
    }
    const std::string comment = cursor.raw_comment();
    if( !comment.empty()  && p.export_comments ) {
        //@todo remove doxygen like formatting from the string
        line = line + "comment = \"" + Exporter::escape_to_lua_string( comment ) + "\", ";
    }
    line = line + "args = " + export_arguments( p );
    line = line + " }";
    return line;
}
