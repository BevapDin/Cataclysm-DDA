#include "CppFunction.h"

#include "CppClass.h"
#include "Type.h"
#include "Cursor.h"
#include "Parser.h"
#include "Exporter.h"
#include "exceptions.h"

CppFunction::CppFunction( CppClass &/*p*/, const Cursor &c, const size_t arg_count ) : CppCallable( c )
{
    while( arguments_.size() > arg_count ) {
        arguments_.pop_back();
    }
    cursor.visit_children( [this]( const Cursor & c, const Cursor &/*parent*/ ) {
        const CXCursorKind k = c.kind();
        if( k == CXCursor_CXXOverrideAttr ) {
            overridden = true;
            return CXChildVisit_Continue;
        }
        return CXChildVisit_Continue;
    } );
}

CppFunction::~CppFunction() = default;

std::string CppFunction::export_( Exporter &p ) const
{
    // @todo: add support for *some* operators
    if( is_operator() ) {
        throw SkippedObjectError( "operator" );
    }
    if( is_deleted() ) {
        throw SkippedObjectError( "deleted" );
    }
    // Overridden methods are ignored because the parent class already contains
    // them and we scan the parent class and include it in the output anyway.
    if( overridden ) {
        throw SkippedObjectError( "overridden" );
    }

    std::string line;
    line = line + "{ ";
    const std::string lua_name = p.lua_name( full_name() );
    line = line + "name = \"" + lua_name + "\", ";
    if( cursor.is_static_method() ) {
        line = line + "static = true, ";
    }
    line = line + "rval = " + result_type_as_lua_string( p ) + ", ";
    // .back because full_name itself includes the name of the class
    if( lua_name != full_name().back() ) {
        line = line + "cpp_name = \"" + full_name().back() + "\", ";
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

bool CppFunction::is_const_method() const
{
    return cursor.is_const_method();
}

bool CppFunction::is_static() const
{
    return cursor.is_static_method();
}

FullyQualifiedId CppFunction::full_name() const
{
    // @todo
    return FullyQualifiedId( ( is_static() ? "static " : "" ) + CppCallable::full_name() );
}

bool CppFunction::is_public() const
{
    return cursor.is_public();
}
