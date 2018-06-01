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
        //@todo handle delete attribute?
//        c.dump( "Cursor kind not handled upon parsing class function definition" );
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
        throw SkippedObjectError( "static member function, not supported" );
    }
    line = line + "rval = " + result_type_as_lua_string( p ) + ", ";
    // .back because full_name itself includes the name of the class
    if( lua_name != full_name().back() ) {
        line = line + "cpp_name = \"" + full_name().back() + "\", ";
    }
    line = line + "args = " + export_arguments( p );
    line = line + " }";
    return line;
}

bool CppFunction::is_const_method() const
{
    return cursor.is_const_method();
}

FullyQualifiedId CppFunction::full_name() const
{
    return CppCallable::full_name();
}

bool CppFunction::is_public() const
{
    return cursor.is_public();
}
