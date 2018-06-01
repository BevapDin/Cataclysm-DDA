#pragma once

#include <type_traits>
#include <string>

template<typename C, typename F>
inline std::string enumerate( const std::string &prefix, const C &container, const F &func,
                              const std::string &separator, const std::string &postfix )
{
    if( container.empty() ) {
        return std::string();
    }
    bool first = true;
    std::string result = prefix;
    for( const auto &e : container ) {
        if( first ) {
            first = false;
        } else {
            result += separator;
        }
        result += func( e );
    }
    return result + postfix;
}
