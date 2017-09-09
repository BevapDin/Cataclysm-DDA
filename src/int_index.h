#pragma once
#ifndef INT_INDEX_H
#define INT_INDEX_H

#include <limits>

/**
 * Wrapper for some kind of index. The index is stored as `int`.
 * It behaves like a standard `int`, but with limited functionality:
 * - comparison to other `int_index` objects with the same `tag_type`.
 * - increment, decrement (only by one).
 */
template<typename tag_type>
class int_index
{
    private:
        int value_;

    public:
        /// Create an (hopefully, but not guaranteed) invalid index.
        int_index() : value_( std::numeric_limits<decltype( value_ )>::min() ) { }
        explicit int_index( const decltype( value_ ) v ) : value_( v ) { }

        int value() const {
            return value_;
        }

        bool operator==( const int_index<tag_type> &rhs ) const {
            return value_ == rhs.value_;
        }
        bool operator!=( const int_index<tag_type> &rhs ) const {
            return !operator==( rhs );
        }
        bool operator<( const int_index<tag_type> &rhs ) const {
            return value_ < rhs.value_;
        }
        bool operator>( const int_index<tag_type> &rhs ) const {
            return value_ > rhs.value_;
        }

        int_index<tag_type> &operator++() {
            ++value_;
            return *this;
        }
        int_index<tag_type> operator++( int ) {
            const auto result = *this;
            ++value_;
            return result;
        }

        int_index<tag_type> &operator--() {
            --value_;
            return *this;
        }
        int_index<tag_type> operator--( int ) {
            const auto result = *this;
            --value_;
            return result;
        }
};

#endif
