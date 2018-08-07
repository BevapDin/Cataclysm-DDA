#include "Exporter.h"

#include "common-clang.h"
#include "Matcher.h"
#include "exceptions.h"
#include "CppClass.h"
#include "CppEnum.h"
#include "CppFreeFunction.h"
#include "CppVariable.h"
#include "Cursor.h"
#include "Type.h"
#include "TranslationUnit.h"
#include "Parser.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cassert>

bool valid_cpp_identifer( const std::string &ident )
{
    if( ident.empty() ) {
        return false;
    }
    for( size_t i = 0; i < ident.length(); ++i ) {
        const char c = ident[i];
        if( i != 0 && c >= '0' && c <= '9' ) {
            continue;
        }
        if( c >= 'a' && c <= 'z' ) {
            continue;
        }
        if( c >= 'A' && c <= 'Z' ) {
            continue;
        }
        if( c == '_' ) {
            continue;
        }
        if( c == ':' && i + 1 < ident.length() && ident[i + 1] == ':' ) {
            ++i;
            continue;
        }
        return false;
    }
    return true;
}

static std::string remove_const( const std::string &name )
{
    if( name.compare( 0, 6, "const " ) == 0 ) {
        return name.substr( 6 );
    } else {
        return name;
    }
}

static const std::set<int> numeric_fixed_points = { {
        CXType_Char_U, CXType_UChar, CXType_Char16,
        CXType_Char32, CXType_UShort, CXType_UInt,
        CXType_ULong, CXType_ULongLong, CXType_UInt128,
        CXType_Char_S, CXType_SChar, CXType_WChar,
        CXType_Short, CXType_Int, CXType_Long,
        CXType_LongLong, CXType_Int128,
    }
};
static const std::set<int> numeric_floating_pints = { {
        CXType_Float, CXType_Double, CXType_LongDouble,
    }
};

bool Exporter::ignore_result_of( const FullyQualifiedId &name ) const
{
    return ignore_result_of_those->match( name.as_string() );
}

bool Exporter::is_readonly( const FullyQualifiedId &name ) const
{
    return readonly_identifiers->match( name.as_string() );
}

std::string Exporter::translate_identifier( const std::string &name ) const
{
    static const std::map<std::string, std::string> function_name_translation_table = { {
            {{"begin"}, {"cppbegin"}},
            {{"end"}, {"cppend"}},
        }
    };
    const auto iter = function_name_translation_table.find( name );
    return iter != function_name_translation_table.end() ?  iter->second : name;
}

FullyQualifiedId Exporter::derived_class( const Type &t ) const
{
    if( t.kind() == CXType_Typedef ) {
        return derived_class( t.get_declaration().get_underlying_type() );
    }
    if( t.kind() == CXType_Record || t.kind() == CXType_Enum ) {
        return FullyQualifiedId( remove_const( t.spelling() ) );
    }
    return FullyQualifiedId( remove_const( t.spelling() ) );
}

Exporter::Exporter() : readonly_identifiers( new MultiMatcher() ),
    blocked_identifiers( new MultiMatcher() ), ignore_result_of_those( new MultiMatcher() )
{
}

Exporter::~Exporter() = default;

std::string Exporter::translate_member_type( const Type &t ) const
{
    if( const auto res = build_in_lua_type( t ) ) {
        return "\"" + *res + "\"";
    }

    if( export_enabled( t ) ) {
        return "\"" + lua_name( FullyQualifiedId( remove_const( t.spelling() ) ) ) + "\"";
    }

    if( const auto res = const_cast<Exporter &>( *this ).register_generic( t ) ) {
        return *res;
    }

    if( t.kind() == CXType_LValueReference ) {
        const Type pt = t.get_pointee();
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( pt.spelling() ) ) ) + "\"";
        }
    }

    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( pt.spelling() ) ) ) + "\"";
        }
    }

    if( t.kind() == CXType_Typedef ) {
        return translate_member_type( t.get_declaration().get_underlying_type() );
    }

    const Type ct = t.get_canonical_type();
    throw TypeTranslationError( "unhandled type " + t.spelling() + "[" + str(
                                    t.kind() ) + "] as member (" + ct.spelling() + "[" + str( ct.kind() ) + "])" );
}

