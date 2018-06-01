#include <stdexcept>
#include <iostream>
#include <vector>

#include "Parser.h"
#include "Matcher.h"
#include "Exporter.h"

int main( int argc, const char *argv[] )
{
    try {
        std::vector<std::string> args( argv + 1, argv + argc );

        // @todo load from configuration file or command line
        // All the headers in the src folder that will be searched for the definitions
        // of the exported types. The script will complain if a type is requested, but
        // not found.
        const std::vector<std::string> headers = { {
                "npc.h", "map.h", "game.h", "item.h",
                "creature.h", "monster.h", "character.h", "player.h",
                "enums.h", "calendar.h", "mtype.h", "itype.h", "bodypart.h",
                "ui.h", "martialarts.h", "trap.h", "field.h", "overmap.h",
                "mutation.h", "effect.h", "material.h", "start_location.h", "ammo.h",
                "monstergenerator.h", "item_stack.h", "mongroup.h", "weather_gen.h",
                "vehicle.h", "veh_type.h", "fault.h", "vitamin.h", "damage.h",
            }
        };

        Exporter exporter;
        Parser parser( exporter );

        // All the classes that should be available in Lua. See doc/LUA_SUPPORT.md for
        // the meaning of by_reference/by_value/by_value_and_reference.
        exporter.add_export_by_reference( "effect_type" );
        exporter.add_export_by_value_and_reference( "calendar" );
        exporter.add_export_by_reference( "mutation_branch" );
        exporter.add_export_by_reference( "Character" );
        exporter.add_export_by_value( "map_stack" );
        exporter.add_export_by_reference( "game" );
        exporter.add_export_by_value( "encumbrance_data" );
        exporter.add_export_by_reference( "stats" );
        exporter.add_export_by_reference( "player" );
        exporter.add_export_by_value_and_reference( "item" );
		// problematic because it's using move operations and does not allow copies
        //exporter.add_export_by_value( "item_location" );
        exporter.add_export_by_value( "point" );
        exporter.add_export_by_value( "tripoint" );
        exporter.add_export_by_reference( "uimenu" );
        exporter.add_export_by_reference( "field_entry" );
        exporter.add_export_by_reference( "field" );
        exporter.add_export_by_reference( "map" );
        exporter.add_export_by_reference( "ter_t" );
        exporter.add_export_by_reference( "furn_t" );
        exporter.add_export_by_reference( "Creature" );
        exporter.add_export_by_reference( "monster" );
        exporter.add_export_by_reference( "martialart" );
        exporter.add_export_by_reference( "material_type" );
        exporter.add_export_by_reference( "start_location" );
        exporter.add_export_by_reference( "ma_buff" );
        exporter.add_export_by_reference( "ma_technique" );
        exporter.add_export_by_reference( "Skill" );
        exporter.add_export_by_reference( "quality" );
        //exporter.add_export_for_string_id( "npc_template_id", "npc_template" );
        //exporter.add_export_for_string_id( "species_id", "species_type" );
        exporter.add_export_by_reference( "ammunition_type" );
        exporter.add_export_by_reference( "MonsterGroup" );
        exporter.add_export_by_reference( "mtype" );
        exporter.add_export_by_reference( "mongroup" );
        exporter.add_export_by_reference( "overmap" );
        exporter.add_export_by_value( "units::volume" );
        exporter.add_export_by_value( "units::mass" );
        exporter.add_export_by_value( "nc_color" );
        exporter.add_export_by_value( "time_duration" );
        exporter.add_export_by_value( "time_point" );
        exporter.add_export_by_reference( "itype" );
        exporter.add_export_by_reference( "trap" );
        exporter.add_export_by_reference( "w_point" );
        exporter.add_export_by_reference( "vehicle" );
        exporter.add_export_by_value_and_reference( "vehicle_part" );
        exporter.add_export_by_value( "vehicle_stack" );
        exporter.add_export_by_reference( "vpart_info" );
        exporter.add_export_by_reference( "fault" );
        exporter.add_export_by_reference( "effect" );
        exporter.add_export_by_reference( "vitamin" );
        exporter.add_export_by_reference( "npc" );

        // Enums that should be available in Lua.
        exporter.add_export_enumeration( "body_part" );
        exporter.add_export_enumeration( "hp_part" );
        exporter.add_export_enumeration( "side" );
        exporter.add_export_enumeration( "phase_id" );
        exporter.add_export_enumeration( "m_size" );
        //exporter.add_export_enumeration( "game_message_type" );
        exporter.add_export_enumeration( "season_type" );
        exporter.add_export_enumeration( "add_type" );
        exporter.add_export_enumeration( "field_id" );
        exporter.add_export_enumeration( "damage_type" );

        // Data members are checked against `readonly_identifiers` to see whether
        // they should be writable from Lua. Const members are always non-writable.

        // Not writable to avoid messing things up (can be changed via JSON):
        exporter.readonly_identifiers->emplace_back<RegexMatcher>(".*::id$");
        exporter.readonly_identifiers->emplace_back<RegexMatcher>(".*::loadid$");


        // One can block specific members from being exported:
        // The given string or regex is matched against the C++ name of the member.
        // This includes the parameter list (only type names) for function.

        // Exported to Lua via a global variables:
        exporter.blocked_identifiers->emplace_back<SimpleMatcher>( "game::m" );
        exporter.blocked_identifiers->emplace_back<SimpleMatcher>( "game::u" );
        // Never exported, internal usage only:
        exporter.blocked_identifiers->emplace_back<RegexMatcher>(".*::was_loaded");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>(".*::active_items");
        // Stack objects are created by the containing class and not by anyone else.
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("map_stack::map_stack\\(.*\\)");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("vehicle_stack::vehicle_stack\\(.*\\)");
        // Those are used during loading from JSON and should not be used any other time.
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("static .*::load\\(.*JsonObject.*\\)");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("static .*::load_.*\\(.*JsonObject.*\\)");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("static .*::reset\\(\\)");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("static .*::check_consistency\\(\\)");
        exporter.blocked_identifiers->emplace_back<RegexMatcher>("static .*::finalize\\(\\)");


        // Some functions can be used even if the return type can not be exported (their
        // side effects are still useful). Functions listed here will be exported as returning
        // "void", if their actual result type can not be exported directly.
        exporter.ignore_result_of_those->emplace_back<RegexMatcher>("game::explosion\\(.*\\)");
        exporter.ignore_result_of_those->emplace_back<RegexMatcher>("map::put_items_from_loc\\(.*\\)");

        std::string source_directory = "../src/";
        std::string output_path = "generated_class_definitions.lua";
        bool parse_via_one_header = false;
        for( auto i = args.begin(); i != args.end(); ) {
            const std::string &arg = *i;
            if( arg == "--export-comments" ) {
                exporter.export_comments = true;
            } else if( arg == "--one-header" ) {
                parse_via_one_header = true;
            } else if( arg == "-s" && i + 1 != args.end() ) {
                i = args.erase( i );
                source_directory = *i + "/"; // may create a double slash at the end, but it'l work anyway
            } else if( arg == "-o" && i + 1 != args.end() ) {
                i = args.erase( i );
                output_path = *i;
            } else if( arg == "-?" || arg == "--help" ) {
                std::cout << "--export-comments ... Export comments from C++\n";
                std::cout << "--one-header ... Put all includes into one header and parse that (should be faster)\n";
                std::cout << "-s <dir> ... directory containing the source files\n";
                std::cout << "-o <path> ... path to the output file\n";
                return 1;
            } else {
                throw std::runtime_error( "Unknown argument '" + arg + "'" );
            }
            i = args.erase( i );
        }

        if( parse_via_one_header ) {
            auto copy = headers;
            for( std::string &header : copy ) {
                header = source_directory + header;
            }
            copy.push_back("foo.h");
            parser.parse(copy);
        } else {
            for( const std::string &header : headers ) {
                parser.parse( source_directory + header );
            }
        }

        exporter.export_(parser, output_path );
        return 0;
    } catch( const std::exception &err ) {
        std::cerr << "Error: " << err.what() << "\n";
        return 1;
    }
}
