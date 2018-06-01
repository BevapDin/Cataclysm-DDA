#include "TokenRange.h"

#include "common-clang.h"

TokenRange::TokenRange( const CXTranslationUnit &tu, const CXSourceRange &extent ) : tu( tu )
{
    clang_tokenize( tu, extent, &tokens, &count );
}

TokenRange::TokenRange( TokenRange &&other ) : tokens( other.tokens ), count( other.count ),
    tu( other.tu )
{
    other.count = 0;
}

TokenRange::~TokenRange()
{
    if( count > 0 ) {
        clang_disposeTokens( tu, tokens, count );
    }
}

TokenRange::Iterator TokenRange::begin() const
{
    return Iterator( tokens, tu );
}

TokenRange::Iterator TokenRange::end() const
{
    return Iterator( tokens + count, tu );
}

std::string Token::spelling() const
{
    return string( &clang_getTokenSpelling, tu, t );
}

