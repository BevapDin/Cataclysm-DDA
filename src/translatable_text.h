#pragma once
#ifndef TRANSLATABLE_TEXT_H
#define TRANSLATABLE_TEXT_H

#include <string>

class JsonIn;

/**
 * Represents a text that can be translated at run time.
 *
 * Direct access to its contents should be avoided, you should most likely
 * operate on the translated string instead.
 *
 * The only property you may get is @ref empty (because an empty string is
 * always translated into an empty string and the same for non-empty strings).
 */
class translatable_text
{
    private:
        /// untranslated text
        std::string text_;

    public:
        /**
         * Creates an object that yields an empty string when being translated.
         */
        translatable_text() = default;
        /**
         * Creates an object that uses the given string to be translated.
         * The input should have been marked for translation (e.g. with
         * @ref translate_marker).
         */
        explicit translatable_text( const std::string &text ) : text_( text ) { }
        explicit translatable_text( const char *const text ) : text_( text ) { }

        std::string translated() const {
            return translated_cstring();
        }
        const char *translated_cstring() const;
        // intentionally named long and convoluted and unusual as it should not be used
        const std::string &get_untranslated_text() const {
            return text_;
        }

        // Only for legacy code that does not support this class directly.
        //@todo remove this
        operator std::string() const {
            return translated();
        }

        bool empty() const {
            return text_.empty();
        }
        bool operator==( const translatable_text &rhs ) const {
            return text_ == rhs.text_;
        }

        void deserialize( JsonIn &jsin );
};

#endif