std::string Exporter::translate_argument_type( const Type &t ) const
{
    if( const auto res = build_in_lua_type( t ) ) {
        return "\"" + *res + "\"";
    }

    if( export_enabled( t ) ) {
        return "\"" + lua_name( FullyQualifiedId( remove_const( t.spelling() ) ) ) + "\"";
    }

    if( const auto res = const_cast<Exporter &>( *this ).register_generic( t ) ) {
        return *res;
    }

    if( t.kind() == CXType_LValueReference ) {
        const Type pt = t.get_pointee();
        if( pt.is_const_qualified() ) {
            //debug_message("pointee: " + pt.spelling() + " (from " + t.spelling() + ")");
            // "const int &" can be satisfied by the build-in Lua "int".
            // But it won't work correctly with "int &" as the value passed to
            // the C++ functions is a temporary.
            if( const auto res = build_in_lua_type( pt ) ) {
                return "\"" + *res + "\"";
            }
        }
        // Can't forward a *non-const* reference to a build-in Lua type (e.g. Lua number)
        // to a C++ function, example: "int &", "std::string &"
        if( build_in_lua_type( pt ) ) {
            throw TypeTranslationError( "type " + t.spelling() + " (a non-const reference to a build-in value) as argument" );
        }
        // "const foo &" and "foo &" can be satisfied by an exported object.
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( pt.spelling() ) ) ) + "\"";
        }
        debug_message( "X1: " + pt.spelling() + "  " + derived_class( pt ) );
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( pt.spelling() ) ) ) + "\"";
        }
        debug_message( "X2: " + pt.spelling() + "  " + derived_class( pt ) );
        if( const auto res = const_cast<Exporter &>( *this ).register_generic( pt ) ) {
            return *res;
        }
        debug_message( "Could not choose how to translate L-value-reference " + pt.spelling() );
    }

    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( pt.spelling() ) ) ) + "\"";
        }
        if( const auto res = const_cast<Exporter &>( *this ).register_generic( pt ) ) {
            return *res;
        }
    }
    if( t.kind() == CXType_Typedef ) {
        return translate_argument_type( t.get_declaration().get_underlying_type() );
    }

    const Type ct = t.get_canonical_type();
    throw TypeTranslationError( "unhandled type " + t.spelling() + "[" + str(
                                    t.kind() ) + "] as argument (" + ct.spelling() + "[" + str( ct.kind() ) + "])" );
}

void Exporter::export_( const Parser &parser, const std::string &lua_file )
{
    std::set<FullyQualifiedId> handled_types;
    std::ofstream f( lua_file.c_str() );
    for( const auto &e : types_to_export ) {
        const FullyQualifiedId &t = e.first;
        if( const CppClass *const obj = parser.get_class( t ) ) {
            f << obj->export_( *this ) << "\n";
            handled_types.insert( t );
        }
    }

    f << "\n";
    for( const auto &e : types_to_export ) {
        if( const CppEnum *const obj = parser.get_enum( e.first ) ) {
            f << obj->export_( *this ) << "\n";
            handled_types.insert( e.first );
        }
    }

    f << "\n";
    f << "global_functions = {\n";
    {
        std::set<std::string> funcs;
        for( const CppFreeFunction &func : parser.functions ) {
            try {
                funcs.insert( func.export_( *this ) );
            } catch( const TypeTranslationError &e ) {
                funcs.insert( "-- " + func.full_name() + " ignored because: " + e.what() );
            } catch( const SkippedObjectError &e ) {
                funcs.insert( "-- " + func.full_name() + " ignored because: " + e.what() );
            }
        }
        for( const std::string &line : funcs ) {
            f << line << ",\n";
        }
    }
    f << "}\n";

    f << "\n";
    f << "variables = {\n";
    {
        std::set<std::string> vars;
        for( const CppVariable &var : parser.variables ) {
            try {
                vars.insert( var.export_( *this ) );
            } catch( const TypeTranslationError &e ) {
                vars.insert( "-- " + var.full_name() + " ignored because: " + e.what() );
            } catch( const SkippedObjectError &e ) {
                vars.insert( "-- " + var.full_name() + " ignored because: " + e.what() );
            }
        }
        for( const std::string &line : vars ) {
            f << line << ",\n";
        }
    }
    f << "}\n";

    f << "\n";
    std::set<std::string> tmp;
    for( const auto &e : generic_types ) {
        tmp.insert( e.second );
    }
    for( const auto &e : tmp ) {
        f << e << "\n";
    }
    f.close();
    if( !f ) {
        throw std::runtime_error( "writing to " + lua_file + " failed" );
    }

    for( const auto &e : types_to_export ) {
        const FullyQualifiedId &t = e.first;
        if( handled_types.count( t ) > 0 ) {
            continue;
        }
        error_message( "Type " + t + " not found in any input source file" );
    }
}

