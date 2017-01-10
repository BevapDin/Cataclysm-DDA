#ifndef UI_TABS_H
#define UI_TABS_H

#include "cursesdef.h"

#include <vector>
#include <string>
#include <functional>

// only for WINDOW_PTR
#include "output.h"

class ui_tabs {
    private:
        class tab {
            public:
                std::string caption;
                std::function<bool(ui_tabs &)> callback;
        };
        
        std::vector<tab> _tabs;
        size_t _current = 0;

        WINDOW_PTR w_ptr;
        WINDOW *w;
        
    public:
        /**
         * This works exactly like @ref newwin
         */
        ui_tabs(int y, int x, int dy, int dx);
        ui_tabs(WINDOW *w);
        ~ui_tabs();

        WINDOW *window() const { return w; }

        template<typename T>
        void add(const std::string &caption, T callback) {
            _tabs.push_back( tab{ caption, callback } );
        }

        void draw_tabs();
        void draw_tab( int offset_x, size_t index ) const;

        size_t current() const { return _current; }
        void current( const size_t v ) { _current = v; }
        size_t size() const { return _tabs.size(); }

        void next() { current(std::min(current() + 1, _tabs.size())); }
        void prev() { current(current() == 0 ? 0 : current() - 1); }

        void loop();
};

#endif
