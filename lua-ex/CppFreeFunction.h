#pragma once

#include "CppCallable.h"

#include <string>
#include <list>

class CppFreeFunction : public CppCallable {
    private:

    public:
        CppFreeFunction( const Cursor &c );
        CppFreeFunction( const CppFreeFunction & ) = delete;
        ~CppFreeFunction() override;

        std::list<std::string> export_( Exporter &p ) const;
};
