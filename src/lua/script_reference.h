#pragma once
#ifndef SCRIPT_REFERENCE_H
#define SCRIPT_REFERENCE_H

#include "call.h"

#include <string>

class lua_engine;

class JsonIn;

namespace catalua
{

class script_reference
{
    private:
        int id_;

    public:
        script_reference( const std::string &script );
        script_reference( const script_reference & ) = default;
        script_reference &operator=( const script_reference & ) = default;
        script_reference( JsonIn &jsin );

        int id() const {
            return id_;
        }
};

} // namespace catalua

#endif
