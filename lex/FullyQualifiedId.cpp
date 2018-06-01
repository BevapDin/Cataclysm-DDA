#include "FullyQualifiedId.h"

#include <cassert>

FullyQualifiedId::FullyQualifiedId( const FullyQualifiedId &ns, const std::string &name )
    : id_( ns.id_ )
{
    if( !id_.empty() ) {
        id_ += "::";
    }
    id_ += name;
}

FullyQualifiedId::FullyQualifiedId( const std::string &id )
    : id_( id )
{
}

std::string FullyQualifiedId::back() const
{
    const auto p = id_.rfind( "::" );
    if( p == std::string::npos ) {
        return id_;
    } else {
        return id_.substr( p + 2 );
    }
}

bool FullyQualifiedId::is_in_namespace( const FullyQualifiedId &ns ) const
{
    if( id_ == ns.id_ ) {
        return true;
    }
    if( id_.compare( 0, ns.id_.length() + 2, ns.id_ + "::" ) == 0 ) {
        return true;
    }
    return false;
}
