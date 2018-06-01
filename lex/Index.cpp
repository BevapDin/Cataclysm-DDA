#include "Index.h"

#include "common-clang.h"

#include <stdexcept>

Index::Index() : index_( clang_createIndex( 0, 0 ) )
{
    if( !index_ ) {
        throw std::runtime_error( "could not create index" );
    }
}

Index::~Index()
{
    clang_disposeIndex( index_ );
}
