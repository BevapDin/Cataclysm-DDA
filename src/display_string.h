#pragma once

#include "utf8_wrapper.h"

class display_string_accessor;

/**
 * Represents the final displayable text:
 *
 * - It's UTF8 encoded and always valid.
 * - Its content can not be accessed directly, only the display driver should
 *   access it.
 * - It is not interpreted by display driver (it does not contain color tags,
 *   '%' characters are printed directly).
 */
class display_string {
    private:
        friend display_string_accessor;

        utf8_wrapper data_;

    public:
        display_string() = default;
        display_string( const display_string & ) = default;
        display_string( display_string && ) = default;

        /**
         * @throws std::exception if the input is not valid UTF8.
         */
        display_string( std::string d );
        display_string( utf8_wrapper d ) : data_( std::move( d ) ) { }

        display_string &operator=( const display_string & ) = default;
        display_string &operator=( display_string && ) = default;

        friend display_string operator+( const display_string &lhs, const display_string &rhs ) {
            // concatenation of two valid UTF8 sequences is always a valid sequence
            return display_string( lhs.data_ + rhs.data_ );
        }
};
