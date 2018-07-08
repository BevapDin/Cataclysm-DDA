#pragma once

#include "CppCallable.h"

#include <string>
#include <list>

class CppClass;

class CppFunction : public CppCallable
{
    private:
        bool overridden = false;

    public:
        CppFunction( CppClass &p, const Cursor &c );
        CppFunction( const CppFunction & ) = delete;
        ~CppFunction() override;

        std::list<std::string> export_( Exporter &p ) const;

        bool is_const_method() const;
        bool is_static() const;
        bool is_public() const;

        std::string full_name() const override;
};