cata::optional<std::string> Exporter::build_in_lua_type( const std::string &t ) const
{
    // Lua has build in support for strings and the bindings generator translates
    // "string" to Lua strings.
    if( t == "std::string" ) {
        return std::string( "string" );
    }
    // If the input is a string, we can't really do anything more as we don't have
    // any information about the base type it
    return {};
}

cata::optional<std::string> Exporter::build_in_lua_type( const Type &t ) const
{
    // First try the canonical type as reported by clang. This will handle all common
    // typedefs correctly.
    const Type ct = t.get_canonical_type();
    if( numeric_fixed_points.count( ct.kind() ) > 0 ) {
        return std::string( "int" );
    } else if( numeric_floating_pints.count( ct.kind() ) > 0 ) {
        return std::string( "float" );
    } else if( ct.kind() == CXType_Bool ) {
        return std::string( "bool" );
    } else if( ct.kind() == CXType_Pointer ) {
        return {};
    } else if( ct.kind() == CXType_LValueReference ) {
        return {};
    } else if( ct.kind() == CXType_RValueReference ) {
        return {};
    }

    if( t.kind() == CXType_Typedef ) {
        return build_in_lua_type( t.get_declaration().get_underlying_type() );
    }

    // It's not a build-in C++ type that we can handle, so look at its actual name.
    // This removes the const because we export 'const int' and 'const std::string' the
    // same as non-const counterparts.
    // This calls the function with a string argument, and thereby handles std::string
    //@todo handle fully qualified names?
    if( const auto res = build_in_lua_type( remove_const( t.spelling() ) ) ) {
        return res;
    }

    return {};
}

std::string Exporter::translate_result_type( const Type &t )const
{
    if( const auto res = build_in_lua_type( t ) ) {
        return "\"" + *res + "\"";
    }

    if( export_enabled( t ) ) {
        return "\"" + lua_name( FullyQualifiedId( remove_const( t.spelling() ) ) ) + "\"";
    }

    if( const auto res = const_cast<Exporter &>( *this ).register_generic( t ) ) {
        return *res;
    }

    // Only allowed as result type, therefor hard coded here.
    if( t.get_canonical_type().kind() == CXType_Void ) {
        return "nil";
    }

    if( t.kind() == CXType_LValueReference ) {
        const Type pt = t.get_pointee();
        const std::string spt = pt.spelling();
        if( pt.is_const_qualified() ) {
            // A const reference. Export it like a value (which means Lua will get a copy)
            if( const auto res = build_in_lua_type( pt ) ) {
                return "\"" + *res + "\"";
            }
        }
        // const and non-const reference){
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( spt ) ) ) + "&\"";
        }

        // Generic types are exported as values and as reference
        if( const auto res = const_cast<Exporter &>( *this ).register_generic( pt ) ) {
            return *res;
        }
    }
    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        const std::string spt = pt.spelling();
        // "const foo *" and "foo *" can be satisfied by an by-reference object.
        if( export_enabled( pt ) ) {
            return "\"" + lua_name( FullyQualifiedId( remove_const( spt ) ) ) + "&\"";
        }
    }

    if( t.kind() == CXType_Typedef ) {
        return translate_result_type( t.get_declaration().get_underlying_type() );
    }

    const Type ct = t.get_canonical_type();
    throw TypeTranslationError( "unhandled type " + t.spelling() + "[" + str(
                                    t.kind() ) + "] as result (" + ct.spelling() + "[" + str( ct.kind() ) + "])" );
}

std::string trim( const std::string &text );
bool is_valid_in_identifier( const char c );

