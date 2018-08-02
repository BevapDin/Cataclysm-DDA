#include <stdexcept>
#include <iostream>
#include <vector>

#include "Parser.h"
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
                "creature.h", "monster.h", "character.h", "player.h", "effect.h",
                "enums.h", "calendar.h", "mtype.h", "itype.h", "bodypart.h",
                "ui.h", "martialarts.h", "trap.h", "field.h", "overmap.h",
                "mutation.h", "material.h", "start_location.h", "ammo.h",
                "monstergenerator.h", "item_stack.h", "mongroup.h", "weather_gen.h",
                 "damage.h", "skill.h", "recipe.h", "requirements.h", "gun_mode.h",
            }
        };

        Exporter exporter;
        Parser parser( exporter );

        // All the classes that should be available in Lua. See doc/LUA_SUPPORT.md for
        // the meaning of by_reference/by_value/by_value_and_reference.
        exporter.add_export_by_reference( FullyQualifiedId( "effect_type" ) );
        exporter.add_export_by_value_and_reference( FullyQualifiedId( "calendar" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "mutation_branch" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "Character" ) );
        exporter.add_export_by_value( FullyQualifiedId( "map_stack" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "game" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "start_location" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "species_type" ) );
        exporter.add_export_by_value( FullyQualifiedId( "encumbrance_data" ) );
        exporter.add_export_by_value( FullyQualifiedId( "item_stack_iterator" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "stats" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "player" ) );
        exporter.add_export_by_value_and_reference( FullyQualifiedId( "item" ) );
        // problematic because it's using move operations and does not allow copies
        //exporter.add_export_by_value( FullyQualifiedId( "item_location" ) );
        exporter.add_export_by_value( FullyQualifiedId( "point" ) );
        exporter.add_export_by_value( FullyQualifiedId( "tripoint" ) );
        exporter.add_export_by_value( FullyQualifiedId( "uimenu" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "field_entry" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "field" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "map" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "ter_t" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "furn_t" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "Creature" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "monster" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "martialart" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "material_type" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "start_location" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "ma_buff" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "ma_technique" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "Skill" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "quality" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "npc_template" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "ammunition_type" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "MonsterGroup" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "mtype" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "mongroup" ) );
        exporter.add_export_by_value_and_reference( FullyQualifiedId( "overmap" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "recipe" ) );
        exporter.add_export_by_value( FullyQualifiedId( "units::volume" ), "volume" );
        exporter.add_export_by_value( FullyQualifiedId( "units::mass" ), "mass" );
        exporter.add_export_by_value( FullyQualifiedId( "nc_color" ) );
        exporter.add_export_by_value( FullyQualifiedId( "time_duration" ) );
        exporter.add_export_by_value( FullyQualifiedId( "std::list<item>::iterator" ), "item_stack_iterator" );
        exporter.add_export_by_value( FullyQualifiedId( "time_point" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "itype" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "trap" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "w_point" ) );
        exporter.add_export_by_reference( FullyQualifiedId( "gun_mode" ) );

        // Enums that should be available in Lua.
        exporter.add_export_enumeration( FullyQualifiedId( "body_part" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "hp_part" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "side" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "phase_id" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "m_size" ) );
        //exporter.add_export_enumeration( FullyQualifiedId( "game_message_type" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "season_type" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "add_type" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "field_id" ) );
        exporter.add_export_enumeration( FullyQualifiedId( "damage_type" ) );

        std::string source_directory = "../src/";
        std::string output_path = "generated_class_definitions.lua";
        bool parse_via_one_header = false;
        for( auto i = args.begin(); i != args.end(); ) {
            const std::string &arg = *i;
            if( arg == "--one-header" ) {
                parse_via_one_header = true;
            } else if( arg == "-s" && i + 1 != args.end() ) {
                i = args.erase( i );
                source_directory = *i + "/"; // may create a double slash at the end, but it'l work anyway
            } else if( arg == "-o" && i + 1 != args.end() ) {
                i = args.erase( i );
                output_path = *i;
            } else if( arg == "-?" || arg == "--help" ) {
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

        exporter.export_( parser, output_path );
        return 0;
    } catch( const std::exception &err ) {
        std::cerr << "Error: " << err.what() << "\n";
        return 1;
    }
}
