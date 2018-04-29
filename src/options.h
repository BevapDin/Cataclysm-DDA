#pragma once
#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>
#include <typeinfo>
#include <utility>
#include <unordered_map>
#include <vector>

#include "poly_pimpl.h"

class JsonIn;
class JsonOut;

class options_manager
{
    private:
        static std::vector<std::pair<std::string, std::string>> build_tilesets_list();
        static std::vector<std::pair<std::string, std::string>> build_soundpacks_list();

        bool load_legacy();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        std::map<std::string, std::string> post_json_verify;

        friend options_manager &get_options();
        options_manager();

        template<typename T>
        void add( const T &opt );

    public:
        enum copt_hide_t {
            /** Don't hide this option */
            COPT_NO_HIDE,
            /** Hide this option in SDL build */
            COPT_SDL_HIDE,
            /** Show this option in SDL builds only */
            COPT_CURSES_HIDE,
            /** Hide this option in non-Windows Curses builds */
            COPT_POSIX_CURSES_HIDE,
            /** Hide this option in builds without sound support */
            COPT_NO_SOUND_HIDE,
            /** Hide this option always, it is set as a mod. **/
            COPT_ALWAYS_HIDE
        };

        static bool is_hidden( const copt_hide_t hide );

        class cOpt_base
        {
                friend class options_manager;
            protected:
                cOpt_base( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h ) : sName( n ), sPage( p ), sMenuText( m ), sTooltip( t ), hide( h ) { }

            public:
                virtual ~cOpt_base() = default;

                int getSortPos() const;

                /**
                 * Option values are saved as strings in JSON, this function
                 * returns the string that is to be stored. It should
                 * *not* be translated!
                 */
                virtual std::string get_legacy_value() const = 0;
                /**
                 * Reverse of @ref get_legacy_value. Converts the value into
                 * the internal type and stores it as the option value.
                 */
                virtual void set_from_legacy_value( const std::string &v ) = 0;

                /// The *id* of this option.
                std::string getName() const;
                /// The id of the page this option belongs onto.
                std::string getPage() const;
                /// The translated displayed option name.
                std::string getMenuText() const;
                /// The translated displayed option tool tip.
                std::string getTooltip() const;
                virtual std::string getType() const = 0;

                /// The translated currently selected option value.
                virtual std::string getValueName() const = 0;
                virtual std::string getDefaultText( const bool bTranslated = true ) const = 0;

                virtual void setNext() = 0;
                virtual void setPrev() = 0;
                virtual void setInteractive() = 0;

                template<typename T>
                T value_as() const;

                virtual bool operator==( const cOpt_base & ) const = 0;
                bool operator!=( const cOpt_base &rhs ) const {
                    return !operator==( rhs );
                }

                void setPrerequisite( const std::string &sOption );
                std::string getPrerequisite() const;
                bool hasPrerequisite() const;

                void serialize( JsonOut &jsout ) const;

                // as required by poly_pimpl
                virtual cOpt_base *clone() const = 0;

            protected:
                // Wrapper for debugmsg so we don't have to include "debug.h" in this header.
                void report_type_error( const std::string &expected ) const;

            private:
                // The *id* of this option.
                std::string sName;
                // The id of the page that this option belongs onto.
                std::string sPage;
                // The *untranslated* displayed option name (short string).
                std::string sMenuText;
                // The *untranslated* displayed option tool tip (longer string).
                std::string sTooltip;
                std::string sPrerequisite;
                copt_hide_t hide = COPT_NO_HIDE;
                int iSortPos;
        };
        /**
         * Basic type safe option container. Provides a function to get the option
         * value in the type that is used in the code.
         * All option classes should derive from this instead of from cOpt_base.
         */
        template<typename T>
        class typed_option : public cOpt_base
        {
            protected:
                template<class... Args>
                typed_option( Args &&... args ) : cOpt_base( std::forward<Args>( args )... ) { }

            public:
                ~typed_option() override = default;
                virtual T value() const = 0;
                virtual void value( const T &v ) = 0;

