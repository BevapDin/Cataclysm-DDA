#include "options.h"
#include "game.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include "filesystem.h"
#include "string_formatter.h"
#include "cursesdef.h"
#include "path_info.h"
#include "mapsharing.h"
#include "json.h"
#include "sounds.h"
#include "cata_utility.h"
#include "input.h"
#include "worldfactory.h"
#include "catacharset.h"
#include "game_constants.h"
#include "string_input_popup.h"

#ifdef TILES
#include "cata_tiles.h"
#endif // TILES

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "cursesport.h"
#endif

#include <stdlib.h>
#include <string>
#include <locale>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <memory>

bool trigdist;
bool use_tiles;
bool log_from_top;
int message_ttl;
bool fov_3d;
bool tile_iso;

#ifdef TILES
extern std::unique_ptr<cata_tiles> tilecontext;
#endif // TILES

std::map<std::string, std::string> TILESETS; // All found tilesets: <name, tileset_dir>
std::map<std::string, std::string> SOUNDPACKS; // All found soundpacks: <name, soundpack_dir>
std::map<std::string, int> mOptionsSort;

options_manager &get_options()
{
    static options_manager single_instance;
    return single_instance;
}

options_manager::options_manager()
{
    enable_json("DEFAULT_REGION");
    // to allow class based init_data functions to add values to a 'string' type option, add:
    //   enable_json("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON");
    // then, in the my_class::load_json (or post-json setup) method:
    //   get_options().add_value("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON", "thisvalue");
}

static const std::string blank_value( 1, 001 ); // because "" might be valid

void options_manager::enable_json(const std::string &lvar)
{
    post_json_verify[ lvar ] = blank_value;
}

options_manager::display_device_option::display_device_option() : typed_option<int>( "DISPLAY", "graphics",
            translate_marker( "Display" ),
            translate_marker( "Sets which video display will be used to show the game.  Requires restart." ),
            options_manager::COPT_CURSES_HIDE ),
            value_( 0 )
{
}

options_manager::string_map_option::string_map_option( const std::string &n, const std::string &p,
        const std::string &m, const std::string &t, const copt_hide_t h, const std::string &d,
        const std::vector<std::pair<std::string, std::string>> &i ) : typed_option<std::string>( n, p, m, t,
                    h ), value_( d ), vItems( i ), default_value_( d )
{
    if( !vItems.empty() && getItemPos( d ) == -1 ) {
        debugmsg( "default value \"%s\" for option %s is not in the list of options", d, getName() );
        value_ = default_value_ = vItems[0].first;
    }
    // If the list is empty, the default is invalid anyway, but that's probably
    // fine as the option values are added later.
}

void options_manager::add_retry(const std::string &lvar, const::std::string &lval)
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() && it->second == blank_value ) {
        // initialized with impossible value: valid
        post_json_verify[ lvar ] = lval;
    }
}

void options_manager::add_value( const std::string &lvar, const std::string &lval,
                                 const std::string &lvalname )
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() ) {
        const auto ot = options.find( lvar );
        if( ot == options.end() ) {
            return;
        }
        string_map_option &opt = dynamic_cast<string_map_option&>( *ot->second );
        opt.add_value( lval, lvalname );
        // our value was saved, then set to default, so set it again.
        if ( it->second == lval ) {
            opt.set_from_legacy_value( lval );
        }
    }
}

void options_manager::string_map_option::add_value( const std::string &lval, const std::string &lvalname )
{
    for( auto eit = vItems.begin(); eit != vItems.end(); ++eit) {
        if( eit->first == lval ) { // already in
            return;
        }

    }
    vItems.emplace_back( lval, lvalname.empty() ? lval : lvalname );
}

template<typename T>
void options_manager::add( const T &opt )
{
    const auto iter = options.emplace( opt.getName(), make_poly_pimpl<T>( opt ) );
    cOpt_base &nopt = *iter.first->second;

    if( !is_hidden( nopt.hide ) ) {
        mOptionsSort[nopt.getPage()]++;
        nopt.iSortPos = mOptionsSort[nopt.getPage()] - 1;
    } else {
        nopt.iSortPos = -1;
    }
}

void options_manager::add_external( const std::string sNameIn, const std::string sPageIn,
                                    const std::string /*sType*/,
                                    const std::string sMenuTextIn, const std::string sTooltipIn )
{
    string_map_option thisOpt( sNameIn, sPageIn, sMenuTextIn, sTooltipIn, COPT_ALWAYS_HIDE );

    add( thisOpt );
}

void options_manager::cOpt_base::setPrerequisite( const std::string &sOption )
{
    if ( !get_options().has_option(sOption) ) {
        debugmsg( "setPrerequisite: unknown option %s", sOption );

    } else if( !dynamic_cast<const bool_option*>( &get_options().get_option( sOption ) ) ) {
        debugmsg( "setPrerequisite: option %s not of type bool", sOption );
    }

    sPrerequisite = sOption;
}

std::string options_manager::cOpt_base::getPrerequisite() const
{
    return sPrerequisite;
}

bool options_manager::cOpt_base::hasPrerequisite() const
{
    if ( sPrerequisite.empty() ) {
        return true;
    }

    return ::get_option<bool>(sPrerequisite);
}

bool options_manager::is_hidden( const copt_hide_t hide )
{
    switch( hide ) {
    case COPT_NO_HIDE:
        return false;

    case COPT_SDL_HIDE:
#ifdef TILES
        return true;
#else
        return false;
#endif

    case COPT_CURSES_HIDE:
#ifndef TILES // If not defined.  it's curses interface.
        return true;
#else
        return false;
#endif

    case COPT_POSIX_CURSES_HIDE:
        // Check if we on windows and using wincurses.
#if (defined TILES || defined _WIN32 || defined WINDOWS)
        return false;
#else
        return true;
#endif

    case COPT_NO_SOUND_HIDE:
#ifndef SDL_SOUND // If not defined, we have no sound support.
        return true;
#else
        return false;
#endif

    case COPT_ALWAYS_HIDE:
        return true;
    }
    // Make compiler happy, this is unreachable.
    return false;
}

int options_manager::cOpt_base::getSortPos() const
{
    return iSortPos;
}

std::string options_manager::cOpt_base::getName() const
{
    return sName;
}

std::string options_manager::cOpt_base::getPage() const
{
    return sPage;
}

std::string options_manager::cOpt_base::getMenuText() const
{
    return _( sMenuText.c_str() );
}

std::string options_manager::cOpt_base::getTooltip() const
{
    return _( sTooltip.c_str() );
}

bool options_manager::string_map_option::operator==( const cOpt_base &rhs_ ) const
{
    const string_map_option &rhs = dynamic_cast<const string_map_option&>( rhs_ );
    return value_ == rhs.value_;
}

std::string options_manager::bool_option::get_legacy_value() const
{
    return value_ ? "true" : "false";
}

std::string options_manager::float_option::get_legacy_value() const
{
    std::ostringstream ssTemp;
    ssTemp.imbue( std::locale::classic() );
    ssTemp.precision( 2 );
    ssTemp.setf( std::ios::fixed, std::ios::floatfield );
    ssTemp << value_;
    return ssTemp.str();
}

std::string options_manager::string_input_option::get_legacy_value() const
{
    return value_;
}

std::string options_manager::display_device_option::get_legacy_value() const
{
    return to_string( value_ );
}

std::string options_manager::int_option::get_legacy_value() const
{
    return to_string( value_ );
}

std::string options_manager::string_map_option::get_legacy_value() const
{
    return value_;
}

void options_manager::string_map_option::value( const std::string &v )
{
    //@todo verify
    value_ = v;
}

std::string options_manager::bool_option::getValueName() const
{
    return value_ ? _( "True" ) : _( "False" );
}

std::string options_manager::float_option::getValueName() const
{
    std::ostringstream ssTemp;
    ssTemp.imbue( std::locale() );
    ssTemp.precision( 2 );
    ssTemp.setf( std::ios::fixed, std::ios::floatfield );
    ssTemp << value_;
    return ssTemp.str();
}

std::string options_manager::string_input_option::getValueName() const
{
    return value_;
}

std::string options_manager::display_device_option::getValueName() const
{
    if( displays_.empty() ) {
        return to_string( value_ );
    }
    assert( displays_.count( value_ ) > 0 );
    return string_format( _( "%d: %s" ), value_, displays_.at( value_ ) );
}

std::string options_manager::int_option::getValueName() const
{
    return string_format( format_, value_ );
}

std::string options_manager::string_map_option::getValueName() const
{
    const auto iter = std::find_if( vItems.begin(), vItems.end(), [&]( const std::pair<std::string, std::string> &e ) {
        return e.first == value_;
    } );
    return _( iter->second.c_str() );
}

std::string options_manager::bool_option::getDefaultText( const bool /*bTranslated*/ ) const
{
    return default_value_ ? _( "Default: True" ) : _( "Default: False" );
}

std::string options_manager::float_option::getDefaultText( const bool /*bTranslated*/ ) const
{
    return string_format( _( "Default: %.2f - Min: %.2f, Max: %.2f" ), default_value_, min_value_,
                          max_value_ );
}

std::string options_manager::string_input_option::getDefaultText( const bool /*bTranslated*/ ) const
{
    return string_format( _( "Default: %s" ), default_value_ );
}

std::string options_manager::display_device_option::getDefaultText( const bool /*bTranslated*/ ) const
{
    if( displays_.empty() ) {
        return _( "Default: none" );
    }
    return string_format( _( "Default: %d: %s" ), displays_.begin()->first, displays_.begin()->second );
}

std::string options_manager::int_option::getDefaultText(const bool /*bTranslated*/) const
{
    //@todo consider the @ref int_option::format_ setting here
    return string_format(_("Default: %d - Min: %d, Max: %d"), default_value_, min_value_, max_value_);
}

