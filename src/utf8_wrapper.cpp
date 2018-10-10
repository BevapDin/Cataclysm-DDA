#include "utf8_wrapper.h"

#include "catacharset.h"

// In an attempt to maintain compatibility with gcc 4.6, use an initializer function
// instead of a delegated constructor.
// When we declare a hard dependency on gcc 4.7+, turn this back into a delegated constructor.
void utf8_wrapper::init_utf8_wrapper()
{
    const char *utf8str = _data.c_str();
    int len = _data.length();
    while( len > 0 ) {
        const uint32_t ch = UTF8_getch( &utf8str, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        _length++;
        _display_width += mk_wcwidth( ch );
    }
}

utf8_wrapper::utf8_wrapper( const std::string &d ) : _data( d ), _length( 0 ), _display_width( 0 )
{
    init_utf8_wrapper();
}

utf8_wrapper::utf8_wrapper( const char *d ) : _length( 0 ), _display_width( 0 )
{
    _data = std::string( d );
    init_utf8_wrapper();
}

size_t utf8_wrapper::byte_start( size_t bstart, size_t start ) const
{
    if( bstart >= _data.length() ) {
        return _data.length();
    }
    const char *utf8str = _data.c_str() + bstart;
    int len = _data.length() - bstart;
    while( len > 0 && start > 0 ) {
        const uint32_t ch = UTF8_getch( &utf8str, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        start--;
    }
    return utf8str - _data.c_str();
}

size_t utf8_wrapper::byte_start_display( size_t bstart, size_t start ) const
{
    if( bstart >= _data.length() ) {
        return _data.length();
    }
    if( start == 0 ) {
        return 0;
    }
    const char *utf8str = _data.c_str() + bstart;
    int len = _data.length() - bstart;
    while( len > 0 ) {
        const char *prevstart = utf8str;
        const uint32_t ch = UTF8_getch( &utf8str, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        const int width = mk_wcwidth( ch );
        if( static_cast<int>( start ) >= width ) {
            // If width is 0, include the code point (might be combination character)
            // Same when width is actually smaller than start
            start -= width;
        } else {
            // If width is 2 and start is 1, stop before the multi-cell code point
            // Same when width is 1 and start is 0.
            return prevstart - _data.c_str();
        }
    }
    return _data.length();
}

void utf8_wrapper::insert( size_t start, const utf8_wrapper &other )
{
    const size_t bstart = byte_start( 0, start );
    _data.insert( bstart, other._data );
    _length += other._length;
    _display_width += other._display_width;
}

utf8_wrapper utf8_wrapper::substr( size_t start, size_t length ) const
{
    return substr_byte( byte_start( 0, start ), length, false );
}

utf8_wrapper utf8_wrapper::substr_display( size_t start, size_t length ) const
{
    return substr_byte( byte_start_display( 0, start ), length, true );
}

utf8_wrapper utf8_wrapper::substr_byte( size_t bytestart, size_t length,
                                        bool use_display_width ) const
{
    if( length == 0 || bytestart >= _data.length() ) {
        return utf8_wrapper();
    }
    size_t bend;
    if( use_display_width ) {
        bend = byte_start_display( bytestart, length );
    } else {
        bend = byte_start( bytestart, length );
    }
    return utf8_wrapper( _data.substr( bytestart, bend - bytestart ) );
}

long utf8_wrapper::at( size_t start ) const
{
    const size_t bstart = byte_start( 0, start );
    const char *utf8str = _data.c_str() + bstart;
    int len = _data.length() - bstart;
    while( len > 0 ) {
        const uint32_t ch = UTF8_getch( &utf8str, &len );
        if( ch != UNKNOWN_UNICODE ) {
            return ch;
        }
    }
    return 0;
}

void utf8_wrapper::erase( size_t start, size_t length )
{
    const size_t bstart = byte_start( 0, start );
    const utf8_wrapper removed( substr_byte( bstart, length, false ) );
    _data.erase( bstart, removed._data.length() );
    _length -= removed._length;
    _display_width -= removed._display_width;
}

void utf8_wrapper::append( const utf8_wrapper &other )
{
    _data.append( other._data );
    _length += other._length;
    _display_width += other._display_width;
}

utf8_wrapper &utf8_wrapper::replace_all( const utf8_wrapper &search, const utf8_wrapper &replace )
{
    for( std::string::size_type i = _data.find( search._data ); i != std::string::npos;
         i = _data.find( search._data, i ) ) {
        erase( i, search.length() );
        insert( i, replace );
        i += replace._data.length();
    }

    return *this;
}

std::string utf8_wrapper::shorten( size_t maxlength ) const
{
    if( display_width() <= maxlength ) {
        return str();
    }
    return substr_display( 0, maxlength - 1 ).str() + "\u2026"; // 2026 is the utf8 for …
}
