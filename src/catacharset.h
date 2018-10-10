#pragma once
#ifndef CATACHARSET_H
#define CATACHARSET_H

#include <stdint.h>
#include <string>
#include <array>
#define ANY_LENGTH 5
#define UNKNOWN_UNICODE 0xFFFD

class utf8_wrapper;

// get a Unicode character from a utf8 string
uint32_t UTF8_getch( const char **src, int *srclen );
// from wcwidth.c, return "cell" width of a Unicode char
int mk_wcwidth( uint32_t ucs );
// convert cursorx value to byte position
int cursorx_to_position( const char *line, int cursorx, int *prevppos = NULL, int maxlen = -1 );
int utf8_width( const char *s, const bool ignore_tags = false );
int utf8_width( const std::string &str, const bool ignore_tags = false );
int utf8_width( const utf8_wrapper &str, const bool ignore_tags = false );

/**
 * Center text inside whole line.
 * @param text to be centered.
 * @param start_pos printable position on line.
 * @param end_pos printable position on line.
 * @return First char position of centered text or start_pos if text is too big.
*/
int center_text_pos( const char *text, int start_pos, int end_pos );
int center_text_pos( const std::string &text, int start_pos, int end_pos );
int center_text_pos( const utf8_wrapper &text, int start_pos, int end_pos );
std::string utf32_to_utf8( uint32_t ch );
std::string utf8_truncate( std::string s, size_t length );

std::string base64_encode( std::string str );
std::string base64_decode( std::string str );

std::wstring utf8_to_wstr( const std::string &str );
std::string wstr_to_utf8( const std::wstring &wstr );

std::string native_to_utf8( const std::string &str );
std::string utf8_to_native( const std::string &str );

#endif
