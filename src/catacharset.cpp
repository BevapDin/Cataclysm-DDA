#include "catacharset.h"
#include <string.h>
#include "debug.h"
#include "cursesdef.h"
#include "wcwidth.h"
#include "options.h"
#include "utf8_wrapper.h"
#if (defined _WIN32 || defined WINDOWS)
#include "platform_win.h"
#include "mmsystem.h"
#endif

//copied from SDL2_ttf code
//except type changed from unsigned to uint32_t
uint32_t UTF8_getch( const char **src, int *srclen )
{
    const unsigned char *p = *( const unsigned char ** )src;
    int left = 0;
    bool overlong = false;
    bool underflow = false;
    uint32_t ch = UNKNOWN_UNICODE;

    if( *srclen == 0 ) {
        return UNKNOWN_UNICODE;
    }
    if( p[0] >= 0xFC ) {
        if( ( p[0] & 0xFE ) == 0xFC ) {
            if( p[0] == 0xFC && ( p[1] & 0xFC ) == 0x80 ) {
                overlong = true;
            }
            ch = ( uint32_t )( p[0] & 0x01 );
            left = 5;
        }
    } else if( p[0] >= 0xF8 ) {
        if( ( p[0] & 0xFC ) == 0xF8 ) {
            if( p[0] == 0xF8 && ( p[1] & 0xF8 ) == 0x80 ) {
                overlong = true;
            }
            ch = ( uint32_t )( p[0] & 0x03 );
            left = 4;
        }
    } else if( p[0] >= 0xF0 ) {
        if( ( p[0] & 0xF8 ) == 0xF0 ) {
            if( p[0] == 0xF0 && ( p[1] & 0xF0 ) == 0x80 ) {
                overlong = true;
            }
            ch = ( uint32_t )( p[0] & 0x07 );
            left = 3;
        }
    } else if( p[0] >= 0xE0 ) {
        if( ( p[0] & 0xF0 ) == 0xE0 ) {
            if( p[0] == 0xE0 && ( p[1] & 0xE0 ) == 0x80 ) {
                overlong = true;
            }
            ch = ( uint32_t )( p[0] & 0x0F );
            left = 2;
        }
    } else if( p[0] >= 0xC0 ) {
        if( ( p[0] & 0xE0 ) == 0xC0 ) {
            if( ( p[0] & 0xDE ) == 0xC0 ) {
                overlong = true;
            }
            ch = ( uint32_t )( p[0] & 0x1F );
            left = 1;
        }
    } else {
        if( ( p[0] & 0x80 ) == 0x00 ) {
            ch = ( uint32_t ) p[0];
        }
    }
    ++*src;
    --*srclen;
    while( left > 0 && *srclen > 0 ) {
        ++p;
        if( ( p[0] & 0xC0 ) != 0x80 ) {
            ch = UNKNOWN_UNICODE;
            break;
        }
        ch <<= 6;
        ch |= ( p[0] & 0x3F );
        ++*src;
        --*srclen;
        --left;
    }
    if( left > 0 ) {
        underflow = true;
    }
    if( overlong || underflow ||
        ( ch >= 0xD800 && ch <= 0xDFFF ) ||
        ( ch == 0xFFFE || ch == 0xFFFF ) || ch > 0x10FFFF ) {
        ch = UNKNOWN_UNICODE;
    }
    return ch;
}