std::string options_manager::string_map_option::getDefaultText(const bool bTranslated) const
{
    const auto iter = std::find_if( vItems.begin(), vItems.end(),
    [this]( const std::pair<std::string, std::string> &elem ) {
        return elem.first == default_value_;
    } );
    const std::string defaultName = iter == vItems.end() ? std::string() :
                                    ( bTranslated ? _( iter->second.c_str() ) : iter->first );
    const std::string sItems = enumerate_as_string( vItems.begin(), vItems.end(),
    [bTranslated]( const std::pair<std::string, std::string> &elem ) {
        return bTranslated ? _( elem.second.c_str() ) : elem.first;
    }, false );
    return string_format( _( "Default: %s - Values: %s" ),
                          defaultName.c_str(), sItems.c_str() );
}

int options_manager::string_map_option::getItemPos(const std::string sSearch) const
{
    for (size_t i = 0; i < vItems.size(); i++) {
        if( vItems[i].first == sSearch ) {
            return i;
        }
    }
    return -1;
}

std::vector<std::pair<std::string, std::string>> options_manager::string_map_option::getItems() const
{
    return vItems;
}

void options_manager::float_option::setNext()
{
    value_ += step_;
    if( value_ > max_value_ ) {
        value_ = min_value_;
    }
}

void options_manager::display_device_option::set_displays( const decltype( displays_ ) & displays )
{
    displays_ = displays;
    if( !displays_.empty() && displays_.count( value_ ) == 0 ) {
        value_ = displays_.begin()->first;
    }
    assert( displays_.empty() || displays_.count( value_ ) > 0 );
}

void options_manager::display_device_option::value( const int &v )
{
    //@todo validate
    value_ = v;
}

void options_manager::display_device_option::setNext()
{
    if( displays_.size() <= 1 ) {
        return;
    }
    const auto iter = displays_.find( value_ );
    assert( iter != displays_.end() );
    const auto next = std::next( iter );
    if( next == displays_.end() ) {
        value_ = displays_.begin()->first;
    } else {
        value_ = next->first;
    }
}

void options_manager::int_option::setNext()
{
    value_++;
    if (value_ > max_value_) {
        value_ = min_value_;
    }
}

void options_manager::string_map_option::setNext()
{
    int iNext = getItemPos(value_) + 1;
    if (iNext >= (int)vItems.size()) {
        iNext = 0;
    }

    value_ = vItems[iNext].first;
}

void options_manager::float_option::setPrev()
{
    value_ -= step_;
    if( value_ < min_value_ ) {
        value_ = max_value_;
    }
}

void options_manager::display_device_option::setPrev()
{
    if( displays_.size() <= 1 ) {
        return;
    }
    const auto iter = displays_.find( value_ );
    assert( iter != displays_.end() );
    if( iter == displays_.begin() ) {
        value_ = std::prev( displays_.end() )->first;
    } else {
        value_ = std::prev( iter )->first;
    }
}

void options_manager::int_option::setPrev()
{
    value_--;
    if (value_ < min_value_) {
        value_ = max_value_;
    }
}

void options_manager::string_map_option::setPrev()
{
    int iPrev = getItemPos(value_) - 1;
    if (iPrev < 0) {
        iPrev = vItems.size() - 1;
    }
}

void options_manager::float_option::setInteractive()
{
    const std::string old_opt_val = getValueName();
    const std::string opt_val = string_input_popup()
                                .title( getMenuText() )
                                .width( 10 )
                                .text( old_opt_val )
                                .only_digits( false )
                                .query_string();
    if( opt_val.empty() || opt_val == old_opt_val ) {
        return;
    }
    std::istringstream ssTemp( opt_val );
    // This uses the current locale, to allow the users
    // to use their own decimal format.
    float tmpFloat;
    ssTemp >> tmpFloat;
    if( ssTemp ) {
        //@todo maybe throw instead of clamping? Or use default value?
        value( tmpFloat );

    } else {
        popup( _( "Invalid input: not a number" ) );
    }
}

void options_manager::string_input_option::setInteractive()
{
    const std::string sMenuText = getMenuText();
    const int iMenuTextLength = sMenuText.length();
    string_input_popup()
        .width( ( iMaxLength > 80 ) ? 80 : ( ( iMaxLength < iMenuTextLength ) ? iMenuTextLength : iMaxLength + 1 ) )
        .description( sMenuText )
        .max_length( iMaxLength )
        .edit( value_ );
}

void options_manager::int_option::setInteractive()
{
    const std::string old_opt_val = getValueName();
    const std::string opt_val = string_input_popup()
                                .title( getMenuText() )
                                .width( 10 )
                                .text( old_opt_val )
                                .only_digits( true )
                                .query_string();
    if( !opt_val.empty() && opt_val != old_opt_val ) {
        // option is of type "int": string_input_popup
        // has taken care that the string contains
        // only digits, parsing is done in set_from_legacy_value
        set_from_legacy_value( opt_val );
    }
}

void options_manager::string_map_option::setInteractive()
{
    setNext();
}

void options_manager::bool_option::set_from_legacy_value( const std::string &sSetIn )
{
    value_ = sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t";
}

void options_manager::float_option::set_from_legacy_value( const std::string &sSetIn )
{
    std::istringstream ssTemp( sSetIn );
    ssTemp.imbue( std::locale::classic() );
    float tmpFloat;
    ssTemp >> tmpFloat;
    if( ssTemp ) {
        //@todo maybe throw instead of clamping? Or use default value?
        value( tmpFloat );
    } else {
        debugmsg( "invalid floating point option: %s", sSetIn.c_str() );
    }
}

void options_manager::string_input_option::set_from_legacy_value( const std::string &sSetIn )
{
    value_ = (iMaxLength > 0) ? sSetIn.substr(0, iMaxLength) : sSetIn;
}

void options_manager::display_device_option::set_from_legacy_value( const std::string &sSetIn )
{
    value_ = atoi( sSetIn.c_str() );
    if( !displays_.empty() && displays_.count( value_ ) == 0 ) {
        value_ = displays_.begin()->first;
    }
    assert( displays_.empty() || displays_.count( value_ ) > 0 );
}

void options_manager::int_option::set_from_legacy_value( const std::string &sSetIn )
{
    value( atoi( sSetIn.c_str() ) );
}

void options_manager::string_map_option::set_from_legacy_value( const std::string &sSetIn )
{
    if (getItemPos(sSetIn) != -1) {
        value_ = sSetIn;
    }
}

/** Fill a mapping with values.
 * Scans all directories in FILENAMES[dirname_label] directory for
 * a file named FILENAMES[filename_label].
 * All found values added to resource_option as name, resource_dir.
 * Furthermore, it builds possible values list for string_map_option class.
 */
static std::vector<std::pair<std::string, std::string>> build_resource_list(
    std::map<std::string, std::string> &resource_option, std::string operation_name,
    std::string dirname_label, std::string filename_label ) {
    std::vector<std::pair<std::string, std::string>> resource_names;

    resource_option.clear();
    auto const resource_dirs = get_directories_with( FILENAMES[filename_label],
                                                     FILENAMES[dirname_label], true );

    for( auto &resource_dir : resource_dirs ) {
        read_from_file( resource_dir + "/" + FILENAMES[filename_label], [&]( std::istream &fin ) {
            std::string resource_name;
            std::string view_name;
            // should only have 2 values inside it, otherwise is going to only load the last 2 values
            while( !fin.eof() ) {
                std::string sOption;
                fin >> sOption;

                if( sOption.empty() ) {
                    getline( fin, sOption );    // Empty line, chomp it
                } else if( sOption[0] == '#' ) { // # indicates a comment
                    getline( fin, sOption );
                } else {
                    if( sOption.find( "NAME" ) != std::string::npos ) {
                        resource_name.clear();
                        getline( fin, resource_name );
                        resource_name = trim( resource_name );
                    } else if( sOption.find( "VIEW" ) != std::string::npos ) {
                        view_name.clear();
                        getline( fin, view_name );
                        view_name = trim( view_name );
                        break;
                    }
                }
            }
            resource_names.emplace_back( resource_name, view_name.empty() ? resource_name : view_name );
            if( resource_option.count( resource_name ) != 0 ) {
                DebugLog( D_ERROR, DC_ALL ) << "Found " << operation_name << " duplicate with name " << resource_name;
            } else {
                resource_option.insert( std::pair<std::string,std::string>( resource_name, resource_dir ) );
            }
        } );
    }

    return resource_names;
}

std::vector<std::pair<std::string, std::string>> options_manager::build_tilesets_list()
{
    auto tileset_names = build_resource_list( TILESETS, "tileset",
                                                     "gfxdir", "tileset-conf");

    if( tileset_names.empty() ) {
        tileset_names.emplace_back( "hoder", translate_marker( "Hoder's" ) );
        tileset_names.emplace_back( "deon", translate_marker( "Deon's" ) );
    }
    return tileset_names;
}

std::vector<std::pair<std::string, std::string>> options_manager::build_soundpacks_list()
{
    auto soundpack_names = build_resource_list( SOUNDPACKS, "soundpack",
                                                             "sounddir", "soundpack-conf");
    if( soundpack_names.empty() ) {
        soundpack_names.emplace_back( "basic", translate_marker( "Basic" ) );
    }
    return soundpack_names;
}

