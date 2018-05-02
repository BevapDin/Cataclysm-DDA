#include "calendar.h"
#include <cmath>
#include <limits>
#include <array>

#include "output.h"
#include "options.h"
#include "translations.h"
#include "string_formatter.h"
#include "json.h"
#include "debug.h"
#include "rng.h"

// Divided by 100 to prevent overflowing when converted to moves
const int calendar::INDEFINITELY_LONG( std::numeric_limits<int>::max() / 100 );

static season_type mutable_initial_season = SPRING;
const season_type &calendar::initial_season = mutable_initial_season;

static time_point mutable_start = calendar::time_of_cataclysm;
const time_point &calendar::start = mutable_start;

static time_point mutable_turn = calendar::time_of_cataclysm;
const time_point &calendar::turn = mutable_turn;

const time_point calendar::before_time_starts = time_point::from_turn( -1 );
const time_point calendar::time_of_cataclysm = time_point::from_turn( 0 );

// Internal constants, not part of the calendar interface.
// Times for sunrise, sunset at equinoxes

/** Hour of sunrise at winter solstice */
#define SUNRISE_WINTER   7

/** Hour of sunrise at fall and spring equinox */
#define SUNRISE_EQUINOX 6

/** Hour of sunrise at summer solstice */
#define SUNRISE_SUMMER   5

/** Hour of sunset at winter solstice */
#define SUNSET_WINTER   17

/** Hour of sunset at fall and spring equinox */
#define SUNSET_EQUINOX 19

/** Hour of sunset at summer solstice */
#define SUNSET_SUMMER   21

// How long, does sunrise/sunset last?
static const time_duration twilight_duration = 1_hours;

void calendar::increment_current_turn()
{
    increment_current_turn( 1_turns );
}

void calendar::increment_current_turn( const time_duration &delta )
{
    mutable_turn += delta;
}

void calendar::init( const time_duration &time_into_cataclysm, const season_type season )
{
    mutable_turn = mutable_start = time_of_cataclysm + time_into_cataclysm;
    mutable_initial_season = season;
}

void calendar::deserialize( JsonObject &data )
{
    data.read( "turn", mutable_turn );
    data.read( "calendar_start", mutable_start );
    mutable_initial_season = static_cast<season_type>( data.get_int( "initial_season", static_cast<int>( SPRING ) ) );
}

void calendar::serialize( JsonOut &jsout )
{
    jsout.member( "turn", mutable_turn );
    jsout.member( "calendar_start", mutable_start );
    jsout.member( "initial_season", static_cast<int>( initial_season ) );
}

moon_phase get_moon_phase( const time_point &p )
{
    //One full phase every 2 rl months = 2/3 season length
    const time_duration moon_phase_duration = calendar::season_length() * 2.0 / 3.0;
    //Switch moon phase at noon so it stays the same all night
    const time_duration current_day = ( p - calendar::time_of_cataclysm ) + 1_days / 2;
    const double phase_change = current_day / moon_phase_duration;
    const int current_phase = int( round( phase_change * MOON_PHASE_MAX ) ) % int(MOON_PHASE_MAX);

    return static_cast<moon_phase>( current_phase );
}

time_point sunrise( const time_point &p )
{
    const time_point midnight = p - time_past_midnight( p );
    const time_duration season_length = calendar::season_length();
    const season_type season = season_of_year( p );
    const double percent = ( time_past_new_year( p ) % season_length ) / season_length;

    // double because it is needed as double later
    static const std::array<double, 4> start_hours = { { SUNRISE_EQUINOX, SUNRISE_SUMMER, SUNRISE_EQUINOX, SUNRISE_WINTER } };
    static const std::array<double, 4> end_hours = { { SUNRISE_SUMMER, SUNRISE_EQUINOX, SUNRISE_WINTER, SUNRISE_EQUINOX } };
    const double time = start_hours[season] * ( 1.0 - percent ) + end_hours[season] * percent;

    return midnight + time_duration::from_hours( time );
}

