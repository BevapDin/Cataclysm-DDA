#pragma once

#include "clang-c/Index.h"

class TranslationUnit;

class Index {
    private:
        friend class TranslationUnit;

        CXIndex index_;

    public:
        Index();
        Index( const Index & ) = delete;

        ~Index();

        Index &operator=( const Index & ) = delete;
};
