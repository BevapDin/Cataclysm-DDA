#include "scrollable_list.h"

#include "output.h"

void scrollable_list::draw() const
{
    const scrollable_data &data = data_.get();
    const size_t count = data.count();
    for( int i = 0; i < height; ++i ) {
        const size_t index = i + scroll_offset_;
        if( index >= count ) {
            break;
        }
        const bool active = index == static_cast<size_t>( cursor_ );
        nc_color color = active ? active_color_ : default_color_;
        print_colored_text( w, i + y, x, color, color, data.get_entry_by_index( index, active ) );
    }
}

void scrollable_list::data( scrollable_data &new_data ) {
    data_ = std::ref( new_data );
    cursor( 0 );
    scroll_offset_ = 0;
}

void scrollable_list::erase() {
    const std::string empty_line( width, ' ' );
    for( int i = 0; i < height; ++i ) {
        mvwprintw( w, y + i, x, empty_line.c_str() );
    }
}

void scrollable_list::draw_scrollbar( const int offset_y, const int offset_x,
                                      const nc_color bar_color ) const
{
    ::draw_scrollbar( w, cursor_, page_size(), count(), offset_y, offset_x, bar_color );
}

void scrollable_list::cursor( const int new_cursor ) {
    cursor_ = new_cursor;
    if( cursor_ < 0 ) {
        cursor_ = count() - 1;
    } else if( cursor_ >= count() ) {
        cursor_ = 0;
    }
    calcStartPos( scroll_offset_, cursor_, page_size(), count() );
}
