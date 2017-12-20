#pragma once
#ifndef TERRAIN_WINDOW_H
#define TERRAIN_WINDOW_H

#include "cursesdef.h"
#include "enums.h"

#include <memory>
#include <vector>

class terrain_window {
    private:
        catacurses::WINDOW *&w;
        tripoint center_;

    public:
        terrain_window( catacurses::WINDOW *&w ) : w ( w ) {
        }

        void center( const tripoint &c ) {
            center_ = center;
        }
        const tripoint &center() const {
            return center_;
        }

        class drawer {
            public:
                drawer() = default;
                virtual ~drawer() = default;

                virtual void draw( const catacurses::window &w, const tripoint &center ) = 0;
        };

        void add( std::unique_ptr<drawer> drawer_ptr ) {
            drawers.emplace_back( std::move( drawer_ptr ) )
        }
        void clear() {
            drawers.clear();
        }

        void draw() {
            const auto copy = drawers;
            for( const std::unique_ptr<drawer> &ptr : copy ) {
                ptr->draw( w, center_ );
            }
            // Place the cursor over the player as is expected by screen readers.
            wmove( w, POSY + g->u.pos().y - center_.y, POSX + g->u.pos().x - center_.x );

            wrefresh( w );
        }

    private:
        std::vector<std::unique_ptr<drawer>> drawers;
};

#endif
