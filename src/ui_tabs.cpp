#include "ui_tabs.h"

ui_tabs::ui_tabs(int y, int x, int dy, int dx)
: w_ptr( newwin( y, x, dy, dx ) )
, w( w_ptr.get() )
{
}

ui_tabs::ui_tabs(WINDOW *w)
: w_ptr()
, w(w)
{
}

ui_tabs::~ui_tabs() = default;

void ui_tabs::draw()
{
    werase( w );

    // borders
    for (int i = 1; i < getmaxx(w) - 1; i++) {
        mvwputch(w, 2, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w, 4, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w, getmaxy(w) - 1, i, BORDER_COLOR, LINE_OXOX);

        if (i > 2 && i < getmaxy(w) - 1) {
            mvwputch(w, i, 0, BORDER_COLOR, LINE_XOXO);
            mvwputch(w, i, getmaxx(w) - 1, BORDER_COLOR, LINE_XOXO);
        }
    }

    size_t tabs_length = 0;
    std::vector<int> tab_len;
    for (const auto &t : _tabs) {
        // +2 for the border
        const size_t len = utf8_width(t->caption) + 2;
        tabs_length += len;
        tab_len.push_back( len );
    }

    int next_pos = 2;
    // Free space on _tabs window. '<', '>' symbols is drawning on free space.
    // Initial value of next_pos is free space too.
    // '1' is used for SDL/curses screen column reference.
    const int free_space = (getmaxx(w) - tabs_length - 1 - next_pos);
    const int spaces = std::max( free_space / static_cast<int>(_tabs.size()) - 1, 0);
    for (size_t i = 0; i < _tabs.size(); ++i) {
        draw_tab(next_pos, i);
        next_pos += tab_len[i] + spaces;
    }

    mvwputch(w, 2,  0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w, 2, getmaxx(w) - 1, BORDER_COLOR, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, BORDER_COLOR, LINE_XXXO); // |-
    mvwputch(w, 4, getmaxx(w) - 1, BORDER_COLOR, LINE_XOXX); // -|

    mvwputch(w, getmaxy(w) - 1, 0, BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(w, getmaxy(w) - 1, getmaxx(w) - 1, BORDER_COLOR, LINE_XOOX); // _|
}

void ui_tabs::draw_tab( int offset_x, const size_t index ) const
{
    const bool selected = index == _current;
    const std::string &caption = _tabs[index]->caption;
    
    int offset_x_right = offset_x + utf8_width( caption ) + 1;

    mvwputch( w, 0, offset_x,       c_ltgray, LINE_OXXO ); // |^
    mvwputch( w, 0, offset_x_right, c_ltgray, LINE_OOXX ); // ^|
    mvwputch( w, 1, offset_x,       c_ltgray, LINE_XOXO ); // |
    mvwputch( w, 1, offset_x_right, c_ltgray, LINE_XOXO ); // |

    mvwprintz( w, 1, offset_x + 1, selected ? h_ltgray : c_ltgray, "%s", caption.c_str() );

    for( int i = offset_x + 1; i < offset_x_right; i++ ) {
        mvwputch( w, 0, i, c_ltgray, LINE_OXOX );  // -
    }

    if( selected ) {
        mvwputch( w, 1, offset_x - 1,       h_ltgray, '<' );
        mvwputch( w, 1, offset_x_right + 1, h_ltgray, '>' );

        for( int i = offset_x + 1; i < offset_x_right; i++ ) {
            mvwputch( w, 2, i, c_black, ' ' );
        }

        mvwputch( w, 2, offset_x,       c_ltgray, LINE_XOOX ); // _|
        mvwputch( w, 2, offset_x_right, c_ltgray, LINE_XXOO ); // |_

    } else {
        mvwputch( w, 2, offset_x,       c_ltgray, LINE_XXOX ); // _|_
        mvwputch( w, 2, offset_x_right, c_ltgray, LINE_XXOX ); // _|_
    }
}
