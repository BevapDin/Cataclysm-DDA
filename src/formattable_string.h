#pragma once

#include "string_formatter.h"

#include <string>
extern "C" {
extern char *gettext(const char*);
}

template<typename ...Args>
class formattable_string {
    private:
        std::string data_;

    public:
        formattable_string() = default;
        formattable_string( std::string d ) : data_( std::move( d ) ) {
            cata::string_formatter( d ).test<Args...>();
        }

        template<typename ...Args2>
        friend std::string string_format( const formattable_string<Args...> &d, Args2 &&...args ) {
            static_assert( std::is_same<std::tuple<Args...>, std::tuple<Args2...>>::value, "");
            return ::string_format( d.data_, std::forward<Args2>( args )... );
        }

        bool operator==(const formattable_string<Args...> &d ) const {
            return data_ == d.data_;
        }

        friend formattable_string<Args...> _( const formattable_string<Args...> &d ) {
            return formattable_string( gettext( d.data_.c_str() ) );
        }

        template<typename StreamType>
        void deserialize( StreamType &stream ) {
            *this = stream.get_string();
        }
};
