#pragma once

#include "clang-c/Index.h"

#include <vector>
#include <string>

class Cursor;
class Index;

class TranslationUnit {
    private:
        CXTranslationUnit tu;
        friend class Cursor;

    public:
        TranslationUnit( const Index &index, const std::vector<const char*> &args, const unsigned flags );
        TranslationUnit( const Index &index, const std::vector<const char*> &args, const std::string &text, const unsigned flags );
        TranslationUnit( const TranslationUnit & ) = delete;

        ~TranslationUnit();

        TranslationUnit &operator=( const TranslationUnit & ) = delete;

        std::vector<std::string> get_diagnostics() const;
        std::vector<std::string> get_includes() const;

        Cursor cursor() const;
};
