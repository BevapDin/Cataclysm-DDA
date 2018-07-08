#include "CppFunction.h"

#include "CppClass.h"
#include "Type.h"
#include "Cursor.h"
#include "Parser.h"
#include "Exporter.h"
#include "exceptions.h"

CppFunction::CppFunction( CppClass &/*p*/, const Cursor &c ) : CppCallable( c )
{
    cursor.visit_children( [this]( const Cursor & c, const Cursor &/*parent*/ ) {
        const CXCursorKind k = c.kind();
        if( k == CXCursor_CXXOverrideAttr ) {
            overridden = true;
            return CXChildVisit_Continue;
        }
        //@todo handle delete attribute?
//        c.dump( "Cursor kind not handled upon parsing class function definition" );
        return CXChildVisit_Continue;
    } );
}

CppFunction::~CppFunction() = default;

std::list<std::string> CppFunction::export_( Exporter &p ) const
{
    return export_cb( p, [&p, this]( const std::string & args ) {
        // @todo: add support for *some* operators
        if( is_operator() ) {
            throw SkippedObjectError( "operator" );
        }
        // Overridden methods are ignored because the parent class already contains
        // them and we scan the parent class and include it in the output anyway.
        if( overridden ) {
            return std::string(); // silently ignored.
        }
        std::string result;
        try {
            result = p.translate_result_type( cursor.get_result_type() );
        } catch( const TypeTranslationError & ) {
            if( p.ignore_result_of( full_name_with_args() ) ) {
                throw;
            }
            // Otherwise just make the wrapper ignore the result of the function call
            result = "nil";
        }

        std::string line;
        line = line + "{ ";
        const std::string lua_name = p.translate_identifier( cpp_name() );
        line = line + "name = \"" + lua_name + "\", ";
        if( cursor.is_static_method() ) {
            line = line + "static = true, ";
        }
        line = line + "rval = " + result + ", ";
        if( lua_name != cpp_name() ) {
            line = line + "cpp_name = \"" + cpp_name() + "\", ";
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

bool CppFunction::is_const_method() const
{
    return cursor.is_const_method();
}

bool CppFunction::is_static() const
{
    return cursor.is_static_method();
}

std::string CppFunction::full_name() const
{
    return ( is_static() ? "static " : "" ) + CppCallable::full_name();
}

bool CppFunction::is_public() const
{
    return cursor.is_public();
}
