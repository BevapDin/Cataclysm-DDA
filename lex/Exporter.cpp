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

bool Exporter::is_blocked( const std::string &name ) const
{
    return blocked_identifiers->match( name );
}

bool Exporter::ignore_result_of( const std::string &name ) const
{
    return ignore_result_of_those->match( name );
}

bool Exporter::is_readonly( const std::string &name ) const
{
    return is_readonly_identifiers->match( name );
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

std::string Exporter::derived_class( const Type &t ) const
{
    if( t.kind() == CXType_Typedef ) {
        return derived_class( t.get_declaration().get_underlying_type() );
    }
    if( t.kind() == CXType_Record || t.kind() == CXType_Enum ) {
        return remove_const( t.spelling() );
    }
    return remove_const( t.spelling() );
}

Exporter::Exporter() : readonly_identifiers( new MultiMatcher() ),
    blocked_identifiers( new MultiMatcher() ), ignore_result_of_those( new MultiMatcher() ),
    is_readonly_identifiers( new MultiMatcher() )
{
}

Exporter::~Exporter() = default;

std::string Exporter::translate_member_type( const Type &t ) const
{
    std::string res = build_in_lua_type( t );
    if( !res.empty() ) {
        return "\"" + res + "\"";
    }

    if( export_by_reference( t ) ) {
        return "\"" + remove_const( t.spelling() ) + "\"";
    }
    if( export_by_value( t ) ) {
        return "\"" + remove_const( t.spelling() ) + "\"";
    }

    res = const_cast<Exporter &>( *this ).register_generic( t );
    if( !res.empty() ) {
        return res;
    }

    if( t.kind() == CXType_LValueReference ) {
        const Type pt = t.get_pointee();
        // We can return a reference to the member in Lua, therefor allow types that
        // support by-reference semantic.
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( pt.spelling() ) + "\"";
        }
    }

    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( pt.spelling() ) + "\"";
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
    std::string res = build_in_lua_type( t );
    if( !res.empty() ) {
        return "\"" + res + "\"";
    }

    if( export_by_value( t ) ) {
        return "\"" + remove_const( t.spelling() ) + "\"";
    }

    res = const_cast<Exporter &>( *this ).register_generic( t );
    if( !res.empty() ) {
        return res;
    }

    if( t.kind() == CXType_LValueReference ) {
        const Type pt = t.get_pointee();
        if( pt.is_const_qualified() ) {
            //debug_message("pointee: " + pt.spelling() + " (from " + t.spelling() + ")");
            // "const int &" can be satisfied by the build-in Lua "int".
            // But it won't work correctly with "int &" as the value passed to
            // the C++ functions is a temporary.
            res = build_in_lua_type( pt );

            if( !res.empty() ) {
                return "\"" + res + "\"";
            }
        }
        // "const foo &" and "foo &" can be satisfied by an exported object.
        if( export_by_value( pt ) ) {
            return "\"" + remove_const( pt.spelling() ) + "\"";
        }
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( pt.spelling() ) + "\"";
        }
        res = const_cast<Exporter &>( *this ).register_generic( pt );
        if( !res.empty() ) {
            return res;
        }
    }

    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        // "const foo *" and "foo *" can be satisfied with by-reference values, they
        // have an overload that provides the pointer. It does not work for(  by-value objects.
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( pt.spelling() ) + "\"";
        }
        res = const_cast<Exporter &>( *this ).register_generic( pt );
        if( !res.empty() ) {
            return res;
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
    std::set<std::string> handled_types;
    std::ofstream f( lua_file.c_str() );
    f << "classes = {\n";
    for( const auto &c : types_to_export ) {
        if( const CppClass *const obj = parser.get_class( c ) ) {
            f << obj->export_( *this ) << ",\n";
            handled_types.insert( c );
        }
    }
    f << "}\n";

    f << "\n";
    f << "enums = {\n";
    for( const auto &e : types_to_export ) {
        if( const CppEnum *const obj = parser.get_enum( e ) ) {
            f << obj->export_( *this ) << ",\n";
            handled_types.insert( e );
        }
    }
    f << "}\n";

    f << "\n";
    f << "functions = {\n";
    {
        std::vector<std::string> funcs;
        for( const CppFreeFunction &func : parser.functions ) {
            const auto lines = func.export_( *this );
            funcs.insert( funcs.end(), lines.begin(), lines.end() );
        }
        std::sort( funcs.begin(), funcs.end() );
        for( const std::string &line : funcs ) {
            f << line << ",\n";
        }
    }
    f << "}\n";

    f << "\n";
    f << "variables = {\n";
    {
        std::vector<std::string> vars;
        for( const CppVariable &var : parser.variables ) {
            vars.push_back( var.export_( *this ) );
        }
        std::sort( vars.begin(), vars.end() );
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
    //@todo check for IO errors

    for( const auto &t : types_to_export ) {
        if( handled_types.count( t ) > 0 ) {
            continue;
        }
        error_message( "Type " + t + " not found in any input source file" );
    }
}

std::string Exporter::build_in_lua_type( const std::string &t ) const
{
    // Lua has build in support for strings and the bindings generator translates
    // std::string to Lua strings.
    if( t == "std::string" ) {
        return "std::string";
    }
    // If the input is a string, we can't really do anything more as we don't have
    // any information about the base type it
    return std::string();
}

std::string Exporter::build_in_lua_type( const Type &t ) const
{
    // First try the canonical type as reported by clang. This will handle all common
    // typedefs correctly.
    const Type ct = t.get_canonical_type();
    if( numeric_fixed_points.count( ct.kind() ) > 0 ) {
        return "int";
    } else if( numeric_floating_pints.count( ct.kind() ) > 0 ) {
        return "float";
    } else if( ct.kind() == CXType_Bool ) {
        return "bool";
    } else if( ct.kind() == CXType_Pointer ) {
        return std::string();
    } else if( ct.kind() == CXType_LValueReference ) {
        return std::string();
    } else if( ct.kind() == CXType_RValueReference ) {
        return std::string();
    }

    if( t.kind() == CXType_Typedef ) {
        return build_in_lua_type( t.get_declaration().get_underlying_type() );
    }

    // It's not a build-in C++ type that we can handle, so look at its actual name.
    // This removes the const because we export 'const int' and 'const std::string' the
    // same as non-const counterparts.
    // This calls the function with a string argument, and thereby handles std::string
    //@todo handle fully qualified names?
    const std::string res = build_in_lua_type( remove_const( t.spelling() ) );
    if( !res.empty() ) {
        return res;
    }

    return std::string();
}

std::string Exporter::translate_result_type( const Type &t )const
{
    std::string res = build_in_lua_type( t );
    if( !res.empty() ) {
        return "\"" + res + "\"";
    }

    if( export_by_value( t ) ) {
        return "\"" + remove_const( t.spelling() ) + "\"";
    }

    res = const_cast<Exporter &>( *this ).register_generic( t );
    if( !res.empty() ) {
        return res;
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
            res = build_in_lua_type( pt );
            if( !res.empty() ) {
                return "\"" + res + "\"";
            }

            if( export_by_value( pt ) ) {
                return "\"" + remove_const( spt ) + "\"";
            }
        }
        // const and non-const reference){
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( spt ) + "&\"";
        }

        // Generic types are exported as values and as reference
        res = const_cast<Exporter &>( *this ).register_generic( pt );
        if( !res.empty() ) {
            return res;
        }
    }
    if( t.kind() == CXType_Pointer ) {
        const Type pt = t.get_pointee();
        const std::string spt = pt.spelling();
        // "const foo *" and "foo *" can be satisfied by an by-reference object.
        if( export_by_reference( pt ) ) {
            return "\"" + remove_const( spt ) + "&\"";
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

std::string Exporter::register_std_iterator( const std::string &name, const Type &t )
{
    const std::string sp = remove_const( t.spelling() );
    std::string element_type = extract_templates( Parser::fully_qualifid( "std", name ), sp,
                               "::iterator" );
    if( element_type.empty() ) {
        if( t.kind() == CXType_Typedef ) {
            return register_std_iterator( name, t.get_declaration().get_underlying_type() );
        }
        return std::string();
    }

    if( build_in_lua_type( element_type ) == "std::string" ) {
        element_type = "std::string";
    } else if( export_by_value( element_type ) ) {
        // pass
    } else {
        debug_message( sp + " is a " + name + " based on " + element_type + ", but is not exported" );
        return std::string();
    }

    generic_types[sp] = "make_" + name + "_class(\"" + element_type + "\")";
    return "\"std::" + name + "<" + element_type + ">::iterator\"";
}

std::string Exporter::register_std_container( const std::string name, const Type &t )
{
    std::string res = register_std_iterator( name, t );
    if( !res.empty() ) {
        return res;
    }
    const std::string sp = remove_const( t.spelling() );
    std::string element_type = extract_templates( Parser::fully_qualifid( "std", name ), sp );
    if( element_type.empty() ) {
        if( t.kind() == CXType_Typedef ) {
            return register_std_container( name, t.get_declaration().get_underlying_type() );
        }
        return std::string();
    }

    if( build_in_lua_type( element_type ) == "std::string" ) {
        element_type = "std::string";
    } else if( export_by_value( element_type ) ) {
        // pass
    } else {
        debug_message( sp + " is a " + name + " based on " + element_type +
                       ", but is not exported" );
        return std::string();
    }

    debug_message( sp + " is a " + name + " based on " + element_type );
    generic_types[sp] = "make_" + name + "_class(\"" + element_type + "\")";
    return "\"std::" + name + "<" + element_type + ">\"";
}

std::string Exporter::register_generic( const Type &t )
{
    std::string res = register_std_container( "list", t );
    if( !res.empty() ) {
        return res;
    }
    res = register_std_container( "vector", t );
    if( !res.empty() ) {
        return res;
    }
    res = register_std_container( "set", t );
    if( !res.empty() ) {
        return res;
    }

    return std::string();
}

bool Exporter::export_by_value( const Type &name ) const
{
    if( export_by_value( remove_const( name.spelling() ) ) ) {
        return true;
    }
    return export_by_value( derived_class( name ) );
}

bool Exporter::export_by_reference( const Type &name ) const
{
    if( export_by_reference( remove_const( name.spelling() ) ) ) {
        return true;
    }
    return export_by_reference( derived_class( name ) );
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

std::string Exporter::escape_to_lua_string( const std::string &text ) const
{
    std::string result;
    for( const char c : text ) {
        if( c == '"' || c == '\\' ) {
            result += '\\';
        } else if( c == '\n' ) {
            result += "\\\\n";
            continue;
        }
        result += c;
    }
    return result;
}

bool Exporter::add_id_typedef( const Cursor &cursor, const std::string &id_type,
                               std::map<std::string, std::string> &ids_map )
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

    if( !export_enabled( base_type ) ) {
        // Don't add base_type to be exported, as not all (string|int)_id functions
        // may be available. This requires the user to manually enable
        // id types to be exported.
        //types_to_export_for_id_only.insert( base_type );
        debug_message( t.spelling() + " (" + typedef_name + ") is a " + id_type +
                       ", but it's not exported" );
        return false;
    }

    ids_map.emplace( typedef_name, base_type );
    // A id itself is always handled by value. It's basically a std::string/int.
    assert( valid_cpp_identifer( typedef_name ) );
    types_exported_by_value.insert( typedef_name );
    //    debug_message( "Automatically added " + typedef_name + " as " + id_type + "<" + base_type + ">" );
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

void Exporter::add_export_for_string_id( const std::string &id_name, const std::string &cpp_name )
{
    assert( valid_cpp_identifer( cpp_name ) );
    assert( valid_cpp_identifer( id_name ) );
    string_ids.emplace( id_name, cpp_name );
    types_to_export.insert( cpp_name );
    types_exported_by_value.insert( id_name );
}

void Exporter::add_export_by_value( const std::string &cpp_name )
{
    assert( valid_cpp_identifer( cpp_name ) );
    types_exported_by_value.insert( cpp_name );
    types_to_export.insert( cpp_name );
}

void Exporter::add_export_by_reference( const std::string &cpp_name )
{
    assert( valid_cpp_identifer( cpp_name ) );
    types_exported_by_reference.insert( cpp_name );
    types_to_export.insert( cpp_name );
}

void Exporter::add_export_by_value_and_reference( const std::string &cpp_name )
{
    assert( valid_cpp_identifer( cpp_name ) );
    types_exported_by_value.insert( cpp_name );
    types_exported_by_reference.insert( cpp_name );
    types_to_export.insert( cpp_name );
}

void Exporter::add_export_enumeration( const std::string &cpp_name )
{
    assert( valid_cpp_identifer( cpp_name ) );
    add_export_by_value( cpp_name );
}

std::string Exporter::get_header_for_argument( const Type &t ) const
{
    if( !build_in_lua_type( t ).empty() ) {
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

    return t.get_declaration().location_file();
}
