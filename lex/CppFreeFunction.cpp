#include "CppFreeFunction.h"

#include "Exporter.h"
#include "Cursor.h"
#include "Type.h"
#include "exceptions.h"

CppFreeFunction::CppFreeFunction( const Cursor &c ) : CppCallable( c )
{
}

CppFreeFunction::~CppFreeFunction() = default;

std::list<std::string> CppFreeFunction::export_( Exporter &p ) const
{
    return export_cb( p, [&p, this]( const std::string & args ) {
        // @todo: add support for *some* operators
        if( is_operator() ) {
            if( operator_name().compare(0, 3, "\"\"_") == 0)  {
                // user defined literal
                return std::string();
            }
            throw SkippedObjectError( "operator" );
        }
        std::string result;
        try {
            result = p.translate_result_type( cursor.get_result_type() );
        } catch( const TypeTranslationError & ) {
            //@todo
            if( p.ignore_result_of( FullyQualifiedId( full_name_with_args() ) ) ) {
                throw;
            }
            // Otherwise just make the wrapper ignore the result of the function call
            result = "nil";
        }

        std::string line;
        line = line + "{ ";
        const std::string lua_name = p.lua_name( full_name() );
        line = line + "name = \"" + lua_name + "\", ";
        line = line + "rval = " + result + ", ";
        if( lua_name != full_name().as_string() ) {
            line = line + "cpp_name = \"" + full_name() + "\", ";
        }
        const std::string comment = cursor.raw_comment();
        if( !comment.empty()  && p.export_comments ) {
            //@todo remove doxygen like formatting from the string
            line = line + "comment = \"" + p.escape_to_lua_string( comment ) + "\", ";
        }
        line = line + "args = " + args;
        line = line + " }";
        return line;
    } );
}