                std::string getType() const override {
                    return typeid(T).name();
                }
        };

        class bool_option : public typed_option<bool>
        {
            private:
                bool value_;
                bool default_value_;

            public:
                bool_option( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h, const bool def ) : typed_option<bool>( n, p, m, t, h ), value_( def ), default_value_( def ) { }
                ~bool_option() override = default;

                bool value() const override {
                    return value_;
                }
                void value( const bool &v ) override {
                    value_ = v;
                }

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;
                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                void setNext() override {
                    value_ = !value_;
                }
                void setPrev() override {
                    value_ = !value_;
                }
                void setInteractive() override {
                    // Don't need interactivity here, there is only one thing the user
                    // can change the value to.
                    value_ = !value_;
                }

                bool operator==( const cOpt_base &rhs ) const override {
                    const auto o = dynamic_cast<const bool_option*>( &rhs );
                    return o && value_ == o->value_;
                }

                cOpt_base *clone() const override {
                    return new bool_option( *this );
                }
        };

        class float_option : public typed_option<float>
        {
            private:
                float value_;
                float default_value_;
                float min_value_;
                float max_value_;
                float step_;

            public:
                float_option( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h, const float d, const float mi, const float ma, const float s ) : typed_option<float>( n, p, m, t, h ), value_( d), default_value_(d), min_value_(mi), max_value_(ma), step_(s) { }
                ~float_option() override = default;

                float value() const override {
                    return value_;
                }
                void value( const float &v ) override {
                    //@todo maybe throw instead of clamping? Or use default value?
                    value_ = std::max( min_value_, std::min( max_value_, v ) );
                }

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;
                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                void setNext() override;
                void setPrev() override;
                void setInteractive() override;

                bool operator==( const cOpt_base &rhs ) const override {
                    const auto o = dynamic_cast<const float_option*>( &rhs );
                    return o && value_ == o->value_;
                }

                cOpt_base *clone() const override {
                    return new float_option(*this);
                }
        };

        class string_input_option : public typed_option<std::string>
        {
            private:
                std::string value_;
                std::string default_value_;
                int iMaxLength;

            public:
                string_input_option( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h, const std::string &d, const int l ) : typed_option<std::string>( n, p, m, t, h ), value_( d ), default_value_( d ), iMaxLength( l ) { }
                ~string_input_option() override = default;

                std::string value() const override {
                    return value_;
                }
                void value( const std::string &v ) override {
                    //@todo maybe throw instead of clamping? Or use default value?
                    value_ = v.substr( 0, iMaxLength );
                }

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;
                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                void setNext() override {
                    setInteractive();
                }
                void setPrev() override {
                    setInteractive();
                }
                void setInteractive() override;

                bool operator==( const cOpt_base &rhs ) const override {
                    const auto o = dynamic_cast<const string_input_option*>( &rhs );
                    return o && value_ == o->value_;
                }

                cOpt_base *clone() const override {
                    return new string_input_option(*this);
                }
        };

        class display_device_option : public typed_option<int>
        {
            private:
                // As long as the @ref displays_ is not empty, this is always
                // a valid index into it.
                int value_;
                std::map<int, std::string> displays_;

            public:
                display_device_option();
                ~display_device_option() override = default;

                int value() const override {
                    return value_;
                }
                void value( const int &v ) override;

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;
                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                void setNext() override;
                void setPrev() override;
                void setInteractive() override;

                void set_displays( const decltype( displays_ ) & displays );

                bool operator==( const cOpt_base &rhs ) const override {
                    const auto o = dynamic_cast<const display_device_option*>( &rhs );
                    return o && value_ == o->value_;
                }

                cOpt_base *clone() const override {
                    return new display_device_option(*this);
                }
        };

        class int_option : public typed_option<int>
        {
            private:
                int value_;
                int default_value_;
                int min_value_;
                int max_value_;
                std::string format_;