std::string extract_templates( const std::string &template_name, const std::string &name )
{
    if( name.compare( 0, template_name.length(), template_name ) != 0 ) {
        return std::string();
    }
    const std::string n = trim( name.substr( template_name.length() ) );
    if( n.empty() || n.front() != '<' || n.back() != '>' ) {
        return std::string();
    }
    return trim( n.substr( 1, n.length()  - 2 ) );
}

std::string extract_templates( const std::string &template_name, const std::string &name,
                               const std::string &postfix )
{
    if( name.length() <= postfix.length() ) {
        return std::string();
    }
    if( name.compare( name.length() - postfix.length(), postfix.length(), postfix ) != 0 ) {
        return std::string();
    }
    return extract_templates( template_name, name.substr( 0, name.length() - postfix.length() ) );
}

cata::optional<std::string> Exporter::register_std_iterator( const std::string &name, const Type &t )
{
    const std::string sp = remove_const( t.spelling() );
    std::string element_type = extract_templates( FullyQualifiedId( Parser::cpp_standard_namespace, name ).as_string(), sp, "::iterator" );
    if( element_type.empty() ) {
        if( t.kind() == CXType_Typedef ) {
            return register_std_iterator( name, t.get_declaration().get_underlying_type() );
        }
        return {};
    }

    if( build_in_lua_type( element_type ).value_or( "" ) == "std::string" ) {
        element_type = "std::string";
    } else if( export_enabled( FullyQualifiedId( element_type ) ) ) {
        // pass
    } else {
        debug_message( sp + " is a " + name + " based on " + element_type + ", but is not exported" );
        return {};
    }

    generic_types[sp] = "make_std_iterator_class(\"" + element_type + "\")";
    return "\"std::" + name + "<" + element_type + ">::iterator\"";
}

cata::optional<std::string> Exporter::register_std_container( const std::string name, const Type &t )
{
    if( const auto res = register_std_iterator( name, t ) ) {
        return res;
    }
    const std::string sp = remove_const( t.spelling() );
    std::string element_type = extract_templates( FullyQualifiedId( Parser::cpp_standard_namespace, name ).as_string(), sp );
    if( element_type.empty() ) {
        if( t.kind() == CXType_Typedef ) {
            return register_std_container( name, t.get_declaration().get_underlying_type() );
        }
        return {};
    }

    if( build_in_lua_type( element_type ).value_or( "" ) == "std::string" ) {
        element_type = "std::string";
    } else if( export_enabled( FullyQualifiedId( element_type ) ) ) {
        // pass
    } else {
        debug_message( sp + " is a " + name + " based on " + element_type +
                       ", but is not exported" );
        return {};
    }

    debug_message( sp + " is a " + name + " based on " + element_type );
    generic_types[sp] = "make_std_" + name + "_class(\"" + element_type + "\")";
    return "\"std::" + name + "<" + element_type + ">\"";
}

cata::optional<std::string> Exporter::register_generic( const Type &t )
{
    if( const auto res = register_std_container( "list", t ) ) {
        return res;
    }
    if( const auto res = register_std_container( "vector", t ) ) {
        return res;
    }
    if( const auto res = register_std_container( "set", t ) ) {
        return res;
    }
    // @todo add more
    return {};
}

bool Exporter::export_enabled( const FullyQualifiedId name ) const
{
    return types_to_export.count( name ) > 0;
}
bool Exporter::export_enabled( const Type &name, const std::string &path ) const
{
    if( export_all_in_path_ && path.compare( 0, export_all_in_path_->length(), *export_all_in_path_ ) == 0 ) {
        return true;
    }
    return export_enabled( derived_class( name ) );
}

bool Exporter::export_enabled( const Type &name ) const
{
    return export_enabled( name, name.get_declaration().location_path() );
}

void Exporter::debug_message( const std::string &message ) const
{
    std::cout << message << std::endl;
}

void Exporter::info_message( const std::string &message ) const
{
    std::cout << message << std::endl;
}

void Exporter::error_message( const std::string &message ) const
{
    std::cerr << message << std::endl;
}

std::string Exporter::escape_to_lua_string( const std::string &text )
{
    std::string result;
    for( const char c : text ) {
        if( c == '"' || c == '\\' ) {
            result += '\\';
        } else if( c == '\n' ) {
            result += "\\n";
            continue;
        }
        result += c;
    }
    return result;
}

