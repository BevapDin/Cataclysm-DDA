#pragma once

#include "CppCallable.h"

#include <string>

class CppFreeFunction : public CppCallable {
    private:

    public:
        CppFreeFunction( const Cursor &c, size_t arg_count );
        CppFreeFunction( const CppFreeFunction & ) = delete;
        ~CppFreeFunction() override;

        std::string export_( Exporter &p ) const;
};