            public:
                int_option( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h, const int d, const int mi, const int ma, const std::string &f = "%i" ) : typed_option<int>( n, p, m, t, h ), value_( d), default_value_(d), min_value_(mi), max_value_(ma), format_(f) { }
                ~int_option() override = default;

                int value() const override {
                    return value_;
                }
                void value( const int &v ) override {
                    //@todo maybe throw instead of clamping? Or use default value?
                    value_ = std::max( min_value_, std::min( max_value_, v ) );
                }

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;
                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                void setNext() override;
                void setPrev() override;
                void setInteractive() override;

                bool operator==( const cOpt_base &rhs ) const override {
                    const auto o = dynamic_cast<const int_option*>( &rhs );
                    return o && value_ == o->value_;
                }

                cOpt_base *clone() const override {
                    return new int_option(*this);
                }
        };

        class string_map_option : public typed_option<std::string>
        {
                friend class options_manager;
            public:
                string_map_option( const std::string &n, const std::string &p, const std::string &m, const std::string &t, const copt_hide_t h ) : typed_option<std::string>( n, p, m, t, h ) { }
                ~string_map_option() override = default;

                std::string get_legacy_value() const override;
                void set_from_legacy_value( const std::string &v ) override;

                std::string value() const override {
                    return value_;
                }
                void value( const std::string &v ) override;

                std::string getValueName() const override;
                std::string getDefaultText( const bool bTranslated = true ) const override;

                int getItemPos( const std::string sSearch ) const;
                std::vector<std::pair<std::string, std::string>> getItems() const;
                void add_value( const std::string &lval, const std::string &lvalname );

                void setNext() override;
                void setPrev() override;
                void setInteractive() override;

                bool operator==( const cOpt_base &rhs ) const override;

                cOpt_base *clone() const override {
                    return new string_map_option(*this);
                }

            private:
                std::string value_;
                // first is internal value, second is untranslated text
                std::vector<std::pair<std::string, std::string>> vItems;
                std::string default_value_;
        };

        typedef std::unordered_map<std::string, poly_pimpl<cOpt_base>> options_container;

        void init();
        void load();
        bool save();
        std::string show( const bool ingame = false, const bool world_options_only = false );

        void add_value( const std::string &myoption, const std::string &myval,
                        const std::string &myvaltxt = "" );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        /**
         * Returns a copy of the options in the "world default" page. The options have their
         * current value, which acts as the default for new worlds.
         */
        options_container get_world_defaults() const;
        std::vector<std::string> getWorldOptPageItems() const;

        options_container *world_options;

        /** Check if an option exists? */
        bool has_option( const std::string &name ) const;

        cOpt_base &get_option( const std::string &name );

        void add_external( const std::string sNameIn, const std::string sPageIn, const std::string sType,
                           const std::string sMenuTextIn, const std::string sTooltipIn );

        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  // first is option value, second is display name of that value
                  std::vector<std::pair<std::string, std::string>> sItemsIn, std::string sDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

    private:
        options_container options;
        // first is page id, second is untranslated page name
        std::vector<std::pair<std::string, std::string>> vPages;
        std::map<int, std::vector<std::string>> mPageItems;
        int iWorldOptPage;
};

bool use_narrow_sidebar(); // short-circuits to on if terminal is too small

/** A mapping(string:string) that stores all tileset values.
 * Firsts string is tileset NAME from config.
 * Second string is directory that contain tileset.
 */
extern std::map<std::string, std::string> TILESETS;
/** A mapping(string:string) that stores all soundpack values.
 * Firsts string is soundpack NAME from config.
 * Second string is directory that contains soundpack.
 */
extern std::map<std::string, std::string> SOUNDPACKS;

options_manager &get_options();

template<typename T>
inline T options_manager::cOpt_base::value_as() const
{
    const auto o = dynamic_cast<const options_manager::typed_option<T> *>( this );
    if( o ) {
        return o->value();
    }
    report_type_error( typeid( T ).name() );
    return T();
}

template<typename T>
inline T get_option( const std::string &name )
{
    return get_options().get_option( name ).value_as<T>();
}

#endif