time_point sunset( const time_point &p )
{
    const time_point midnight = p - time_past_midnight( p );
    const time_duration season_length = calendar::season_length();
    const season_type season = season_of_year( p );
    const double percent = ( time_past_new_year( p ) % season_length ) / season_length;

    // double because it is needed as double later
    static const std::array<double, 4> start_hours = { { SUNSET_EQUINOX, SUNSET_SUMMER, SUNSET_EQUINOX, SUNSET_WINTER } };
    static const std::array<double, 4> end_hours = { { SUNSET_SUMMER, SUNSET_EQUINOX, SUNSET_WINTER, SUNSET_EQUINOX } };
    const double time = start_hours[season] * ( 1.0 - percent ) + end_hours[season] * percent;

    return midnight + time_duration::from_hours( time );
}

bool is_night( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    const time_duration sunrise = time_past_midnight( ::sunrise( p ) );
    const time_duration sunset = time_past_midnight( ::sunset( p ) );

    return now > sunset + twilight_duration || now < sunrise;
}

double current_daylight_level( const time_point &p )
{
    const time_duration now = time_past_new_year( p );
    const double percent = ( now % calendar::season_length() ) / calendar::season_length();
    double modifier = 1.0;
    // For ~Boston: solstices are +/- 25% sunlight intensity from equinoxes
    static double deviation = 0.25;
    
    switch( season_of_year( p ) ) {
    case SPRING:
        modifier = 1. + (percent * deviation);
        break;
    case SUMMER:
        modifier = (1. + deviation) - (percent * deviation);
        break;
    case AUTUMN:
        modifier = 1. - (percent * deviation);
        break;
    case WINTER:
        modifier = (1. - deviation) + (percent * deviation);
        break;
    }
    
    return double(modifier * DAYLIGHT_LEVEL);
}

float sunlight( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    const time_duration sunrise = time_past_midnight( ::sunrise( p ) );
    const time_duration sunset = time_past_midnight( ::sunset( p ) );

    double daylight_level = current_daylight_level( p );

    int current_phase = static_cast<int>( get_moon_phase( p ) );
    if ( current_phase > int(MOON_PHASE_MAX)/2 ) {
        current_phase = int(MOON_PHASE_MAX) - current_phase;
    }

    int moonlight = 1 + int(current_phase * MOONLIGHT_PER_QUARTER);

    if( now > sunset + twilight_duration || now < sunrise ) { // Night
        return moonlight;
    } else if( now >= sunrise && now <= sunrise + twilight_duration ) {
        const double percent = ( now - sunrise ) / twilight_duration;
        return double(moonlight) * (1. - percent) + daylight_level * percent;
    } else if( now >= sunset && now <= sunset + twilight_duration ) {
        const double percent = ( now - sunset ) / twilight_duration;
        return daylight_level * (1. - percent) + double(moonlight) * percent;
    } else {
        return daylight_level;
    }
}

std::string to_string_clipped( const time_duration &d )
{
    //@todo: change INDEFINITELY_LONG to time_duration
    if( to_turns<int>( d ) >= calendar::INDEFINITELY_LONG ) {
        return _( "forever" );
    }

    if( d < 1_minutes ) {
        //@todo: add to_seconds,from_seconds, operator ""_seconds, but currently
        // this could be misleading as we only store turns, which are 6 whole seconds
        const int sec = to_turns<int>( d ) * 6;
        return string_format( ngettext( "%d second", "%d seconds", sec ), sec );
    } else if( d < 1_hours ) {
        const int min = to_minutes<int>( d );
        return string_format( ngettext( "%d minute", "%d minutes", min ), min );
    } else if( d < 1_days ) {
        const int hour = to_hours<int>( d );
        return string_format( ngettext( "%d hour", "%d hours", hour ), hour );
    } else if( d < calendar::season_length() || calendar::eternal_season() ) {
        // eternal seasons means one season is indistinguishable from the next,
        // therefore no way to count them
        const int day = to_days<int>( d );
        return string_format( ngettext( "%d day", "%d days", day ), day );
    } else if( d < calendar::year_length() && !calendar::eternal_season() ) {
        //@todo: consider a to_season function, but season length is variable, so
        // this might be misleading
        const int season = to_turns<int>( d ) / to_turns<int>( calendar::season_length() );
        return string_format( ngettext( "%d season", "%d seasons", season ), season );
    } else {
        //@todo: consider a to_year function, but year length is variable, so
        // this might be misleading
        const int year = to_turns<int>( d ) / to_turns<int>( calendar::year_length() );
        return string_format( ngettext( "%d year", "%d years", year ), year );
    }
}