void options_manager::init()
{
    options.clear();
    vPages.clear();
    mPageItems.clear();
    mOptionsSort.clear();

    vPages.emplace_back( "general", translate_marker( "General" ) );
    vPages.emplace_back( "interface", translate_marker( "Interface" ) );
    vPages.emplace_back( "graphics", translate_marker( "Graphics" ) );
    // when sharing maps only admin is allowed to change these.
    if(!MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin()) {
        vPages.emplace_back( "debug", translate_marker( "Debug" ) );
    }
    iWorldOptPage = vPages.size();
    // when sharing maps only admin is allowed to change these.
    if(!MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin()) {
        vPages.emplace_back( "world_default", translate_marker( "World Defaults" ) );
    }

    ////////////////////////////GENERAL//////////////////////////
    add( string_input_option( "DEF_CHAR_NAME", "general",
                              translate_marker( "Default character name" ),
                              translate_marker( "Set a default character name that will be used instead of a random name on character creation." ),
                              COPT_NO_HIDE, "", 30 ) );

    mOptionsSort["general"]++;

    add( bool_option( "AUTO_PICKUP", "general",
                      translate_marker( "Auto pickup enabled" ),
                      translate_marker( "Enable item auto pickup.  Change pickup rules with the Auto Pickup Manager." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "AUTO_PICKUP_ADJACENT", "general",
                      translate_marker( "Auto pickup adjacent" ),
                      translate_marker( "If true, will enable to pickup items one tile around to the player.  You can assign No Auto Pickup zones with the Zones Manager 'Y' key for e.g.  your homebase." ),
                      COPT_NO_HIDE, false ) );

    get_option("AUTO_PICKUP_ADJACENT").setPrerequisite("AUTO_PICKUP");

    add( int_option( "AUTO_PICKUP_WEIGHT_LIMIT", "general",
                     translate_marker( "Auto pickup weight limit" ),
                     translate_marker( "Auto pickup items with weight less than or equal to [option] * 50 grams.  You must also set the small items option.  '0' disables this option" ),
                     COPT_NO_HIDE, 0, 0, 20 ) );

    get_option("AUTO_PICKUP_WEIGHT_LIMIT").setPrerequisite("AUTO_PICKUP");

    add( int_option( "AUTO_PICKUP_VOL_LIMIT", "general",
                     translate_marker( "Auto pickup volume limit" ),
                     translate_marker( "Auto pickup items with volume less than or equal to [option] * 50 milliliters.  You must also set the light items option.  '0' disables this option" ),
                     COPT_NO_HIDE, 0, 0, 20 ) );

    get_option("AUTO_PICKUP_VOL_LIMIT").setPrerequisite("AUTO_PICKUP");

    add( bool_option( "AUTO_PICKUP_SAFEMODE", "general",
                      translate_marker( "Auto pickup safe mode" ),
                      translate_marker( "Auto pickup is disabled as long as you can see monsters nearby.  This is affected by 'Safe Mode proximity distance'." ),
                      COPT_NO_HIDE, false ) );

    get_option("AUTO_PICKUP_SAFEMODE").setPrerequisite("AUTO_PICKUP");

    add( bool_option( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS", "general",
                      translate_marker( "List items within no auto pickup zones" ),
                      translate_marker( "If false, you will not see messages about items, you step on, within no auto pickup zones." ),
                      COPT_NO_HIDE, true ) );

    get_option("NO_AUTO_PICKUP_ZONES_LIST_ITEMS").setPrerequisite("AUTO_PICKUP");

    mOptionsSort["general"]++;

    add( bool_option( "AUTO_PULP_BUTCHER", "general",
                      translate_marker( "Auto pulp or butcher" ),
                      translate_marker( "If true, enables auto pulping resurrecting corpses or auto butchering any corpse.  Never pulps acidic corpses.  Disabled as long as any enemy monster is seen." ),
                      COPT_NO_HIDE, false ) );

    add( string_map_option( "AUTO_PULP_BUTCHER_ACTION", "general",
                            translate_marker( "Auto pulp or butcher action" ),
                            translate_marker( "Action to perform when 'Auto pulp or butcher' is enabled.  Pulp: Pulp corpses you stand on.  - Pulp Adjacent: Also pulp corpses adjacent from you.  - Butcher: Butcher corpses you stand on." ),
    COPT_NO_HIDE, "butcher", { { "pulp", translate_marker( "Pulp" ) }, { "pulp_adjacent", translate_marker( "Pulp Adjacent" ) }, { "butcher", translate_marker( "Butcher" ) } } ) );

    get_option("AUTO_PULP_BUTCHER_ACTION").setPrerequisite("AUTO_PULP_BUTCHER");

    mOptionsSort["general"]++;

    add( bool_option( "DANGEROUS_PICKUPS", "general",
                      translate_marker( "Dangerous pickups" ),
                      translate_marker( "If false, will cause player to drop new items that cause them to exceed the weight limit." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["general"]++;

    add( bool_option( "SAFEMODE", "general",
                      translate_marker( "Safe mode" ),
                      translate_marker( "If true, will hold the game and display a warning if a hostile monster/npc is approaching." ),
                      COPT_NO_HIDE, true ) );

    add( int_option( "SAFEMODEPROXIMITY", "general",
                     translate_marker( "Safe mode proximity distance" ),
                     translate_marker( "If safe mode is enabled, distance to hostiles at which safe mode should show a warning.  0 = Max player view distance." ),
                     COPT_NO_HIDE, 0, 0, MAX_VIEW_DISTANCE ) );

    add( bool_option( "SAFEMODEVEH", "general",
                      translate_marker( "Safe mode when driving" ),
                      translate_marker( "When true, safe mode will alert you of hostiles while you are driving a vehicle." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "AUTOSAFEMODE", "general",
                      translate_marker( "Auto reactivate safe mode" ),
                      translate_marker( "If true, safe mode will automatically reactivate after a certain number of turns.  See option 'Turns to auto reactivate safe mode.'" ),
                      COPT_NO_HIDE, false ) );

    add( int_option( "AUTOSAFEMODETURNS", "general",
                     translate_marker( "Turns to auto reactivate safe mode" ),
                     translate_marker( "Number of turns after which safe mode is reactivated. Will only reactivate if no hostiles are in 'Safe mode proximity distance.'" ),
                     COPT_NO_HIDE, 50, 1, 100 ) );

    mOptionsSort["general"]++;

    add( float_option( "TURN_DURATION", "general",
                       translate_marker( "Realtime turn progression" ),
                       translate_marker( "If enabled, monsters will take periodic gameplay turns.  This value is the delay between each turn, in seconds.  Works best with Safe Mode disabled.  0 = disabled." ),
                       COPT_NO_HIDE, 0.0f, 0.0f, 10.0f, 0.05f ) );

    mOptionsSort["general"]++;

    add( bool_option( "AUTOSAVE", "general",
                      translate_marker( "Autosave" ),
                      translate_marker( "If true, game will periodically save the map.  Autosaves occur based on in-game turns or real-time minutes, whichever is larger." ),
                      COPT_NO_HIDE, false ) );

    add( int_option( "AUTOSAVE_TURNS", "general",
                     translate_marker( "Game turns between autosaves" ),
                     translate_marker( "Number of game turns between autosaves" ),
                     COPT_NO_HIDE, 50, 10, 1000 ) );

    get_option("AUTOSAVE_TURNS").setPrerequisite("AUTOSAVE");

    add( int_option( "AUTOSAVE_MINUTES", "general",
                     translate_marker( "Real minutes between autosaves" ),
                     translate_marker( "Number of real time minutes between autosaves" ),
                     COPT_NO_HIDE, 5, 0, 127 ) );

    get_option("AUTOSAVE_MINUTES").setPrerequisite("AUTOSAVE");

    mOptionsSort["general"]++;

    add( bool_option( "CIRCLEDIST", "general",
                      translate_marker( "Circular distances" ),
                      translate_marker( "If true, the game will calculate range in a realistic way: light sources will be circles, diagonal movement will cover more ground and take longer.  If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall." ),
                      COPT_NO_HIDE, false ) );

    add( string_map_option( "DROP_EMPTY", "general",
                            translate_marker( "Drop empty containers" ),
                            translate_marker( "Set to drop empty containers after use.  No: Don't drop any. - Watertight: All except watertight containers. - All: Drop all containers." ),
    COPT_NO_HIDE, "no", { { "no", translate_marker( "No" ) }, { "watertight", translate_marker( "Watertight" ) }, { "all", translate_marker( "All" ) } } ) );

    add( bool_option( "AUTO_NOTES", "general",
                      translate_marker( "Auto notes" ),
                      translate_marker( "If true, automatically sets notes on places that have stairs that go up or down" ),
                      COPT_NO_HIDE, true ) );

    add( string_map_option( "DEATHCAM", "general",
                            translate_marker( "DeathCam" ),
                            translate_marker( "Always: Always start deathcam.  Ask: Query upon death.  Never: Never show deathcam." ),
    COPT_NO_HIDE, "ask", { { "always", translate_marker( "Always" ) }, { "ask", translate_marker( "Ask" ) }, { "never", translate_marker( "Never" ) } } ) );

    mOptionsSort["general"]++;

    add( string_map_option( "SOUNDPACKS", "general",
                            translate_marker( "Choose soundpack" ),
                            translate_marker( "Choose the soundpack you want to use." ),
                            COPT_NO_SOUND_HIDE, "basic", build_soundpacks_list() ) );

    add( int_option( "MUSIC_VOLUME", "general",
                     translate_marker( "Music volume" ),
                     translate_marker( "Adjust the volume of the music being played in the background." ),
                     COPT_NO_SOUND_HIDE, 100, 0, 200 ) );

    add( int_option( "SOUND_EFFECT_VOLUME", "general",
                     translate_marker( "Sound effect volume" ),
                     translate_marker( "Adjust the volume of sound effects being played by the game." ),
                     COPT_NO_SOUND_HIDE, 100, 0, 200 ) );

    ////////////////////////////INTERFACE////////////////////////
    // TODO: scan for languages like we do for tilesets.
    add( string_map_option( "USE_LANG", "interface",
                            translate_marker( "Language" ),
                            translate_marker( "Switch Language." ),
    COPT_NO_HIDE, "", {
        { "", translate_marker( "System language" ) },
        // Note: language names are in their own language and are *not* translated at all.
        // Note: Somewhere in Github PR was better link to msdn.microsoft.com with language names.
        // http://en.wikipedia.org/wiki/List_of_language_names
        { "en", R"( English )" },
        { "fr",  R"( Français )" },
        { "de", R"( Deutsch )" },
        { "it_IT", R"( Italiano )" },
        { "es_AR", R"( Español ( Argentina ) )" },
        { "es_ES", R"( Español ( España ) )" },
        { "ja", R"( 日本語 )" },
        { "ko", R"( 한국어 )" },
        { "pl", R"( Polskie )" },
        { "pt_BR", R"( Português ( Brasil ) )" },
        { "ru", R"( Русский )" },
        { "zh_CN", R"( 中文( 天朝 ) )" },
        { "zh_TW", R"( 中文( 台灣 ) )" },
    } ) );

    mOptionsSort["interface"]++;

    add( string_map_option( "USE_CELSIUS", "interface",
                            translate_marker( "Temperature units" ),
                            translate_marker( "Switch between Celsius and Fahrenheit." ),
    COPT_NO_HIDE, "fahrenheit", { { "fahrenheit", translate_marker( "Fahrenheit" ) }, { "celsius", translate_marker( "Celsius" ) } } ) );

    add( string_map_option( "USE_METRIC_SPEEDS", "interface",
                            translate_marker( "Speed units" ),
                            translate_marker( "Switch between km/h and mph." ),
    COPT_NO_HIDE, "mph", { { "mph", translate_marker( "mph" ) }, { "km/h", translate_marker( "km/h" ) } } ) );

    add( string_map_option( "USE_METRIC_WEIGHTS", "interface",
                            translate_marker( "Mass units" ),
                            translate_marker( "Switch between kg and lbs." ),
    COPT_NO_HIDE, "lbs", { { "lbs", translate_marker( "lbs" ) }, { "kg", translate_marker( "kg" ) } } ) );

    add( string_map_option( "VOLUME_UNITS", "interface",
                            translate_marker( "Volume units" ),
                            translate_marker( "Switch between the Cup ( c ), Liter ( L ) or Quart ( qt )." ),
    COPT_NO_HIDE, "l", { { "c", translate_marker( "Cup" ) }, { "l", translate_marker( "Liter" ) }, { "qt", translate_marker( "Quart" ) } } ) );

    add( string_map_option( "24_HOUR", "interface",
                            translate_marker( "Time format" ),
                            translate_marker( "12h: AM/PM, e.g. 7:31 AM - Military: 24h Military, e.g. 0731 - 24h: Normal 24h, e.g. 7:31" ),
                            COPT_NO_HIDE, "12h",
                            //~ 12h time, e.g.  11:59pm
    {   { "12h", translate_marker( "12h" ) },
        //~ Military time, e.g.  2359
        { "military", translate_marker( "Military" ) },
        //~ 24h time, e.g.  23:59
        { "24h", translate_marker( "24h" ) }
    } ) );

    mOptionsSort["interface"]++;

    add( bool_option( "FORCE_CAPITAL_YN", "interface",
                      translate_marker( "Force Y/N in prompts" ),
                      translate_marker( "If true, Y/N prompts are case-sensitive and y and n are not accepted." ),
                      COPT_NO_HIDE, true ) );
    
    add( bool_option( "SNAP_TO_TARGET", "interface",
                      translate_marker( "Snap to target" ),
                      translate_marker( "If true, automatically follow the crosshair when firing/throwing." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "QUERY_DISASSEMBLE", "interface",
                      translate_marker( "Query on disassembly" ),
                      translate_marker( "If true, will query before disassembling items." ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "QUERY_KEYBIND_REMOVAL", "interface",
                      translate_marker( "Query on keybinding removal" ),
                      translate_marker( "If true, will query before removing a keybinding from a hotkey." ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "CLOSE_ADV_INV", "interface",
                      translate_marker( "Close advanced inventory on move all" ),
                      translate_marker( "If true, will close the advanced inventory when the move all items command is used." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "OPEN_DEFAULT_ADV_INV", "interface",
                      translate_marker( "Open default advanced inventory layout" ),
                      translate_marker( "Open default advanced inventory layout instead of last opened layout" ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "INV_USE_ACTION_NAMES", "interface",
                      translate_marker( "Display actions in Use Item menu" ),
                      translate_marker( "If true, actions ( like \"Read\", \"Smoke\", \"Wrap tighter\" ) will be displayed next to the corresponding items." ),
                      COPT_NO_HIDE, true ) );

    mOptionsSort["interface"]++;

    add( bool_option( "DIAG_MOVE_WITH_MODIFIERS", "interface",
                      translate_marker( "Diagonal movement with cursor keys and modifiers" ),
                      translate_marker( "If true, allows diagonal movement with cursor keys using CTRL and SHIFT modifiers.  Diagonal movement action keys are taken from keybindings, so you need these to be configured." ),
                      COPT_CURSES_HIDE, true ) );

    mOptionsSort["interface"]++;

    add( bool_option( "VEHICLE_ARMOR_COLOR", "interface",
                      translate_marker( "Vehicle plating changes part color" ),
                      translate_marker( "If true, vehicle parts will change color if they are armor plated" ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "DRIVING_VIEW_OFFSET", "interface",
                      translate_marker( "Auto-shift the view while driving" ),
                      translate_marker( "If true, view will automatically shift towards the driving direction" ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "VEHICLE_DIR_INDICATOR", "interface",
                      translate_marker( "Draw vehicle facing indicator" ),
                      translate_marker( "If true, when controlling a vehicle, a white 'X' ( in curses version ) or a crosshair ( in tiles version ) at distance 10 from the center will display its current facing." ),
                      COPT_NO_HIDE, true ) );

    mOptionsSort["interface"]++;

    add( string_map_option( "SIDEBAR_POSITION", "interface",
                            translate_marker( "Sidebar position" ),
                            translate_marker( "Switch between sidebar on the left or on the right side.  Requires restart." ),
                            //~ sidebar position
    COPT_NO_HIDE, "right", { { "left", translate_marker( "Left" ) }, { "right", translate_marker( "Right" ) } } ) );

    add( string_map_option( "SIDEBAR_STYLE", "interface",
                            translate_marker( "Sidebar style" ),
                            translate_marker( "Switch between a narrower or wider sidebar." ),
                            //~ sidebar style
    COPT_NO_HIDE, "narrow", { { "wider", translate_marker( "Wider" ) }, { "narrow", translate_marker( "Narrow" ) } } ) );

    add( string_map_option( "LOG_FLOW", "interface",
                            translate_marker( "Message log flow" ),
                            translate_marker( "Where new log messages should show." ),
                            //~ sidebar/message log flow direction
    COPT_NO_HIDE, "new_bottom", { { "new_top", translate_marker( "Top" ) }, { "new_bottom", translate_marker( "Bottom" ) } } ) );

    add( int_option( "MESSAGE_TTL", "interface",
                     translate_marker( "Sidebar log message display duration" ),
                     translate_marker( "Number of turns after which a message will be removed from the sidebar log.  '0' disables this option." ),
                     COPT_NO_HIDE, 0, 0, 1000 ) );

    add( string_map_option( "ACCURACY_DISPLAY", "interface",
                            translate_marker( "Aim window display style" ),
                            translate_marker( "How should confidence and steadiness be communicated to the player." ),
                            //~ aim bar style - bars or numbers
    COPT_NO_HIDE, "bars", { { "numbers", translate_marker( "Numbers" ) }, { "bars", translate_marker( "Bars" ) } } ) );

    add( string_map_option( "MORALE_STYLE", "interface",
                            translate_marker( "Morale style" ),
                            translate_marker( "Morale display style in sidebar." ),
                            //~ aim bar style - bars or numbers
    COPT_NO_HIDE, "Vertical", { { "vertical", translate_marker( "Vertical" ) }, { "horizontal", translate_marker( "Horizontal" ) } } ) );

    mOptionsSort["interface"]++;

    add( int_option( "MOVE_VIEW_OFFSET", "interface",
                     translate_marker( "Move view offset" ),
                     translate_marker( "Move view by how many squares per keypress." ),
                     COPT_NO_HIDE, 1, 1, 50 ) );

    add( bool_option( "MENU_SCROLL", "interface",
                      translate_marker( "Centered menu scrolling" ),
                      translate_marker( "If true, menus will start scrolling in the center of the list, and keep the list centered." ),
                      COPT_NO_HIDE, true ) );

    add( string_map_option( "SHIFT_LIST_ITEM_VIEW", "interface",
                            translate_marker( "Shift list item view" ),
                            translate_marker( "Centered or to edge, shift the view toward the selected item if it is outside of your current viewport." ),
    COPT_NO_HIDE, "centered", { { "false", translate_marker( "False" ) }, { "centered", translate_marker( "Centered" ) }, { "edge", translate_marker( "To edge" ) } } ) );

    add( bool_option( "AUTO_INV_ASSIGN", "interface",
                      translate_marker( "Auto inventory letters" ),
                      translate_marker( "If false, new inventory items will only get letters assigned if they had one before." ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "ITEM_HEALTH_BAR", "interface",
                      translate_marker( "Show item health bars" ),
                      translate_marker( "If true, show item health bars instead of reinforced, scratched etc. text." ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "ITEM_SYMBOLS", "interface",
                      translate_marker( "Show item symbols" ),
                      translate_marker( "If true, show item symbols in inventory and pick up menu." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["interface"]++;

    add( bool_option( "ENABLE_JOYSTICK", "interface",
                      translate_marker( "Enable joystick" ),
                      translate_marker( "Enable input from joystick." ),
                      COPT_CURSES_HIDE, true ) );

    add( string_map_option( "HIDE_CURSOR", "interface",
                            translate_marker( "Hide mouse cursor" ),
                            translate_marker( "Show: Cursor is always shown.  Hide: Cursor is hidden.  HideKB: Cursor is hidden on keyboard input and unhidden on mouse movement." ),
    COPT_CURSES_HIDE, "show", {
        //~ show mouse cursor
        { "show", translate_marker( "Show" ) },
        //~ hide mouse cursor
        { "hide", translate_marker( "Hide" ) },
        //~ hide mouse cursor when keyboard is used
        { "hidekb", translate_marker( "HideKB" ) }
    } ) );

    ////////////////////////////GRAPHICS/////////////////////////
    add( bool_option( "ANIMATIONS", "graphics",
                      translate_marker( "Animations" ),
                      translate_marker( "If true, will display enabled animations." ),
                      COPT_NO_HIDE, true ) );

    add( bool_option( "ANIMATION_RAIN", "graphics",
                      translate_marker( "Rain animation" ),
                      translate_marker( "If true, will display weather animations." ),
                      COPT_NO_HIDE, true ) );

    get_option("ANIMATION_RAIN").setPrerequisite("ANIMATIONS");

    add( bool_option( "ANIMATION_SCT", "graphics",
                      translate_marker( "SCT animation" ),
                      translate_marker( "If true, will display scrolling combat text animations." ),
                      COPT_NO_HIDE, true ) );

    get_option("ANIMATION_SCT").setPrerequisite("ANIMATIONS");

    add( int_option( "ANIMATION_DELAY", "graphics",
                     translate_marker( "Animation delay" ),
                     translate_marker( "The amount of time to pause between animation frames in ms." ),
                     COPT_NO_HIDE, 10, 0, 100 ) );

    get_option("ANIMATION_DELAY").setPrerequisite("ANIMATIONS");

    add( bool_option( "FORCE_REDRAW", "graphics",
                      translate_marker( "Force redraw" ),
                      translate_marker( "If true, forces the game to redraw at least once per turn." ),
                      COPT_NO_HIDE, true ) );

    mOptionsSort["graphics"]++;

    add( int_option( "TERMINAL_X", "graphics",
                     translate_marker( "Terminal width" ),
                     translate_marker( "Set the size of the terminal along the X axis.  Requires restart." ),
                     COPT_POSIX_CURSES_HIDE, 80, 80, 960 ) );

    add( int_option( "TERMINAL_Y", "graphics",
                     translate_marker( "Terminal height" ),
                     translate_marker( "Set the size of the terminal along the Y axis.  Requires restart." ),
                     COPT_POSIX_CURSES_HIDE, 24, 24, 270 ) );

    mOptionsSort["graphics"]++;

    add( bool_option( "USE_TILES", "graphics",
                      translate_marker( "Use tiles" ),
                      translate_marker( "If true, replaces some TTF rendered text with tiles." ),
                      COPT_CURSES_HIDE, true ) );

    add( string_map_option( "TILES", "graphics",
                            translate_marker( "Choose tileset" ),
                            translate_marker( "Choose the tileset you want to use." ),
                            COPT_CURSES_HIDE, "ChestHole", build_tilesets_list() ) );

    get_option("TILES").setPrerequisite("USE_TILES");

    add( bool_option( "PIXEL_MINIMAP", "graphics",
                      translate_marker( "Pixel minimap" ),
                      translate_marker( "If true, shows the pixel-detail minimap in game after the save is loaded.  Use the 'Toggle Pixel Minimap' action key to change its visibility during gameplay." ),
                      COPT_CURSES_HIDE, true ) );

    add( string_map_option( "PIXEL_MINIMAP_MODE", "graphics",
                            translate_marker( "Pixel minimap drawing mode" ),
                            translate_marker( "Specified the mode in which the minimap drawn." ),
    COPT_CURSES_HIDE, "dots", {
        { "solid", translate_marker( "Solid" ) },
        { "squares", translate_marker( "Squares" ) },
        { "dots", translate_marker( "Dots" ) }
    } ) );

    get_option("PIXEL_MINIMAP_MODE").setPrerequisite("PIXEL_MINIMAP");

    add( int_option( "PIXEL_MINIMAP_BRIGHTNESS", "graphics",
                     translate_marker( "Pixel minimap brightness" ),
                     translate_marker( "Overall brightness of pixel-detail minimap." ),
                     COPT_CURSES_HIDE, 100, 10, 300 ) );

    get_option("PIXEL_MINIMAP_BRIGHTNESS").setPrerequisite("PIXEL_MINIMAP");

    add( int_option( "PIXEL_MINIMAP_HEIGHT", "graphics",
                     translate_marker( "Pixel minimap height" ),
                     translate_marker( "Height of pixel-detail minimap, measured in terminal rows.  Set to 0 for default spacing." ),
                     COPT_CURSES_HIDE, 0, 0, 100 ) );

    get_option("PIXEL_MINIMAP_HEIGHT").setPrerequisite("PIXEL_MINIMAP");

    add( bool_option( "PIXEL_MINIMAP_RATIO", "graphics",
                      translate_marker( "Maintain pixel minimap aspect ratio" ),
                      translate_marker( "Preserves the square shape of tiles shown on the pixel minimap." ),
                      COPT_CURSES_HIDE, true ) );

    get_option("PIXEL_MINIMAP_RATIO").setPrerequisite("PIXEL_MINIMAP");

    add( int_option( "PIXEL_MINIMAP_BLINK", "graphics",
                     translate_marker( "Enemy beacon blink speed" ),
                     translate_marker( "Controls how fast the enemy beacons blink on the pixel minimap.  Value is multiplied by 200 ms.  Set to 0 to disable." ),
                     COPT_CURSES_HIDE, 10, 0, 50 ) );

    get_option("PIXEL_MINIMAP_BLINK").setPrerequisite("PIXEL_MINIMAP");

    mOptionsSort["graphics"]++;

    // Note: list of display devices is added later by SDL code
    add( display_device_option() );

    add( string_map_option( "FULLSCREEN", "graphics",
                            translate_marker( "Fullscreen" ),
                            translate_marker( "Starts Cataclysm in one of the fullscreen modes.  Requires restart." ),
                            COPT_CURSES_HIDE, "no",
    { { "no", translate_marker( "No" ) }, { "fullscreen", translate_marker( "Fullscreen" ) }, { "windowedbl", translate_marker( "Windowed borderless" ) } } ) );

    add( bool_option( "SOFTWARE_RENDERING", "graphics",
                      translate_marker( "Software rendering" ),
                      translate_marker( "Use software renderer instead of graphics card acceleration.  Requires restart." ),
                      COPT_CURSES_HIDE, false ) );

    add( bool_option( "FRAMEBUFFER_ACCEL", "graphics",
                      translate_marker( "Software framebuffer acceleration" ),
                      translate_marker( "Use hardware acceleration for the framebuffer when using software rendering.  Requires restart." ),
                      COPT_CURSES_HIDE, false ) );

    get_option("FRAMEBUFFER_ACCEL").setPrerequisite("SOFTWARE_RENDERING");

    add( string_map_option( "SCALING_MODE", "graphics",
                            translate_marker( "Scaling mode" ),
                            translate_marker( "Sets the scaling mode, 'none' ( default ) displays at the game's native resolution, 'nearest'  uses low-quality but fast scaling, and 'linear' provides high-quality scaling." ),
    COPT_CURSES_HIDE, "none", {
        //~ Do not scale the game image to the window size.
        { "none", translate_marker( "No scaling" ) },
        //~ An algorithm for image scaling.
        { "nearest", translate_marker( "Nearest neighbor" ) },
        //~ An algorithm for image scaling.
        { "linear", translate_marker( "Linear filtering" ) }
    } ) );

    ////////////////////////////DEBUG////////////////////////////
    add( int_option( "DISTANCE_INITIAL_VISIBILITY", "debug",
                     translate_marker( "Distance initial visibility" ),
                     translate_marker( "Determines the scope, which is known in the beginning of the game." ),
                     COPT_NO_HIDE, 15, 3, 20 ) );

    mOptionsSort["debug"]++;

    add( int_option( "INITIAL_STAT_POINTS", "debug",
                     translate_marker( "Initial stat points" ),
                     translate_marker( "Initial points available to spend on stats on character generation." ),
                     COPT_NO_HIDE, 6, 0, 1000 ) );

    add( int_option( "INITIAL_TRAIT_POINTS", "debug",
                     translate_marker( "Initial trait points" ),
                     translate_marker( "Initial points available to spend on traits on character generation." ),
                     COPT_NO_HIDE, 0, 0, 1000 ) );

    add( int_option( "INITIAL_SKILL_POINTS", "debug",
                     translate_marker( "Initial skill points" ),
                     translate_marker( "Initial points available to spend on skills on character generation." ),
                     COPT_NO_HIDE, 2, 0, 1000 ) );

    add( int_option( "MAX_TRAIT_POINTS", "debug",
                     translate_marker( "Maximum trait points" ),
                     translate_marker( "Maximum trait points available for character generation." ),
                     COPT_NO_HIDE, 12, 0, 1000 ) );

    mOptionsSort["debug"]++;

    add( float_option( "SKILL_TRAINING_SPEED", "debug",
                       translate_marker( "Skill training speed" ),
                       translate_marker( "Scales experience gained from practicing skills and reading books.  0.5 is half as fast as default, 2.0 is twice as fast, 0.0 disables skill training except for NPC training." ),
                       COPT_NO_HIDE, 1.0f, 0.0f, 100.0f, 0.1f ) );

    mOptionsSort["debug"]++;

    add( string_map_option( "SKILL_RUST", "debug",
                            translate_marker( "Skill rust" ),
                            translate_marker( "Set the level of skill rust.  Vanilla: Vanilla Cataclysm - Capped: Capped at skill levels 2 - Int: Intelligence dependent - IntCap: Intelligence dependent, capped - Off: None at all." ),
    COPT_NO_HIDE, "off", {
        //~ plain, default, normal
        { "vanilla", translate_marker( "Vanilla" ) },
        //~ capped at a value
        { "capped", translate_marker( "Capped" ) },
        //~ based on intelligence
        { "int", translate_marker( "Int" ) },
        //~ based on intelligence and capped
        { "intcap", translate_marker( "IntCap" ) },
        { "off", translate_marker( "Off" ) }
    } ) );

    mOptionsSort["debug"]++;

    add( bool_option( "FOV_3D", "debug",
                      translate_marker( "Experimental 3D field of vision" ),
                      translate_marker( "If false, vision is limited to current z-level.  If true and the world is in z-level mode, the vision will extend beyond current z-level.  Currently very bugged!" ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "ENCODING_CONV", "debug",
                      translate_marker( "Experimental path name encoding conversion" ),
                      translate_marker( "If true, file path names are going to be transcoded from system encoding to UTF-8 when reading and will be transcoded back when writing.  Mainly for CJK Windows users." ),
                      COPT_NO_HIDE, true ) );

    ////////////////////////////WORLD DEFAULT////////////////////
    add( int_option( "CORE_VERSION", "world_default",
                     translate_marker( "Core version data" ),
                     translate_marker( "Controls what migrations are applied for legacy worlds" ),
                     COPT_ALWAYS_HIDE, core_version, 1, core_version ) );

    mOptionsSort["world_default"]++;

    add( string_map_option( "DELETE_WORLD", "world_default",
                            translate_marker( "Delete world" ),
                            translate_marker( "Delete the world when the last active character dies." ),
    COPT_NO_HIDE, "no", { { "no", translate_marker( "No" ) }, { "yes", translate_marker( "Yes" ) }, { "query", translate_marker( "Query" ) } } ) );

    mOptionsSort["world_default"]++;

    add( int_option( "CITY_SIZE", "world_default",
                     translate_marker( "Size of cities" ),
                     translate_marker( "A number determining how large cities are.  0 disables cities, roads and any scenario requiring a city start." ),
                     COPT_NO_HIDE, 4, 0, 16 ) );

    add( int_option( "CITY_SPACING", "world_default",
                     translate_marker( "City spacing" ),
                     translate_marker( "A number determining how far apart cities are.  Warning, small numbers lead to very slow mapgen." ),
                     COPT_NO_HIDE, 4, 0, 8 ) );

    add( float_option( "SPAWN_DENSITY", "world_default",
                       translate_marker( "Spawn rate scaling factor" ),
                       translate_marker( "A scaling factor that determines density of monster spawns." ),
                       COPT_NO_HIDE, 1.0f, 0.0f, 50.0f, 0.1f ) );

    add( float_option( "ITEM_SPAWNRATE", "world_default",
                       translate_marker( "Item spawn scaling factor" ),
                       translate_marker( "A scaling factor that determines density of item spawns." ),
                       COPT_NO_HIDE, 1.0f, 0.01f, 10.0, 0.01f ) );

    add( float_option( "NPC_DENSITY", "world_default",
                       translate_marker( "NPC spawn rate scaling factor" ),
                       translate_marker( "A scaling factor that determines density of dynamic NPC spawns." ),
                       COPT_NO_HIDE, 0.1f, 0.0f, 100.0f, 0.01f ) );

    add( float_option( "MONSTER_UPGRADE_FACTOR", "world_default",
                       translate_marker( "Monster evolution scaling factor" ),
                       translate_marker( "A scaling factor that determines the time between monster upgrades.  A higher number means slower evolution.  Set to 0.00 to turn off monster upgrades." ),
                       COPT_NO_HIDE, 4.0f,  0.0f, 100.0f, 0.01f ) );

    mOptionsSort["world_default"]++;

    add( int_option( "MONSTER_SPEED", "world_default",
                     translate_marker( "Monster speed" ),
                     translate_marker( "Determines the movement rate of monsters.  A higher value increases monster speed and a lower reduces it." ),
                     COPT_NO_HIDE, 100, 1, 1000, "%i%%" ) );

    add( int_option( "MONSTER_RESILIENCE", "world_default",
                     translate_marker( "Monster resilience" ),
                     translate_marker( "Determines how much damage monsters can take.  A higher value makes monsters more resilient and a lower makes them more flimsy." ),
                     COPT_NO_HIDE, 100, 1, 1000, "%i%%" ) );

    mOptionsSort["world_default"]++;

    add( string_map_option( "DEFAULT_REGION", "world_default",
                            translate_marker( "Default region type" ),
                            translate_marker( "( WIP feature ) Determines terrain, shops, plants, and more." ),
    COPT_NO_HIDE, "default", { { "default", "default" } } ) );

    mOptionsSort["world_default"]++;

    add( int_option( "INITIAL_TIME", "world_default",
                     translate_marker( "Initial time" ),
                     translate_marker( "Initial starting time of day on character generation." ),
                     COPT_NO_HIDE, 8, 0, 23 ) );

    add( string_map_option( "INITIAL_SEASON", "world_default",
                            translate_marker( "Initial season" ),
                            translate_marker( "Season the player starts in.  Options other than the default delay spawn of the character, so food decay and monster spawns will have advanced." ),
    COPT_NO_HIDE, "spring", { { "spring", translate_marker( "Spring" ) }, { "summer", translate_marker( "Summer" ) }, { "autumn", translate_marker( "Autumn" ) }, { "winter", translate_marker( "Winter" ) } } ) );

    add( int_option( "SEASON_LENGTH", "world_default",
                     translate_marker( "Season length" ),
                     translate_marker( "Season length, in days." ),
                     COPT_NO_HIDE, 14, 14, 127 ) );

    add( int_option( "CONSTRUCTION_SCALING", "world_default",
                     translate_marker( "Construction scaling" ),
                     translate_marker( "Sets the time of construction in percents.  '50' is two times faster than default, '200' is two times longer.  '0' automatically scales construction time to match the world's season length." ),
                     COPT_NO_HIDE, 100, 0, 1000 ) );

    add( bool_option( "ETERNAL_SEASON", "world_default",
                      translate_marker( "Eternal season" ),
                      translate_marker( "Keep the initial season for ever." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "WANDER_SPAWNS", "world_default",
                      translate_marker( "Wander spawns" ),
                      translate_marker( "Emulation of zombie hordes.  Zombie spawn points wander around cities and may go to noise.  Must reset world directory after changing for it to take effect." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "CLASSIC_ZOMBIES", "world_default",
                      translate_marker( "Classic zombies" ),
                      translate_marker( "Only spawn classic zombies and natural wildlife.  Requires a reset of save folder to take effect.  This disables certain buildings." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "BLACK_ROAD", "world_default",
                      translate_marker( "Surrounded start" ),
                      translate_marker( "If true, spawn zombies at shelters.  Makes the starting game a lot harder." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "STATIC_NPC", "world_default",
                      translate_marker( "Static NPCs" ),
                      translate_marker( "If true, static NPCs will spawn at pre-defined locations. Requires world reset." ),
                      COPT_NO_HIDE, false ) );

    add( bool_option( "RANDOM_NPC", "world_default",
                      translate_marker( "Random NPCs" ),
                      translate_marker( "If true, the game will randomly spawn NPCs during gameplay." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "RAD_MUTATION", "world_default",
                      translate_marker( "Mutations by radiation" ),
                      translate_marker( "If true, radiation causes the player to mutate." ),
                      COPT_NO_HIDE, true ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "ZLEVELS", "world_default",
                      translate_marker( "Experimental z-levels" ),
                      translate_marker( "If true, experimental z-level maps will be enabled.  This feature is not finished yet and turning it on will only slow the game down." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "ALIGN_STAIRS", "world_default",
                      translate_marker( "Align up and down stairs" ),
                      translate_marker( "If true, downstairs will be placed directly above upstairs, even if this results in uglier maps." ),
                      COPT_NO_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( string_map_option( "CHARACTER_POINT_POOLS", "world_default",
                            translate_marker( "Character point pools" ),
                            translate_marker( "Allowed point pools for character generation." ),
    COPT_NO_HIDE, "any", { { "any", translate_marker( "Any" ) }, { "multi_pool", translate_marker( "Multi-pool only" ) }, { "no_freeform", translate_marker( "No freeform" ) } } ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "NO_FAULTS", "world_default",
                      translate_marker( "Disables vehicle part faults." ),
                      translate_marker( "If true, disables vehicle part faults, vehicle parts will be totally reliable unless destroyed, and can only be repaired via replacement." ),
                      COPT_ALWAYS_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "FILTHY_MORALE", "world_default",
                      translate_marker( "Morale penalty for filthy clothing." ),
                      translate_marker( "If true, wearing filthy clothing will cause morale penalties." ),
                      COPT_ALWAYS_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "FILTHY_WOUNDS", "world_default",
                      translate_marker( "Infected wounds from filthy clothing." ),
                      translate_marker( "If true, getting hit in a body part covered in filthy clothing may cause infections." ),
                      COPT_ALWAYS_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "NO_VITAMINS", "world_default",
                      translate_marker( "Disables tracking vitamins in food items." ),
                      translate_marker( "If true, disables vitamin tracking and vitamin disorders." ),
                      COPT_ALWAYS_HIDE, false ) );

    mOptionsSort["world_default"]++;

    add( bool_option( "NO_NPC_FOOD", "world_default",
                      translate_marker( "Disables tracking food, thirst and ( partially ) fatigue for NPCs." ),
                      translate_marker( "If true, NPCs won't need to eat or drink and will only get tired enough to sleep, not to get penalties." ),
                      COPT_ALWAYS_HIDE, false ) );

    for (unsigned i = 0; i < vPages.size(); ++i) {
        mPageItems[i].resize(mOptionsSort[vPages[i].first]);
    }

    for( auto &elem : options ) {
        for (unsigned i = 0; i < vPages.size(); ++i) {
            const cOpt_base &opt = *elem.second;
            if( vPages[i].first == opt.getPage() && opt.getSortPos() > -1 ) {
                mPageItems[i][opt.getSortPos()] = elem.first;
                break;
            }
        }
    }

    //Sort out possible double empty lines after options are hidden
    for (unsigned i = 0; i < vPages.size(); ++i) {
        bool bLastLineEmpty = false;
        while( mPageItems[i][0].empty() ) {
            //delete empty lines at the beginning
            mPageItems[i].erase(mPageItems[i].begin());
        }

        while( mPageItems[i][mPageItems[i].size() - 1].empty() ) {
            //delete empty lines at the end
            mPageItems[i].erase(mPageItems[i].end() - 1);
        }

        for (unsigned j = mPageItems[i].size() - 1; j > 0; --j) {
            bool bThisLineEmpty = mPageItems[i][j].empty();

            if (bLastLineEmpty == true && bThisLineEmpty == true) {
                //delete empty lines in between
                mPageItems[i].erase(mPageItems[i].begin() + j);
            }

            bLastLineEmpty = bThisLineEmpty;
        }
    }
}

#ifdef TILES
// Helper method to isolate #ifdeffed tiles code.
static void refresh_tiles( bool used_tiles_changed, bool pixel_minimap_height_changed, bool ingame )
{
    if( used_tiles_changed ) {
        //try and keep SDL calls limited to source files that deal specifically with them
        try {
            tilecontext->reinit();
            tilecontext->load_tileset( get_option<std::string>( "TILES" ) );
            //g->init_ui is called when zoom is changed
            g->reset_zoom();
            if( ingame ) {
                if( g->pixel_minimap_option ) {
                    wrefresh(g->w_pixel_minimap);
                }
                g->refresh_all();
            }
            tilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the tileset failed: %s" ), err.what() );
            use_tiles = false;
        }
    } else if( ingame && g->pixel_minimap_option && pixel_minimap_height_changed ) {
        tilecontext->reinit_minimap();
        g->init_ui();
        wrefresh( g->w_pixel_minimap );
        g->refresh_all();
    }
}
#else
static void refresh_tiles( bool, bool, bool ) {
}
#endif // TILES

void draw_borders_external( const catacurses::window &w, int horizontal_level, std::map<int, bool> &mapLines, const bool world_options_only )
{
    if( !world_options_only ) {
        draw_border( w, BORDER_COLOR, _( " OPTIONS " ) );
    }
    // intersections
    mvwputch( w, horizontal_level, 0, BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w, horizontal_level, getmaxx( w ) - 1, BORDER_COLOR, LINE_XOXX ); // -|
    for( auto &mapLine : mapLines ) {
        mvwputch( w, getmaxy( w ) - 1, mapLine.first + 1, BORDER_COLOR, LINE_XXOX ); // _|_
    }
    wrefresh( w );
}

void draw_borders_internal( const catacurses::window &w, std::map<int, bool> &mapLines )
{
    for( int i = 0; i < getmaxx( w ); ++i ) {
        if( mapLines[i] ) {
            // intersection
            mvwputch( w, 0, i, BORDER_COLOR, LINE_OXXX );
        } else {
            // regular line
            mvwputch( w, 0, i, BORDER_COLOR, LINE_OXOX );
        }
    }
    wrefresh( w );
}

std::string options_manager::show(bool ingame, const bool world_options_only)
{
    // temporary alias so the code below does not need to be changed
    options_container &OPTIONS = options;
    options_container &ACTIVE_WORLD_OPTIONS = world_generator->active_world ? world_generator->active_world->WORLD_OPTIONS :
                                                ( world_options_only ? *world_options : OPTIONS );

    auto OPTIONS_OLD = OPTIONS;
    auto WOPTIONS_OLD = ACTIVE_WORLD_OPTIONS;
    if ( world_generator->active_world == NULL ) {
        ingame = false;
    }

    const int iWorldOffset = ( world_options_only ? 2 : 0 );

    const int iTooltipHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 3 - iTooltipHeight - iWorldOffset;

    const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0 ) + iWorldOffset;

    std::map<int, bool> mapLines;
    mapLines[4] = true;
    mapLines[60] = true;

    catacurses::window w_options_border = catacurses::newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY - iWorldOffset, iOffsetX);
    catacurses::window w_options_tooltip = catacurses::newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    catacurses::window w_options_header = catacurses::newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY, 1 + iOffsetX);
    catacurses::window w_options = catacurses::newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    if( world_options_only ) {
        worldfactory::draw_worldgen_tabs(w_options_border, 1);
    }

    draw_borders_external( w_options_border, iTooltipHeight + 1 + iWorldOffset, mapLines, world_options_only );
    draw_borders_internal( w_options_header, mapLines );

    int iCurrentPage = world_options_only ? iWorldOptPage : 0;
    int iLastPage = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;

    input_context ctxt("OPTIONS");
    ctxt.register_cardinal();
    ctxt.register_action("QUIT");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::stringstream sTemp;

    while(true) {
        auto &cOPTIONS = ( ( ingame || world_options_only ) && iCurrentPage == iWorldOptPage ?
                           ACTIVE_WORLD_OPTIONS : OPTIONS );

        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_options, i, j, BORDER_COLOR, LINE_XOXO);
                } else {
                    mvwputch(w_options, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_options_tooltip, i, j, c_black, ' ');
                }
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, mPageItems[iCurrentPage].size());

        // where the column with the names starts
        const size_t name_col = 5;
        // where the column with the values starts
        const size_t value_col = 62;
        // 2 for the space between name and value column, 3 for the ">> "
        const size_t name_width = value_col - name_col - 2 - 3;
        const size_t value_width = getmaxx( w_options ) - value_col;
        //Draw options
        size_t iBlankOffset = 0; // Offset when blank line is printed.
        for (int i = iStartPos; i < iStartPos + ((iContentHeight > (int)mPageItems[iCurrentPage].size()) ?
                (int)mPageItems[iCurrentPage].size() : iContentHeight); i++) {

            int line_pos; // Current line position in window.
            nc_color cLineColor = c_light_green;
            const cOpt_base &current_opt = *cOPTIONS.at( mPageItems[iCurrentPage][i] );
            bool hasPrerequisite = current_opt.hasPrerequisite();

            line_pos = i - iStartPos;

            sTemp.str("");
            sTemp << i + 1 - iBlankOffset;
            mvwprintz(w_options, line_pos, 1, c_white, sTemp.str().c_str());

            if (iCurrentLine == i) {
                mvwprintz(w_options, line_pos, name_col, c_yellow, ">> ");
            } else {
                mvwprintz(w_options, line_pos, name_col, c_yellow, "   ");
            }

            const std::string name = utf8_truncate( current_opt.getMenuText(), name_width );
            mvwprintz( w_options, line_pos, name_col + 3, hasPrerequisite ? c_white : c_light_gray, name );

            if ( !hasPrerequisite ) {
                cLineColor = c_light_gray;

            // get_legacy_value returns an untranslated representation of the
            // option value, the comparison with a raw string is fine
            } else if( current_opt.get_legacy_value() == "false" ) {
                cLineColor = c_light_red;
            }

            const std::string value = utf8_truncate( current_opt.getValueName(), value_width );
            mvwprintz(w_options, line_pos, value_col, (iCurrentLine == i) ? hilite(cLineColor) :
                      cLineColor, value );
        }

        draw_scrollbar(w_options_border, iCurrentLine, iContentHeight,
                       mPageItems[iCurrentPage].size(), iTooltipHeight + 2 + iWorldOffset, 0, BORDER_COLOR);
        wrefresh(w_options_border);

        //Draw Tabs
        if( !world_options_only ) {
            mvwprintz(w_options_header, 0, 7, c_white, "");
            for (int i = 0; i < (int)vPages.size(); i++) {
                if( mPageItems[i].empty() ) {
                    continue;
                }
                wprintz(w_options_header, c_white, "[");
                if ( ingame && i == iWorldOptPage ) {
                    wprintz(w_options_header,
                            (iCurrentPage == i) ? hilite(c_light_green) : c_light_green, _("Current world"));
                } else {
                    wprintz(w_options_header, (iCurrentPage == i) ?
                            hilite( c_light_green ) : c_light_green, "%s", _( vPages[i].second.c_str() ) );
                }
                wprintz(w_options_header, c_white, "]");
                wputch(w_options_header, BORDER_COLOR, LINE_OXOX);
            }
        }

        wrefresh(w_options_header);

        cOpt_base &current_opt = *cOPTIONS.at( mPageItems[iCurrentPage][iCurrentLine] );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
        if (mPageItems[iCurrentPage][iCurrentLine] == "TERMINAL_X") {
            int new_terminal_x, new_window_width;
            std::stringstream value_conversion(current_opt.getValueName());

            value_conversion >> new_terminal_x;
            new_window_width = projected_window_width(new_terminal_x);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           ngettext("%s #%s -- The window will be %d pixel wide with the selected value.",
                                    "%s #%s -- The window will be %d pixels wide with the selected value.",
                                    new_window_width),
                           current_opt.getTooltip().c_str(),
                           current_opt.getDefaultText().c_str(),
                           new_window_width);
        } else if (mPageItems[iCurrentPage][iCurrentLine] == "TERMINAL_Y") {
            int new_terminal_y, new_window_height;
            std::stringstream value_conversion(current_opt.getValueName());

            value_conversion >> new_terminal_y;
            new_window_height = projected_window_height(new_terminal_y);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           ngettext("%s #%s -- The window will be %d pixel tall with the selected value.",
                                    "%s #%s -- The window will be %d pixels tall with the selected value.",
                                    new_window_height),
                           current_opt.getTooltip().c_str(),
                           current_opt.getDefaultText().c_str(),
                           new_window_height);
        } else
#endif
        {
            fold_and_print(w_options_tooltip, 0, 0, 78, c_white, "%s #%s",
                           current_opt.getTooltip().c_str(),
                           current_opt.getDefaultText().c_str());
        }

        if ( iCurrentPage != iLastPage ) {
            iLastPage = iCurrentPage;
            if ( ingame && iCurrentPage == iWorldOptPage ) {
                mvwprintz( w_options_tooltip, 3, 3, c_light_red, "%s", _("Note: ") );
                wprintz(  w_options_tooltip, c_white, "%s",
                          _("Some of these options may produce unexpected results if changed."));
            }
        }
        wrefresh(w_options_tooltip);

        wrefresh(w_options);

        const std::string action = ctxt.handle_input();

        if( world_options_only && ( action == "NEXT_TAB" || action == "PREV_TAB" || action == "QUIT" ) ) {
            return action;
        }

        if ( !current_opt.hasPrerequisite() &&
            ( action == "RIGHT" || action == "LEFT" || action == "CONFIRM" ) ) {
            popup( _( "Prerequisite for this option not met!\n(%s)" ), get_options().get_option( current_opt.getPrerequisite() ).getMenuText() );
            continue;
        }

        if (action == "DOWN") {
            do {
                iCurrentLine++;
                if (iCurrentLine >= (int)mPageItems[iCurrentPage].size()) {
                    iCurrentLine = 0;
                }
            } while( cOPTIONS.count( mPageItems[iCurrentPage][iCurrentLine] ) == 0 );
        } else if (action == "UP") {
            do {
                iCurrentLine--;
                if (iCurrentLine < 0) {
                    iCurrentLine = mPageItems[iCurrentPage].size() - 1;
                }
            } while( cOPTIONS.count( mPageItems[iCurrentPage][iCurrentLine] ) == 0 );
        } else if (!mPageItems[iCurrentPage].empty() && action == "RIGHT") {
            current_opt.setNext();
        } else if (!mPageItems[iCurrentPage].empty() && action == "LEFT") {
            current_opt.setPrev();
        } else if (action == "NEXT_TAB") {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage++;
            if (iCurrentPage >= (int)vPages.size()) {
                iCurrentPage = 0;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if (action == "PREV_TAB") {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage--;
            if (iCurrentPage < 0) {
                iCurrentPage = vPages.size() - 1;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if (!mPageItems[iCurrentPage].empty() && action == "CONFIRM") {
            current_opt.setInteractive();
        } else if( action == "HELP_KEYBINDINGS" ) {
            // keybinding screen erased the internal borders of main menu, restore it:
            draw_borders_internal( w_options_header, mapLines );
        } else if (action == "QUIT") {
            break;
        }
    }

    //Look for changes
    bool options_changed = false;
    bool world_options_changed = false;
    bool lang_changed = false;
    bool used_tiles_changed = false;
    bool pixel_minimap_changed = false;
    bool sidebar_style_changed = false;

    for (auto &iter : OPTIONS_OLD) {
        if ( iter.second != OPTIONS.at(iter.first) ) {
            options_changed = true;

            if ( iter.second->getPage() == "world_default" ) {
                world_options_changed = true;
            }

            if ( iter.first == "PIXEL_MINIMAP_HEIGHT"
              || iter.first == "PIXEL_MINIMAP_RATIO"
              || iter.first == "PIXEL_MINIMAP_MODE" ) {
                pixel_minimap_changed = true;
            }

            if( iter.first == "SIDEBAR_STYLE" ) {
                sidebar_style_changed = true;
            }

            if ( iter.first == "TILES" || iter.first == "USE_TILES" ) {
                used_tiles_changed = true;

            } else if ( iter.first == "USE_LANG" ) {
                lang_changed = true;
            }
        }
    }
    for( auto &iter : WOPTIONS_OLD ) {
        if( iter.second != ACTIVE_WORLD_OPTIONS.at( iter.first ) ) {
            options_changed = true;
            world_options_changed = true;
        }
    }

    if (options_changed) {
        if(query_yn(_("Save changes?"))) {
            save();
            if( ingame && world_options_changed ) {
                world_generator->active_world->WORLD_OPTIONS = ACTIVE_WORLD_OPTIONS;
                world_generator->save_world( world_generator->active_world, false );
            }
        } else {
            used_tiles_changed = false;
            OPTIONS = OPTIONS_OLD;
            if (ingame && world_options_changed) {
                ACTIVE_WORLD_OPTIONS = WOPTIONS_OLD;
            }
        }
    }
    if( lang_changed ) {
        set_language();
    }

    if( sidebar_style_changed ) {
        if( ingame ) {
            g->toggle_sidebar_style();
        } else {
            #ifdef TILES
                tilecontext->reinit_minimap();
            #endif
            g->narrow_sidebar = !g->narrow_sidebar;
            g->init_ui();
        }
    }

    refresh_tiles( used_tiles_changed, pixel_minimap_changed, ingame );

    return "";
}

void options_manager::cOpt_base::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "info", getTooltip() );
    json.member( "default", getDefaultText( false ) );
    json.member( "name", sName );
    json.member( "value", get_legacy_value() );
    json.end_object();
}

void options_manager::serialize(JsonOut &json) const
{
    json.start_array();

    // @todo: mPageItems is const here, so we can not use its operator[], therefore the copy
    auto mPageItems = this->mPageItems;
    for( size_t j = 0; j < vPages.size(); ++j ) {
        for( auto &elem : mPageItems[j] ) {
            // Skip blanks between option groups
            // to avoid empty json entries being stored
            if( elem.empty() ) {
                continue;
            }
            const auto iter = options.find( elem );
            if( iter != options.end() ) {
                const cOpt_base &opt = *iter->second;
                //Skip hidden option because it is set by mod and should not be saved
                if( opt.hide == COPT_ALWAYS_HIDE ) {
                    continue;
                }
                json.write( opt );
            }
        }
    }

    json.end_array();
}

void options_manager::deserialize(JsonIn &jsin)
{
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject joOptions = jsin.get_object();

        const std::string name = joOptions.get_string("name");
        const std::string value = joOptions.get_string("value");

        add_retry(name, value);
        options.at( name )->set_from_legacy_value( value );
    }
}

bool options_manager::save()
{
    const auto savefile = FILENAMES["options"];

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    fov_3d = ::get_option<bool>( "FOV_3D" );

    update_music_volume();

    return write_to_file( savefile, [&]( std::ostream &fout ) {
        JsonOut jout( fout, true );
        serialize(jout);
    }, _( "options" ) );
}

void options_manager::load()
{
    const auto file = FILENAMES["options"];
    if( !read_from_file_optional_json( file, [&]( JsonIn & jsin ) {
    deserialize( jsin );
    } ) ) {
        if (load_legacy()) {
            if (save()) {
                remove_file(FILENAMES["legacy_options"]);
                remove_file(FILENAMES["legacy_options2"]);
            }
        }
    }

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    fov_3d = ::get_option<bool>( "FOV_3D" );
}

bool options_manager::load_legacy()
{
    const auto reader = [&]( std::istream &fin ) {
        std::string sLine;
        while(!fin.eof()) {
            getline(fin, sLine);

            if( !sLine.empty() && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1) {
                int iPos = sLine.find(' ');
                const std::string loadedvar = sLine.substr(0, iPos);
                const std::string loadedval = sLine.substr(iPos + 1, sLine.length());
                // option with values from post init() might get clobbered
                add_retry(loadedvar, loadedval); // stash it until update();

                options.at( loadedvar )->set_from_legacy_value( loadedval );
            }
        }
    };

    return read_from_file_optional( FILENAMES["legacy_options"], reader ) ||
           read_from_file_optional( FILENAMES["legacy_options2"], reader );
}

bool use_narrow_sidebar()
{
    return TERMY < 25 || g->narrow_sidebar;
}

bool options_manager::has_option( const std::string &name ) const
{
    return options.count( name );
}

options_manager::cOpt_base &options_manager::get_option( const std::string &name )
{
    const auto main_iter = options.find( name );
    if( main_iter == options.end() ) {
        debugmsg( "requested non-existing option %s", name.c_str() );
    }
    cOpt_base &main_opt = *main_iter->second;
    if( !world_generator || !world_generator->active_world ) {
        // Global options contains the default for new worlds, which is good enough here.
        return main_opt;
    }
    auto &wopts = world_generator->active_world->WORLD_OPTIONS;
    const auto world_iter = wopts.find( name );
    if( world_iter != wopts.end() ) {
        return *world_iter->second;
    }
    if( main_opt.getPage() != "world_default" ) {
        // Requested a non-world option, deliver it.
        return main_opt;
    }
    // May be a new option and an old world - import default from global options.
    const auto new_iter = wopts.emplace( name, main_iter->second ).first;
    return *new_iter->second;
}

options_manager::options_container options_manager::get_world_defaults() const
{
    options_container result;
    for( auto &elem : options ) {
        if( elem.second->getPage() == "world_default" ) {
            result.insert( elem );
        }
    }
    return result;
}

std::vector<std::string> options_manager::getWorldOptPageItems() const
{
    // @todo: mPageItems is const here, so we can not use its operator[], therefore the copy
    auto temp = mPageItems;
    return temp[iWorldOptPage];
}

void options_manager::cOpt_base::report_type_error( const std::string &expected ) const
{
    debugmsg( "Tried to get %s value from option %s of type %s", expected, getName(), getType() );
}
