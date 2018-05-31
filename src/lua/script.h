#pragma once
#ifndef SCRIPT_H
#define SCRIPT_H

#include <string>

namespace catalua
{

template<typename value_type, typename ...Args>
class script
{
    private:
        std::string script_;

    public:
        script() = default;
        script( const std::string &s ) : script_( s ) { }

        value_type operator()( Args ... args ) const;
};

} // namespace catalua

#endif
