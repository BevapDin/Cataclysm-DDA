#ifndef UI_TABS_H
#define UI_TABS_H

#include "cursesdef.h"

#include <vector>
#include <string>
#include <functional>

// only for WINDOW_PTR
#include "output.h"

class input_context;

class ui_tabs {
    public:
        class tab {
            protected:
                tab() = default;
            public:
                std::string caption;
                virtual ~tab() = default;
        };
        
    private:
        std::vector<std:.unique_ptr<tab>> _tabs;
        size_t _current = 0;

        WINDOW_PTR w_ptr;
        WINDOW *w;

        void draw_tab( int offset_x, size_t index ) const;

    public:
        /**
         * This works exactly like @ref newwin
         */
        ui_tabs(int y, int x, int dy, int dx);
        ui_tabs(WINDOW *w);
        ~ui_tabs();

        WINDOW *window() const { return w; }

        void add(Tab *t) {
            _tabs.emplace_back(t);
        }

        void draw_tab();

        size_t current() const { return _current; }
        void current( const size_t v ) { _current = v; }
        size_t size() const { return _tabs.size(); }

        void next() { current(std::min(current() + 1, _tabs.size())); }
        void prev() { current(current() == 0 ? 0 : current() - 1); }

        void register_actions(input_context &ctxt) const;
        bool handle_action( const std::string &action );
};

#endif
