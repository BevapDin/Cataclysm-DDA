#pragma once
#ifndef OPTIONS_H
#define OPTIONS_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <memory>

#include "translations.h"
#include "optional.h"
#include "debug.h"

class JsonIn;
class JsonOut;
class JsonValue;

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
    /** Hide this option always, it should not be changed by user directly through UI. **/
    COPT_ALWAYS_HIDE
};

class id_and_option : public std::pair<std::string, translation>
{
    public:
        id_and_option( const std::string &first, const std::string &second )
            : std::pair<std::string, translation>( first, to_translation( second ) ) {
        }
        id_and_option( const std::string &first, const translation &second )
            : std::pair<std::string, translation>( first, second ) {
        }
};

/**
 * The option system uses the following classes:
 *
 * `TypedOptionMetadata`: these stores UI related data (e.g. name, tooltip). It does not store
 * the actual option value, but it has functions to interact with the option value (those usually
 * get called from the UI).
 * This is a class template, it must be used in combination with an option value of the matching type.
 * (e.g. a `TypedOptionMetadata<int>` must be used with an `int` option value).
 *
 * `TypedOptionValue` is the combination of option value and option metadata. It is also a class template
 * and the actual type of the option value must be given to it.
 * This class is derived from `OptionValueBase` and provides the virtual functions declared in that base.
 * References to `OptionValueBase` are given out to the world.
 *
 * This allows to have a type safe interaction between option value and metadata, but it also allows
 * a type-agnostic view from the outside (the remaining game code).
 *
 */
class OptionMetadataBase
{
    private:
        std::string sName;
        std::string sPage;
        // The *untranslated* displayed option name ( short string ).
        std::string sMenuText;
        // The *untranslated* displayed option tool tip ( longer string ).
        std::string sTooltip;
        mutable std::string sPrerequisite;
        mutable std::vector<std::string> sPrerequisiteAllowedValues;

        copt_hide_t hide;

    protected:
        OptionMetadataBase( const std::string &sNameIn, const std::string &sPageIn,
                            const std::string &sMenuTextIn, const std::string &sTooltipIn, copt_hide_t opt_hide );

    public:
        virtual ~OptionMetadataBase();

        /**
         * Option should be hidden in current build.
         * @return true if option should be hidden, false if not.
         */
        bool is_hidden() const;

        std::string getName() const;
        std::string getPage() const;
        /// The translated displayed option name.
        std::string getMenuText() const;
        /// The translated displayed option tool tip.
        std::string getTooltip() const;

        void setPrerequisites( const std::string &sOption,
                               const std::vector<std::string> &sAllowedValues ) const;
        void setPrerequisite( const std::string &sOption, const std::string &sAllowedValue = "true" ) const;
        std::string getPrerequisite() const;
        bool hasPrerequisite() const;
        bool checkPrerequisite() const;

        virtual std::string getDefaultText() const = 0;
        virtual std::string getDefaultTextUntranslated() const = 0;
};

template<typename T>
class TypedOptionMetadata : public OptionMetadataBase
{
    public:
        TypedOptionMetadata( const std::string &sNameIn, const std::string &sPageIn,
                             const std::string &sMenuTextIn, const std::string &sTooltipIn, copt_hide_t opt_hide );

        ~TypedOptionMetadata() override;

        void setNext( T &value ) const;
        void setPrev( T &value ) const;
        void setInteractive( T &value ) const;

        void setValueLegacy( T &value, const std::string &new_value ) const;
        void serialize( JsonOut &json, const T &value ) const;
        void deserializeValue( T &value, const JsonValue &new_value ) const;

        /// The translated currently selected option value.
        std::string getValueName( const T &value ) const;
        // @TODO get rid of this
        std::string getValue( const T &value ) const;
};

template<typename T>
class TypedOptionMetadataWithDefault : public TypedOptionMetadata<T>
{
    private:
        T default_value_;

    public:
        TypedOptionMetadataWithDefault( const std::string &sNameIn, const std::string &sPageIn,
                                        const std::string &sMenuTextIn, const std::string &sTooltipIn, copt_hide_t opt_hide,
                                        const T &defaultIn );

        ~TypedOptionMetadataWithDefault() override;

        std::string getDefaultText() const override;
        std::string getDefaultTextUntranslated() const override;
};

class OptionValueBase
{
    protected:
        OptionValueBase();

    public:
        virtual ~OptionValueBase();

