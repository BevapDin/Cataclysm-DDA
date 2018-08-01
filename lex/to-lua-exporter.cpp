#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>

#include "Parser.h"
#include "Matcher.h"
#include "Exporter.h"
#include "json.h"

void load_config( MultiMatcher &matcher, JsonObject jsobj )
{
    for( const std::string &line : jsobj.get_string_array( "regex" ) ) {
        matcher.emplace_back<RegexMatcher>(line);
    }
    for( const std::string &line : jsobj.get_string_array( "string" ) ) {
        matcher.emplace_back<SimpleMatcher>(line);
    }
}

int main( int argc, const char *argv[] )
{
    try {
        std::vector<std::string> args( argv + 1, argv + argc );

        Exporter exporter;
        Parser parser( exporter );

        std::string source_directory = "../src/";
        std::string output_path = "generated_class_definitions.lua";
        std::string config_path = "config.json";
        bool parse_via_one_header = false;
        for( auto i = args.begin(); i != args.end(); ) {
            const std::string &arg = *i;
            if( arg == "--one-header" ) {
                parse_via_one_header = true;
            } else if( arg == "--export-comments" ) {
                exporter.export_comments = true;
            } else if( arg == "-s" && i + 1 != args.end() ) {
                i = args.erase( i );
                source_directory = *i + "/"; // may create a double slash at the end, but it'l work anyway
            } else if( arg == "--config" && i + 1 != args.end() ) {
                i = args.erase( i );
                config_path = *i;
            } else if( arg == "--include" && i + 1 != args.end() ) {
                i = args.erase( i );
                parser.add_include_path( *i );
            } else if( arg == "-o" && i + 1 != args.end() ) {
                i = args.erase( i );
                output_path = *i;
            } else if( arg == "--export-all" && i + 1 != args.end() ) {
                i = args.erase( i );
                exporter.export_all_in( *i );
            } else if( arg == "-?" || arg == "--help" ) {
                std::cout << "--config <path> ... path to the configuration JSON file\n";
                std::cout << "--include <path> ... additional include path\n";
                std::cout << "--one-header ... Put all includes into one header and parse that (should be faster)\n";
                std::cout << "--export-comments ... Export comments from C++\n";
                std::cout << "-s <dir> ... directory containing the source files\n";
                std::cout << "-o <path> ... path to the output file\n";
                std::cout << "--export-all <directory> ... Export all things declared in a file inside the given directory.\n"; 
                return 1;
            } else {
                throw std::runtime_error( "Unknown argument '" + arg + "'" );
            }
            i = args.erase( i );
        }

        std::vector<std::string> headers;
        {
            std::ifstream fstream( config_path.c_str() );
            JsonIn jsin( fstream );
            JsonObject jsobj = jsin.get_object();

            headers = jsobj.get_string_array( "headers" );
            JsonArray jarr = jsobj.get_array( "types" );
            while( jarr.has_more() ) {
                if( jarr.test_string() ) {
                    exporter.add_export( FullyQualifiedId( jarr.next_string() ) );
                } else {
                    JsonObject job = jarr.next_object();
                    exporter.add_export( FullyQualifiedId( job.get_string( "cpp_name" ) ), job.get_string( "lua_name" ) );
                }
            }
            load_config( *exporter.readonly_identifiers, jsobj.get_object( "readonly_identifiers" ) );
            load_config( *exporter.blocked_identifiers, jsobj.get_object( "blocked_identifiers" ) );
            load_config( *exporter.ignore_result_of_those, jsobj.get_object( "ignore_result_of_those" ) );
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
