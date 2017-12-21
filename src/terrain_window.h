#pragma once
#ifndef TERRAIN_WINDOW_H
#define TERRAIN_WINDOW_H

#include "cursesdef.h"
#include "enums.h"

#include <memory>
#include <vector>

class cata_tiles;
class tripoint_range;

class terrain_window {
    public:
        using map_coord = tripoint;
        using screen_coord = point;

    private:
        catacurses::WINDOW *w;
        tripoint center_;

    public:
        terrain_window( catacurses::WINDOW *w ) : w ( w ) {
        }

        void center( const tripoint &c ) {
            center_ = c;
        }
        const tripoint &center() const {
            return center_;
        }

        bool contains( const map_coord &pos ) const;
        bool contains( const screen_coord &pos ) const;
        map_coord to_map_coord( const screen_coord &pos ) const;
        screen_coord to_screen_coord( const map_coord &pos ) const;

        /**
         * Returns a range that contains all points visible in this window,
         * in map coordinates. The range may contain points that are not
         * valid in the reality bubble (e.g. if the window is larger than
         * the map). The points all have the same z-component: `center().z`.
         */
        tripoint_range map_range() const;

        // operators so that this class can be used like any ordinary curses window
        operator catacurses::window() const {
            return w;
        }
        operator catacurses::WINDOW*() const {
            return w;
        }

};

class terrain_window_drawer {
    public:
        class priority_type {
            private:
                int priority_;

            public:
                priority_type( const int p = 0 ) : priority_( p ) { }

                bool operator<( const priority_type &rhs ) const {
                    return priority_ < rhs.priority_;
                }
        };

    protected:
        priority_type priority_;

    public:
        const priority_type &priority() const {
            return priority_;
        }
        
        terrain_window_drawer() = default;
        terrain_window_drawer( const priority_type &p ) : priority_( p ) { }
        virtual ~terrain_window_drawer() = default;

        virtual void draw( terrain_window &win ) = 0;
#ifdef TILES
        virtual void draw( cata_tiles &tilecontext ) = 0;
#else
        // dummy implementation, not used in non-tiles builds
        void draw( cata_tiles &/*tilecontext*/ ) { };
#endif
};

class terrain_window_drawers {
    private:
        std::vector<std::unique_ptr<terrain_window_drawer>> drawers;

    public:
        void insert( std::unique_ptr<terrain_window_drawer> drawer_ptr );

        template<typename T, typename ...Args>
        void emplace( Args &&... args ) {
            std::unique_ptr<terrain_window_drawer> ptr( new T( std::forward<Args>( args )... ) );
            insert( std::move( ptr ) );
        }

        void clear();

        void draw( terrain_window &win );
        void draw( cata_tiles &tilecontext );
        /**
         * Draws either to the @ref cata_tiles or to game::w_terrain, depending on
         * the build type and the option settings @ref use_tiles.
         */
        void draw();
};

#endif