        /// @TODO: document
        virtual const OptionMetadataBase &metadata() const = 0;

        void setPrerequisites( const std::string &sOption,
                               const std::vector<std::string> &sAllowedValues ) {
            metadata().setPrerequisites( sOption, sAllowedValues );
        }
        void setPrerequisite( const std::string &sOption, const std::string &sAllowedValue = "true" ) {
            metadata().setPrerequisite( sOption, sAllowedValue );
        }
        std::string getPrerequisite() const {
            return metadata().getPrerequisite();
        }
        bool hasPrerequisite() const {
            return metadata().hasPrerequisite();
        }
        bool checkPrerequisite() const {
            return metadata().checkPrerequisite();
        }
        /**
         * Set the value of this option to the next / previous / any available value.
         * This functions is invoked from the UI. The definition of "next", "previous"
         * and "any" are up to the option itself.
         * Only recommendation is this:
         * - The user should be able to cycle through all available option values with
         *   the "prev" and "next" functions.
         * - Switching to "next" and "previous" should be inverse operations.
         * - "any" should either be equivalent to "next" (e.g. for boolean options),
         *   or query the user directly.
         */
        ///@{
        virtual void setNext() = 0;
        virtual void setPrev() = 0;
        virtual void setInteractive() = 0;
        ///@}
        /**
         * @return The current value of this option, formatted to be displayed to the user.
         * The returned string should already be translated (if necessary at all).
         */
        virtual std::string getValueName() const = 0;
        // @TODO get rid of this
        virtual std::string getValue() const = 0;
        // @TODO document this
        virtual std::string getDefaultText() const = 0;
        // @TODO document this
        virtual std::string getDefaultTextUntranslated() const = 0;
        /**
         * Legacy function, don't use in new code.
         *
         * This sets the value of this option to the value given as string.
         * If necessary this must convert the value to the internal type.
         *
         * @throws std::exception If the new value is not valid for this option, of if
         * it can not be converted.
         */
        virtual void setValueLegacy( const std::string &new_value ) = 0;
        /**
         * Assigns the value taken from the other option value.
         * Precondition: the value @p new_value must be of the same type.
         */
        virtual void setValue( const OptionValueBase &new_value ) = 0;
        /// Usual serialization function.
        virtual void serialize( JsonOut &json ) const = 0;
        /**
         * Assign the value from JSON. Note that this is given only the *value* that was
         * serialized, not the whole object that @ref serialize prints.
         * (E.g. @p new_value points to a single int value in JSON.)
         */
        virtual void deserializeValue( const JsonValue &new_value ) = 0;
        /**
         * Compares the values of this and the the other instance.
         * The function *ignores* the metadata. If the underlying values are of
         * different types, it will return non-equal.
         */
        virtual bool operator==( const OptionValueBase &other ) const = 0;
        bool operator!=( const OptionValueBase &other ) const {
            return !operator==( other );
        }

        /// @TODO: document
        virtual std::unique_ptr<OptionValueBase> clone() const = 0;

        template<typename T>
        void setValueDirect( const T &value );
};
/**
 * This class is a intermediate class that allows type safe access to the value.
 * One can cast a @ref OptionValueBase reference to an `TypedOptionValueBase<int>` to access the contained
 * `int` value.
 * Note that we can't use @ref TypedOptionValue for this directly as it has another template parameter
 * that is part of the type, and we may not know this parameter when we only need to access the value.
 */
template<typename ValueType>
class TypedOptionValueBase : public OptionValueBase
{
        static_assert( std::is_same<ValueType, typename std::decay<ValueType>::type>::value,
                       "Template parameter must be an object type, no reference or similar" );
    protected:
        TypedOptionValueBase() = default;

    public:
        ~TypedOptionValueBase() override = default;

        /// Accesses the value contained within this option.
        virtual const ValueType &value() const = 0;
        virtual ValueType &value() = 0;

        bool operator==( const OptionValueBase &other ) const override {
            const auto typed = dynamic_cast<const TypedOptionValueBase<ValueType>*>( &other );
            return typed && value() == typed->value();
        }

        void setValue( const OptionValueBase &new_value ) override {
            const auto typed = dynamic_cast<const TypedOptionValueBase<ValueType>*>( &new_value );
            assert( typed );
            value() = typed->value();
        }
};

template<typename T>
void OptionValueBase::setValueDirect( const T &value )
{
    const auto typed = dynamic_cast<TypedOptionValueBase<T>*>( this );;
    assert( typed );
    typed->value() = value;
}