std::string to_string( const time_duration &d )
{
    if( d < time_duration::from_turns( calendar::INDEFINITELY_LONG ) ) {
        return _( "for ever" );
    }

    if( d <= 1_minutes ) {
        return to_string_clipped( d );
    }

    time_duration divider = 0_turns;
    if( d < 1_hours ) {
        divider = 1_minutes;
    } else if( d < 1_days ) {
        divider = 1_hours;
    } else {
        divider = 24_hours;
    }

    //~ %1$s - greater units of time (e.g. 3 hours), %2$s - lesser units of time (e.g. 11 minutes).
    return string_format( _( "%1$s and %2$s" ),
                          to_string_clipped( d ),
                          to_string_clipped( d % divider ) );
}

std::string to_string_approx( const time_duration &d, const bool verbose )
{
    int turns = to_turns<int>( d );
    const auto make_result = [verbose]( int turns, const char *verbose_str, const char *short_str ) {
        return string_format( verbose ? verbose_str : short_str, to_string_clipped( time_duration::from_turns( turns ) ) );
    };

    int divider = 0;
    int vicinity = 0;

    if( turns > HOURS( 24 ) ) {
        divider = HOURS( 24 );
        vicinity = HOURS( 2 );
    } else if( turns > HOURS( 1 ) ) {
        divider = HOURS( 1 );
        vicinity = MINUTES( 5 );
    } // Minutes and seconds can be estimated precisely.

    if( divider != 0 ) {
        const int remainder = turns % divider;

        if( remainder >= divider - vicinity ) {
            turns += divider;
        } else if( remainder > vicinity ) {
            if( remainder < divider / 2 ) {
                //~ %s - time (e.g. 2 hours).
                return make_result( turns, _( "more than %s" ), ">%s" );
            } else {
                //~ %s - time (e.g. 2 hours).
                return make_result( turns + divider, _( "less than %s" ), "<%s" );
            }
        }
    }
    //~ %s - time (e.g. 2 hours).
    return make_result( turns, _( "about %s" ), "%s" );
}

std::string to_string_time_of_day( const time_point &p )
{
    const int hour = hour_of_day<int>( p );
    const int minute = minute_of_hour<int>( p );
    //@todo add a to_seconds function?
    const int second = ( to_turns<int>( time_past_midnight( p ) ) * 6 ) % 60;
    const std::string format_type = get_option<std::string>( "24_HOUR" );

    if( format_type == "military" ) {
        return string_format( "%02d%02d.%02d", hour, minute, second );
    } else if( format_type == "24h" ) {
        //~ hour:minute (24hr time display)
        return string_format( _( "%02d:%02d:%02d" ), hour, minute, second );
    } else {
        int hour_param = hour % 12;
        if( hour_param == 0 ) {
            hour_param = 12;
        }
        // Padding is removed as necessary to prevent clipping with SAFE notification in wide sidebar mode
        const std::string padding = hour_param < 10 ? " " : "";
        if( hour < 12 ) {
            return string_format( _( "%d:%02d:%02d%sAM" ), hour_param, minute, second, padding );
        } else {
            return string_format( _( "%d:%02d:%02d%sPM" ), hour_param, minute, second, padding );
        }
    }
}