bool Exporter::add_id_typedef( const Cursor &cursor, const std::string &id_type,
                               std::map<std::string, FullyQualifiedId> &ids_map )
{
    const Type t = cursor.get_underlying_type();
    const std::string base_type = remove_const( extract_templates( id_type,
                                  remove_const( t.spelling() ) ) );
    if( base_type.empty() ) {
        return false;
    }

    const std::string typedef_name = remove_const( cursor.spelling() );
    if( ids_map.count( typedef_name ) > 0 ) {
        return true;
    }

    if( !export_enabled( FullyQualifiedId( base_type ) ) ) {
        // Don't add base_type to be exported, as not all (string|int)_id functions
        // may be available. This requires the user to manually enable
        // id types to be exported.
        //types_to_export_for_id_only.insert( base_type );
        debug_message( t.spelling() + " (" + typedef_name + ") is a " + id_type +
                       ", but it's not exported" );
        return false;
    }

    ids_map.emplace( typedef_name, FullyQualifiedId( base_type ) );
    // A id itself is always handled by value. It's basically a std::string/int.
    assert( valid_cpp_identifer( typedef_name ) );
    types_to_export.emplace( FullyQualifiedId( typedef_name ), typedef_name );
    types_to_export.emplace( FullyQualifiedId( id_type + "<" + base_type + ">" ), id_type + "<" + base_type + ">" );
    debug_message( "Automatically added " + typedef_name + " as " + id_type + "<" + base_type + ">" );
    return true;
}

bool Exporter::register_id_typedef( const Cursor &cursor )
{
    // string_id and int_id typedefs are handled separately so we can include the typedef
    // name in the definition of the class. This allows Lua code to use the typedef name
    // instead of the underlying type `string_id<T>`.
    if( add_id_typedef( cursor, "string_id", string_ids ) ) {
        return true;
    }
    if( add_id_typedef( cursor, "int_id", int_ids ) ) {
        return true;
    }
    return false;
}

void Exporter::add_export_for_string_id( const std::string &id_name, const FullyQualifiedId &full_name )
{
    assert( valid_cpp_identifer( id_name ) );
    string_ids.emplace( id_name, full_name );
    types_to_export.emplace( full_name, full_name.as_string() );
}

void Exporter::add_export( const FullyQualifiedId &full_name )
{
    add_export( full_name, full_name.as_string() );
}

void Exporter::add_export( const FullyQualifiedId &full_name, const std::string &lua_name )
{
    types_to_export.emplace( full_name, lua_name );
}

cata::optional<std::string> Exporter::get_header_for_argument( const Type &t ) const
{
    if( build_in_lua_type( t ) ) {
        // build in type is either a build in in C++ as well (e.g. numeric) or
        // it's string, and we assume the header for string is always included.
        return {};
    }
    if( t.kind() == CXType_LValueReference || t.kind() == CXType_Pointer ) {
        // reference or pointer, both just need a forward declaration, which has
        // already been done by the one declaring the function.
        return {};
    }
    if( t.kind() == CXType_Typedef ) {
        return get_header_for_argument( t.get_declaration().get_underlying_type() );
    }
    if( t.kind() == CXType_Enum ) {
        // forward declaration is enough
        return {};
    }
    if( t.kind() == CXType_Void ) {
        return {};
    }
    if( t.spelling().compare( 0, 12, "std::vector<" ) == 0 ) {
        return std::string( "#include <vector>" );
    }
    if( t.spelling().compare( 0, 10, "std::list<" ) == 0 ) {
        return std::string( "#include <list>" );
    }
    if( t.spelling().compare( 0, 9, "std::set<" ) == 0 ) {
        return std::string( "#include <set>" );
    }
    const std::string header = t.get_declaration().location_file();
    if( !header.empty() ) {
        debug_message( "Type " + t.spelling() + " has header " + header );
        return "#include \"" + header + "\"";
    }
    debug_message( "Type " + t.spelling() + " has empty source file" );
    return {};
}

std::string Exporter::lua_name( const FullyQualifiedId &full_name ) const
{
    const auto iter = types_to_export.find( full_name );
    if( iter != types_to_export.end() ) {
        return iter->second;
    }
    return translate_identifier( full_name.back() );
}

void Exporter::export_all_in( const std::string &path )
{
    export_all_in_path_ = path;
}
