#include "CppCallable.h"

#include "Exporter.h"
#include "Type.h"
#include "common.h"
#include "exceptions.h"
#include "FullyQualifiedId.h"
#include "TokenRange.h"

CppCallable::CppCallable( const Cursor &c ) : cursor( c ), arguments_( c.get_arguments() )
{
}

CppCallable::~CppCallable() = default;

size_t CppCallable::min_arguments( const Cursor &c )
{
    size_t result = 0;
    for( const Cursor &arg : c.get_arguments() ) {
        if( arg.has_default_value() ) {
            break;
        } else {
            ++result;
        }
    }
    return result;
}

size_t CppCallable::max_arguments( const Cursor &c )
{
    return c.get_arguments().size();
}

std::string CppCallable::export_arguments( Exporter &p ) const
{
    if( arguments_.empty() ) {
        return "{ }";
    }
    return enumerate( "{ ", std::vector<Cursor>( arguments_.begin(), arguments_.end() ), [&p]( const Cursor & a ) {
        return p.translate_argument_type( a.type() );
    }, ", ", " }" );
}

bool is_valid_in_identifier( const char c )
{
    return c == '_' || ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' );
}

std::string trim( const std::string &text )
{
    const auto p = text.find_first_not_of( "\n\r\t " );
    //    const auto o = text.find_last_not_of( "\n\r\t " );
    if( p != std::string::npos ) {
        text.substr( p );
    }
    return text;
}

std::string CppCallable::operator_name() const
{
    static const std::string op( "operator" );
    // Note: spelling, not fully qualified because we want "operator==", not "std::string::operator=="
    std::string name = cursor.spelling();
    if( name.length() <= op.length() || name.compare( 0, op.length(), op ) != 0 ||
        is_valid_in_identifier( name[op.length()] ) ) {
        return std::string();
    }
    return trim( name.erase( 0, op.length() ) );
}

bool CppCallable::is_operator() const
{
    return !operator_name().empty();
}

bool CppCallable::has_same_arguments( const CppCallable &other ) const
{
    const auto &fargs = arguments_;
    const auto &oargs = other.arguments_;
    if( fargs.size() != oargs.size() ) {
        return false;
    }
    for( auto f = fargs.begin(), o = oargs.begin(); f != fargs.end() && o != oargs.end(); ++f, ++o ) {
        const Type F = f->type().get_canonical_type();
        const Type O = o->type().get_canonical_type();
        if( F != O ) {
            return false;
        }
    }
    return true;
}

FullyQualifiedId CppCallable::full_name() const
{
    return cursor.fully_qualifid();
}

std::string CppCallable::full_name_with_args() const
{
    const std::string args = enumerate( "", arguments_, [&]( const Cursor & a ) {
        return a.type().spelling();
    }, ", ", "" );
    return full_name() + "(" + args + ")";
}

Type CppCallable::result_type() const{
    return cursor.get_result_type();
}

std::string CppCallable::result_type_as_lua_string( Exporter &p ) const
{
    try {
        return p.translate_result_type( result_type() );
    } catch( const TypeTranslationError & ) {
        if( p.ignore_result_of( FullyQualifiedId( full_name_with_args() ) ) ) {
            // Make the wrapper ignore the result of the function call.
            return "nil";
        } else {
            throw;
        }
    }
}

bool CppCallable::is_deleted() const {
    for( const Token &t : cursor.tokens() ) {
        if( t.spelling() == "delete" ) {
            return true;
        }
    }
    return false;
}
