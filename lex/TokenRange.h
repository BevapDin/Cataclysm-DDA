#pragma once

#include "clang-c/Index.h"

#include <string>

class Token
{
    private:
        const CXToken &t;
        const CXTranslationUnit &tu;

    public:
        Token( const CXToken &t, const CXTranslationUnit &tu ) : t( t ), tu( tu ) { }
        Token( const Token & ) = default;

        std::string spelling() const;
};


class TokenRange
{
    private:
        friend class Iterator;

        CXToken *tokens = nullptr;
        unsigned count = 0;
        const CXTranslationUnit &tu;

    public:
        TokenRange( const CXTranslationUnit &tu, const CXSourceRange &extent );
        TokenRange( TokenRange &&other );
        ~TokenRange();

        class Iterator
        {
            private:
                friend class TokenRange;

                const CXToken *ptr;
                const CXTranslationUnit &tu;

                Iterator( const CXToken *const ptr, const CXTranslationUnit &tu ) : ptr( ptr ), tu( tu ) { }

            public:
                Iterator( const Iterator & ) = default;

                void operator++() {
                    ++ptr;
                }
                bool operator==( const Iterator &rhs ) const {
                    return ptr == rhs.ptr;
                }
                bool operator!=( const Iterator &rhs ) const {
                    return !operator==( rhs );
                }
                Token operator*() const {
                    return Token( *ptr, tu );
                }
        };

        Iterator begin() const;
        Iterator end() const;
};
