#pragma once
#ifndef GLYPH_H
#define GLYPH_H

#include "color.h"
#include "optional.h"

#include <string>

class JsonObject;

class glyph
{
    private:
        /** UTF-8 encoded symbol, should be exactly one cell wide. */
        std::string symbol_;
        nc_color color_;

    public:
        glyph();
        glyph( std::string s, nc_color c );

        const std::string &symbol() const {
            return symbol_;
        }
        const nc_color &color() const {
            return color_;
        }

        enum required_or_optional {
            required,
            optional,
        };

        /// Load @ref symbol_ from member of given JSON object.
        void load_symbol( JsonObject &jo, const std::string &name, required_or_optional roo );
        /// Load @ref color_ from member of given JSON object.
        void load_color( JsonObject &jo, const std::string &name, required_or_optional roo );
};

#endif