class OptionValue
{
    private:
        std::unique_ptr<OptionValueBase> pointer_;

    public:
        OptionValue();
        OptionValue( std::unique_ptr<OptionValueBase> pointer );
        OptionValue( const OptionValue &other );

        ~OptionValue();

        OptionValue &operator=( const OptionValue &other );

        OptionValueBase &operator*() {
            return *pointer_;
        }
        const OptionValueBase &operator*() const {
            return *pointer_;
        }
        OptionValueBase *operator->() {
            return pointer_.get();
        }
        const OptionValueBase *operator->() const {
            return pointer_.get();
        }

        template<typename T>
        void setValueDirect( const T &value ) {
            pointer_->setValueDirect( value );
        }
        void setValueLegacy( const std::string &value ) {
            pointer_->setValueLegacy( value );
        }
        void serialize( JsonOut &json ) const {
            pointer_->serialize( json );
        }
};

using options_container = std::unordered_map<std::string, OptionValue>;

class options_manager
{
    private:
        static std::vector<id_and_option> build_tilesets_list();
        static std::vector<id_and_option> build_soundpacks_list();
        static std::vector<id_and_option> load_tilesets_from(
            const std::string &path );
        static std::vector<id_and_option> load_soundpack_from(
            const std::string &path );

        bool load_legacy();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        void update_global_locale();

        std::map<std::string, std::string> post_json_verify;

        std::map<std::string, std::pair<std::string, std::map<std::string, std::string> > > mMigrateOption;

        friend options_manager &get_options();
        options_manager();

        template<typename T, typename ...Args>
        void addOption( Args &&... args );

    public:
        void init();
        void add_options_general();
        void add_options_interface();
        void add_options_graphics();
        void add_options_debug();
        void add_options_world_default();
        void add_options_android();
        void load();
        bool save();
        std::string show( bool ingame = false, bool world_options_only = false );

        void add_value( const std::string &lvar, const std::string &lval,
                        const translation &lvalname );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        std::string migrateOptionName( const std::string &name ) const;
        std::string migrateOptionValue( const std::string &name, const std::string &val ) const;

        /**
         * Returns a copy of the options in the "world default" page. The options have their
         * current value, which acts as the default for new worlds.
         */
        options_container get_world_defaults() const;

        void set_world_options( options_container *options );

        /** Check if an option exists? */
        bool has_option( const std::string &name ) const;

        OptionValueBase &get_option( const std::string &name );

        //add hidden external option with value
        void add_external( const std::string &sNameIn, const std::string &sPageIn, const std::string &sType,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn );

        //add string select option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  // first is option value, second is display name of that value
                  const std::vector<id_and_option> &sItemsIn, std::string sDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add string input option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  const std::string &sDefaultIn, int iMaxLengthIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add bool option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

        //add int option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  int iMinIn, int iMaxIn, int iDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%i" );

        //add int map option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  const std::vector< std::tuple<int, std::string> > &mIntValuesIn,
                  int iInitialIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE,
                  bool verbose = false );

        //add float option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  float fMinIn, float fMaxIn,
                  float fDefaultIn, float fStepIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%.2f" );

    private:
        options_container options;
        cata::optional<options_container *> world_options;

        /**
         * A page (or tab) to be displayed in the options UI.
         * It contains a @ref id that is used to detect what options should go into this
         * page (see @ref OptionMetadataBase::getPage).
         * It also has a name that will be translated and displayed.
         * And it has items, each item is either nothing (will be represented as empty line
         * in the UI) or the name of an option.
         */
        class Page {
        public:
            std::string id_;
            std::string name_;

            std::vector<cata::optional<std::string>> items_;

            void removeDoubleEmptyLines();

            Page( const std::string &id, const std::string &name ) : id_(id), name_(name) { }
        };

        Page general_page_;
        Page interface_page_;
        Page graphics_page_;
        Page debug_page_;
        Page world_default_page_;
        Page android_page_;

        std::vector<std::reference_wrapper<Page>> pages_;
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
inline T get_option( const std::string &name )
{
    const OptionValueBase &opt = get_options().get_option( name );
    const auto typed = dynamic_cast<const TypedOptionValueBase<T>*>( &opt );
    if( !typed ) {
        debugmsg( "Tried to get value of type %s from option %s!", typeid( T ).name(), name );
        return T();
    }
    return typed->value();
}

#endif