std::string utf32_to_utf8( uint32_t ch )
{
    char out[5];
    char *buf = out;
    static const unsigned char utf8FirstByte[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
    int utf8Bytes;
    if( ch < 0x80 ) {
        utf8Bytes = 1;
    } else if( ch < 0x800 ) {
        utf8Bytes = 2;
    } else if( ch < 0x10000 ) {
        utf8Bytes = 3;
    } else if( ch <= 0x10FFFF ) {
        utf8Bytes = 4;
    } else {
        utf8Bytes = 3;
        ch = UNKNOWN_UNICODE;
    }

    buf += utf8Bytes;
    switch( utf8Bytes ) {
        case 4:
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 3:
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 2:
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 1:
            *--buf = ch | utf8FirstByte[utf8Bytes];
    }
    out[utf8Bytes] = '\0';
    return out;
}

//Calculate width of a Unicode string
//Latin characters have a width of 1
//CJK characters have a width of 2, etc
int utf8_width( const char *s, const bool ignore_tags )
{
    int len = strlen( s );
    const char *ptr = s;
    int w = 0;
    bool inside_tag = false;
    while( len > 0 ) {
        uint32_t ch = UTF8_getch( &ptr, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        if( ignore_tags ) {
            if( ch == '<' ) {
                inside_tag = true;
            } else if( ch == '>' ) {
                inside_tag = false;
                continue;
            }
            if( inside_tag ) {
                continue;
            }
        }
        w += mk_wcwidth( ch );
    }
    return w;
}

int utf8_width( const std::string &str, const bool ignore_tags )
{
    return utf8_width( str.c_str(), ignore_tags );
}

int utf8_width( const utf8_wrapper &str, const bool ignore_tags )
{
    return utf8_width( str.c_str(), ignore_tags );
}

//Convert cursor position to byte offset
//returns the first character position in bytes behind the cursor position.
//If the cursor is not on the first half of the character,
//prevpos (which points to the first byte of the cursor located char)
// should be a different value.
int cursorx_to_position( const char *line, int cursorx, int *prevpos, int maxlen )
{
    int dummy;
    int i = 0;
    int c = 0;
    int *p = prevpos ? prevpos : &dummy;
    *p = 0;
    while( c < cursorx ) {
        const char *utf8str = line + i;
        int len = ANY_LENGTH;
        if( utf8str[0] == 0 ) {
            break;
        }
        uint32_t ch = UTF8_getch( &utf8str, &len );
        int cw = mk_wcwidth( ch );
        len = ANY_LENGTH - len;
        if( len <= 0 ) {
            len = 1;
        }
        if( maxlen >= 0 && maxlen < ( i + len ) ) {
            break;
        }
        i += len;
        if( cw <= 0 ) {
            cw = 0;
        }
        c += cw;
        if( c <= cursorx ) {
            *p = i;
        }
    }
    return i;
}

std::string utf8_truncate( std::string s, size_t length )
{
    int last_pos;

    if( length == 0 || s.empty() ) {
        return s;
    }

    last_pos = cursorx_to_position( s.c_str(), length, NULL, -1 );

    return s.substr( 0, last_pos );
}

static const std::array<char, 64> base64_encoding_table = {{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '-'
    }
};

static       std::array<char, 256> base64_decoding_table;
static const std::array<int, 3> mod_table = {{0, 2, 1}};

static void build_base64_decoding_table()
{
    static bool built = false;
    if( !built ) {
        for( int i = 0; i < 64; i++ ) {
            base64_decoding_table[( unsigned char ) base64_encoding_table[i]] = i;
        }
        built = true;
    }
}

std::string base64_encode( std::string str )
{
    //assume it is already encoded
    if( str.length() > 0 && str[0] == '#' ) {
        return str;
    }

    int input_length = str.length();
    int output_length = 4 * ( ( input_length + 2 ) / 3 );

    std::string encoded_data( output_length, '\0' );
    const unsigned char *data = ( const unsigned char * )str.c_str();

    for( int i = 0, j = 0; i < input_length; ) {

        unsigned octet_a = i < input_length ? data[i++] : 0;
        unsigned octet_b = i < input_length ? data[i++] : 0;
        unsigned octet_c = i < input_length ? data[i++] : 0;

        unsigned triple = ( octet_a << 0x10 ) + ( octet_b << 0x08 ) + octet_c;

        encoded_data[j++] = base64_encoding_table[( triple >> 3 * 6 ) & 0x3F];
        encoded_data[j++] = base64_encoding_table[( triple >> 2 * 6 ) & 0x3F];
        encoded_data[j++] = base64_encoding_table[( triple >> 1 * 6 ) & 0x3F];
        encoded_data[j++] = base64_encoding_table[( triple >> 0 * 6 ) & 0x3F];
    }

    for( int i = 0; i < mod_table[input_length % 3]; i++ ) {
        encoded_data[output_length - 1 - i] = '=';
    }

    return "#" + encoded_data;
}

std::string base64_decode( std::string str )
{
    // do not decode if it is not base64
    if( str.length() == 0 || str[0] != '#' ) {
        return str;
    }

    build_base64_decoding_table();

    std::string instr = str.substr( 1 );

    int input_length = instr.length();

    if( input_length % 4 != 0 ) {
        return str;
    }

    int output_length = input_length / 4 * 3;
    const char *data = instr.c_str();

    if( data[input_length - 1] == '=' ) {
        output_length--;
    }
    if( data[input_length - 2] == '=' ) {
        output_length--;
    }

    std::string decoded_data( output_length, '\0' );

    for( int i = 0, j = 0; i < input_length; ) {

        unsigned sextet_a = data[i] == '=' ? 0 & i++ : base64_decoding_table[( unsigned char )data[i++]];
        unsigned sextet_b = data[i] == '=' ? 0 & i++ : base64_decoding_table[( unsigned char )data[i++]];
        unsigned sextet_c = data[i] == '=' ? 0 & i++ : base64_decoding_table[( unsigned char )data[i++]];
        unsigned sextet_d = data[i] == '=' ? 0 & i++ : base64_decoding_table[( unsigned char )data[i++]];

        unsigned triple = ( sextet_a << 3 * 6 )
                          + ( sextet_b << 2 * 6 )
                          + ( sextet_c << 1 * 6 )
                          + ( sextet_d << 0 * 6 );

        if( j < output_length ) {
            decoded_data[j++] = ( triple >> 2 * 8 ) & 0xFF;
        }
        if( j < output_length ) {
            decoded_data[j++] = ( triple >> 1 * 8 ) & 0xFF;
        }
        if( j < output_length ) {
            decoded_data[j++] = ( triple >> 0 * 8 ) & 0xFF;
        }
    }

    return decoded_data;
}

static void strip_trailing_nulls( std::wstring &str )
{
    while( !str.empty() && str.back() == '\0' ) {
        str.pop_back();
    }
}

static void strip_trailing_nulls( std::string &str )
{
    while( !str.empty() && str.back() == '\0' ) {
        str.pop_back();
    }
}

std::wstring utf8_to_wstr( const std::string &str )
{
#if defined(_WIN32) || defined(WINDOWS)
    int sz = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, NULL, 0 ) + 1;
    std::wstring wstr( sz, '\0' );
    MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, &wstr[0], sz );
    strip_trailing_nulls( wstr );
    return wstr;
#else
    std::size_t sz = std::mbstowcs( NULL, str.c_str(), 0 ) + 1;
    std::wstring wstr( sz, '\0' );
    std::mbstowcs( &wstr[0], str.c_str(), sz );
    strip_trailing_nulls( wstr );
    return wstr;
#endif
}

