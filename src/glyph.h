#pragma once
#ifndef GLYPH_H
#define GLYPH_H

#include "color.h"
#include "string_id.h"

#include <string>

class tile;
using tile_id = string_id<tile>;

class glyph {
    private:
        std::string symbol_;
        nc_color color_;
#ifdef TILES
        tile_id tile_;
#endif
    public:
        const std::string &symbol() const {
            return symbol_;
        }
        void set_symbol( std::string s ) {
            symbol_ = std::move( s );
        }

        const nc_color &color() const {
            return color_;
        }
        void set_color( nc_color c ) {
            color_ = c;
        }

        // should only be called from cata_tiles, which is only compiled with
        // TILES being defined, so the function is not used otherwise
        #ifdef TILES
        const tile_id &tile() const {
            return tile_;
        }
        #endif
        void set_tile( tile_id i ) {
            #ifdef TILES
            tile_ = std::move( i );
            #else
            static_cast<void>( i ); // unused
            #endif
        }
};

#endif
