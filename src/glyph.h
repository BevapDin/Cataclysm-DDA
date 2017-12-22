#pragma once
#ifndef GLYPH_H
#define GLYPH_H

#include "color.h"
#include "string_id.h"

#include <string>

class tile;
using tile_id = string_id<tile>;

class glyph
{
    private:
        std::string symbol_;
        nc_color color_;

    public:
        glyph() : symbol_( " " ), color_( c_unset ) { }
        glyph( std::string s, const nc_color c ) : symbol_( std::move( s ) ), color_( c ) { }
        glyph( const char s, const nc_color c ) : symbol_( 1, s ), color_( c ) { }

        const std::string &symbol() const {
            return symbol_;
        }
        void symbol( std::string s ) {
            symbol_ = std::move( s );
        }
        void symbol( const char c ) {
            symbol_ = std::string( 1, c );
        }

        const nc_color &color() const {
            return color_;
        }
        void color( nc_color c ) {
            color_ = c;
        }
};

#endif
