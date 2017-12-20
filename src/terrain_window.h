#pragma once
#ifndef TERRAIN_WINDOW_H
#define TERRAIN_WINDOW_H

#include "cursesdef.h"
#include "enums.h"

#include <memory>
#include <vector>

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

        class priority_type {
            private:
                int priority_;

            public:
                priority_type( const int p = 0 ) : priority_( p ) { }

                bool operator<( const priority_type &rhs ) const {
                    return priority_ < rhs.priority_;
                }
        };

        class drawer {
            protected:
                priority_type priority_;

            public:
                const priority_type &priority() const {
                    return priority_;
                }

                drawer() = default;
                drawer( const priority_type &p ) : priority_( p ) { }
                virtual ~drawer() = default;

                virtual void draw( terrain_window &win ) = 0;
        };

        void insert( std::unique_ptr<drawer> drawer_ptr );

        template<typename T, typename ...Args>
        void emplace( Args &&... args ) {
            std::unique_ptr<drawer> ptr( new T( std::forward<Args>( args )... ) );
            insert( std::move( ptr ) );
        }

        void clear();
        void draw();

        operator catacurses::window() const {
            return w;
        }
        operator catacurses::WINDOW*() const {
            return w;
        }

        // operators so that this class can be used like any ordinary curses window
        operator catacurses::window() const {
            return w;
        }
        operator catacurses::WINDOW*() const {
            return w;
        }

    private:
        std::vector<std::unique_ptr<drawer>> drawers;
};

#endif
