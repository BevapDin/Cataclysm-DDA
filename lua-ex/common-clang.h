#pragma once

#include "clang-c/Index.h"

#include <string>

/**
 * Converts a @ref CXString into a @ref std::string.
 * First argument is the clang function that returns a `CXString`,
 * all further arguments are forwarded to that function and the
 * returned string is converted into a `std::string` and returned.
 */
template < typename F, typename ...Args >
inline std::string string( F func, Args &&... args )
{
    const CXString s = func( std::forward<Args>( args )... );
    const char *const c = clang_getCString( s );
    const std::string result( c ? c : "" );
    clang_disposeString( s );
    return result;
}
