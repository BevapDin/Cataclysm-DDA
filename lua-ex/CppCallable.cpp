#include "CppCallable.h"

#include "Exporter.h"
#include "Type.h"
#include "exceptions.h"
#include "common.h"

CppCallable::CppCallable( const Cursor &c ) : cursor( c ), arguments( cursor.get_arguments() )
{
    for( const Cursor &arg : arguments ) {
        if( arg.has_default_value() ) {
            break;
        } else {
            ++min_arguments;
        }
    }
}

CppCallable::~CppCallable() = default;

std::string CppCallable::export_argument_list( Exporter &p, size_t m ) const
{
    if( m == 0 ) {
        return "{ }";
    }
    auto end = arguments.begin();
    std::advance( end, std::min( m, arguments.size() ) );
    return enumerate( "{ ", std::vector<Cursor>( arguments.begin(), end ), [&p]( const Cursor & a ) {
        return p.translate_argument_type( a.type() );
    }, ", ", " }" );
}

std::list<std::string> CppCallable::export_cb( Exporter &p,
        const std::function<std::string( const std::string & )> &callback )const
{
    if( p.is_blocked( full_name_with_args() ) ) {
        return std::list<std::string> { { "-- " + full_name() + " skipped because it's blocked" } };
    }
    std::list<std::string> result;
    try {
        size_t m = min_arguments;
        while( m <= arguments.size() ) {
            const std::string str = callback( export_argument_list( p, m ) );
            if( !str.empty() ) {
                result.push_back( str );
            }
            ++m;
        }
    } catch( const TypeTranslationError &e ) {
        result.push_back( "-- " + full_name() + " ignored because: " + e.what() );
    } catch( const SkippedObjectError &e ) {
        result.push_back( "-- " + full_name() + " ignored because: " + e.what() );
    }
    return result;
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

bool CppCallable::has_same_arguments( const CppCallable &other ) const
{
    const auto &fargs = arguments;
    const auto &oargs = other.arguments;
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

std::string CppCallable::full_name() const
{
    return cursor.fully_qualifid();
}

std::string CppCallable::full_name_with_args() const
{
    const std::string args = enumerate( "", arguments, [&]( const Cursor & a ) {
        return a.type().spelling();
    }, ", ", "" );
    return full_name() + "(" + args + ")";
}

std::string CppCallable::cpp_name() const {
    return cursor.spelling();
}