weekdays day_of_week( const time_point &p )
{
    /* Design rationale:
     * <kevingranade> here's a question
     * <kevingranade> what day of the week is day 0?
     * <wito> Sunday
     * <GlyphGryph> Why does it matter?
     * <GlyphGryph> For like where people are and stuff?
     * <wito> 7 is also Sunday
     * <kevingranade> NOAA weather forecasts include day of week
     * <GlyphGryph> Also by day0 do you mean the day people start day 0
     * <GlyphGryph> Or actual day 0
     * <kevingranade> good point, turn 0
     * <GlyphGryph> So day 5
     * <wito> Oh, I thought we were talking about week day numbering in general.
     * <wito> Day 5 is a thursday, I think.
     * <wito> Nah, Day 5 feels like a thursday. :P
     * <wito> Which would put the apocalpyse on a saturday?
     * <Starfyre> must be a thursday.  I was never able to get the hang of those.
     * <ZChris13> wito: seems about right to me
     * <wito> kevingranade: add four for thursday. ;)
     * <kevingranade> sounds like consensus to me
     * <kevingranade> Thursday it is */
    const int day_since_cataclysm = to_days<int>( p - calendar::time_of_cataclysm );
    static const weekdays start_day = weekdays::THURSDAY; // Thursday is the start day
    const int result = day_since_cataclysm + static_cast<int>( start_day );
    return static_cast<weekdays>( result % 7 );
}

bool calendar::eternal_season()
{
    static const std::string eternal_season_option_name = "ETERNAL_SEASON";
    return get_option<bool>( eternal_season_option_name );
}

time_duration calendar::year_length()
{
    return season_length() * 4;
}

time_duration calendar::season_length()
{
    static const std::string s = "SEASON_LENGTH";
    // Avoid returning 0 as this value is used in division and expected to be non-zero.
    return time_duration::from_days( std::max( get_option<int>( s ), 1 ) );
}

float calendar::season_ratio()
{
    static const int real_world_season_length = 91;
    return to_days<float>( season_length() ) / real_world_season_length;
}

float calendar::season_from_default_ratio()
{
    static const int default_season_length = 14;
    return to_days<float>( season_length() ) / default_season_length;
}

bool calendar::once_every( const time_duration &event_frequency )
{
    return ( to_turn<int>( calendar::turn ) % to_turns<int>( event_frequency ) ) == 0;
}

const std::string calendar::name_season( season_type s )
{
    static const std::array<std::string, 5> season_names_untranslated = {{
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Spring" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Summer" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Autumn" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Winter" ) ),
        std::string( translate_marker( "End times" ) )
    }};
    if( s >= SPRING && s <= WINTER ) {
        return _( season_names_untranslated[ s ].c_str() );
    }

    return _( season_names_untranslated[ 4 ].c_str() );
}

time_duration rng( time_duration lo, time_duration hi )
{
    return time_duration( rng( lo.turns_, hi.turns_ ) );
}

bool x_in_y( const time_duration &a, const time_duration &b )
{
    return ::x_in_y( to_turns<int>( a ), to_turns<int>( b ) );
}

season_type season_of_year( const time_point &p )
{
    static time_point prev_turn = calendar::before_time_starts;
    static season_type prev_season = calendar::initial_season;
    
    if( p != prev_turn ) {
        prev_turn = p;
        if( calendar::eternal_season() ) {
            // If we use calendar::start to determine the initial season, and the user shortens the season length
            // mid-game, the result could be the wrong season!
            return prev_season = calendar::initial_season;
        }
        return prev_season = static_cast<season_type>( 
            to_turn<int>( p ) / to_turns<int>( calendar::season_length() ) % 4
        );
    }
    
    return prev_season;
}

std::string to_string( const time_point &p )
{
    const int year = to_turns<int>( p - calendar::time_of_cataclysm ) / to_turns<int>( calendar::year_length() ) + 1;
    const std::string time = to_string_time_of_day( p );
    if( calendar::eternal_season() ) {
        const int day = to_days<int>( time_past_new_year( p ) );
        //~ 1 is the year, 2 is the day (of the *year*), 3 is the time of the day in its usual format
        return string_format( _( "Year %1$d, day %2$d %3$s" ), year, day, time );
    } else {
        const int day = day_of_season<int>( p );
        //~ 1 is the year, 2 is the season name, 3 is the day (of the season), 4 is the time of the day in its usual format
        return string_format( _( "Year %1$d, %2$s, day %3$d %4$s" ), year,
                              calendar::name_season( season_of_year( p ) ), day, time );
    }
}