std::string wstr_to_utf8( const std::wstring &wstr )
{
#if defined(_WIN32) || defined(WINDOWS)
    int sz = WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL );
    std::string str( sz, '\0' );
    WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), -1, &str[0], sz, NULL, NULL );
    strip_trailing_nulls( str );
    return str;
#else
    std::size_t sz = std::wcstombs( NULL, wstr.c_str(), 0 ) + 1;
    std::string str( sz, '\0' );
    std::wcstombs( &str[0], wstr.c_str(), sz );
    strip_trailing_nulls( str );
    return str;
#endif
}

std::string native_to_utf8( const std::string &str )
{
    if( get_options().has_option( "ENCODING_CONV" ) && !get_option<bool>( "ENCODING_CONV" ) ) {
        return str;
    }
#if defined(_WIN32) || defined(WINDOWS)
    // native encoded string --> Unicode sequence --> UTF-8 string
    int unicode_size = MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, NULL, 0 ) + 1;
    std::wstring unicode( unicode_size, '\0' );
    MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, &unicode[0], unicode_size );
    int utf8_size = WideCharToMultiByte( CP_UTF8, 0, &unicode[0], -1, NULL, 0, NULL, 0 ) + 1;
    std::string result( utf8_size, '\0' );
    WideCharToMultiByte( CP_UTF8, 0, &unicode[0], -1, &result[0], utf8_size, NULL, 0 );
    strip_trailing_nulls( result );
    return result;
#else
    return str;
#endif
}

std::string utf8_to_native( const std::string &str )
{
    if( get_options().has_option( "ENCODING_CONV" ) && !get_option<bool>( "ENCODING_CONV" ) ) {
        return str;
    }
#if defined(_WIN32) || defined(WINDOWS)
    // UTF-8 string --> Unicode sequence --> native encoded string
    int unicode_size = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, NULL, 0 ) + 1;
    std::wstring unicode( unicode_size, '\0' );
    MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, &unicode[0], unicode_size );
    int native_size = WideCharToMultiByte( CP_ACP, 0, &unicode[0], -1, NULL, 0, NULL, 0 ) + 1;
    std::string result( native_size, '\0' );
    WideCharToMultiByte( CP_ACP, 0, &unicode[0], -1, &result[0], native_size, NULL, 0 );
    strip_trailing_nulls( result );
    return result;
#else
    return str;
#endif
}

int center_text_pos( const char *text, int start_pos, int end_pos )
{
    int full_screen = end_pos - start_pos + 1;
    int str_len = utf8_width( text );
    int position = ( full_screen - str_len ) / 2;

    if( position <= 0 ) {
        return start_pos;
    }

    return start_pos + position;
}

int center_text_pos( const std::string &text, int start_pos, int end_pos )
{
    return center_text_pos( text.c_str(), start_pos, end_pos );
}

int center_text_pos( const utf8_wrapper &text, int start_pos, int end_pos )
{
    int full_screen = end_pos - start_pos + 1;
    int str_len = text.display_width();
    int position = ( full_screen - str_len ) / 2;

    if( position <= 0 ) {
        return start_pos;
    }

    return start_pos + position;
}
