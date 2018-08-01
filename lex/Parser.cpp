#include "Parser.h"

#include "common-clang.h"
#include "Matcher.h"
#include "CppClass.h"
#include "CppEnum.h"
#include "CppFreeFunction.h"
#include "CppVariable.h"
#include "Cursor.h"
#include "Type.h"
#include "TranslationUnit.h"
#include "Exporter.h"

#include <iostream>

/**
 * Check if given identifier (without namespace) is a reserved word.
 */
static bool is_reserved( const std::string &name )
{
    if( name.compare( 0, 2, "__" ) == 0 ) {
        return true;
    }
    if( name.length() >= 2 && name[0] == '_' && name[1] >= 'A' && name[1] <= 'Z' ) {
        return true;
    }
    return false;
}
/**
 * Check whether some identifier should be skipped without issuing a message about it
 * (e.g. because it's common or reserved).
 */
static bool skip_silently( const Cursor &cursor )
{
    // Just a declaration, skip the actual parsing as it requires a proper definition.
    if( !cursor.is_definition() ) {
        return true;
    }
    // Don't export anything that is in the standard library namespace. For now. Maybe later.
    if( cursor.fully_qualifid().is_in_namespace( Parser::cpp_standard_namespace ) ) {
        return true;
    }
    if( is_reserved( cursor.spelling() ) ) {
        return true;
    }
    // anonymous object, can't be exported because it has no name
    if( cursor.spelling().empty() ) {
        return true;
    }
    return false;
}

Parser::Parser( Exporter &e ) : exporter( e )
{
}

Parser::~Parser() = default;

void Parser::skipped( const std::string &what, const FullyQualifiedId &name, const std::string &why )
{
    const std::string id = what + "!" + name;
    // Register what things have been skipped, so we can avoid showing this
    // message again for the same thing. what+!+name is a unique id for it.
    if( skipped_entities.count( id ) == 0 ) {
        debug_message( "Skipping " + what + " " + name + " (" + why + ")" );
        skipped_entities.insert( id );
    }
}

void Parser::parse_typedef( const Cursor &cursor )
{
    exporter.register_id_typedef( cursor );
}

void Parser::parse_enum( const Cursor &cursor )
{
    if( skip_silently( cursor ) ) {
        return;
    }
    const FullyQualifiedId name = cursor.fully_qualifid();
    if( !exporter.export_enabled( cursor.type(), cursor.location_path() ) ) {
        skipped( "enum", name, "not exported" );
        return;
    }
    if( contains( name, enums ) ) {
        return;
    }
    enums.emplace_back( *this, cursor );
}

void Parser::parse_class( const Cursor &cursor )
{
    if( skip_silently( cursor ) ) {
        return;
    }
    const FullyQualifiedId name = cursor.fully_qualifid();
    if( !exporter.export_enabled( cursor.type(), cursor.location_path() ) ) {
        skipped( "class", name, "not exported" );
        return;
    }
    get_or_add_class( cursor );
}

void Parser::parse_union( const Cursor &cursor )
{
    if( skip_silently( cursor ) ) {
        return;
    }
    const FullyQualifiedId name = cursor.fully_qualifid();
    skipped( "union", name, "not supported" );
    //@todo maybe implement it?
}

void Parser::parse_function( const Cursor &cursor )
{
    if( skip_silently( cursor ) ) {
        return;
    }
    //@todo handle blocking
    CppCallable::create_multiple_from( cursor, functions );
}

void Parser::parse_namespace( const Cursor &cursor )
{
    if( is_reserved( cursor.spelling() ) || cursor.spelling().empty() ) {
        return;
    }
    if( cursor.fully_qualifid().is_in_namespace( cpp_standard_namespace ) ) {
        return;
    }
    parse( cursor );
}

void Parser::parse_variable( const Cursor &cursor )
{
    if( skip_silently( cursor ) ) {
        return;
    }
    //@todo handle blocking
    variables.emplace_back( cursor );
}

void Parser::parse( const std::vector<std::string> &headers )
{
    std::string text;
    for( const std::string &header : headers ) {
        text += "#include \"" + header + "\"\n";
    }
    std::vector<const char *> args = { {
            "-x", "c++",
            "-std=c++11",
            "-fsyntax-only",
            "-Wno-pragma-once-outside-header",
        }
    };
    for( const std::string &arg : additional_args ) {
        args.emplace_back( arg.c_str() );
    }
    args.emplace_back( "foo.h" );

    const unsigned opts = CXTranslationUnit_None | CXTranslationUnit_SkipFunctionBodies |
                          CXTranslationUnit_Incomplete;
    tus.emplace_back( index, args, text, opts );

    const auto diagnostics = tus.back().get_diagnostics();
    for( const std::string &msg : diagnostics ) {
        info_message( msg );
    }
    if( !diagnostics.empty() ) {
        throw std::runtime_error("Errors / warnings while parsing header" );
    }
    
    for( const std::string &file : tus.back().get_includes() ) {
        visited_files.insert( file );
    }

    parse( tus.back().cursor() );
}

void Parser::parse( const std::string &header )
{
    if( visited_files.count( header ) > 0 ) {
        info_message( "Skipping file " + header + " as it was already included by another file" );
        return;
    }
    std::vector<const char *> args = { {
            "-x", "c++",
            "-std=c++11",
            "-fsyntax-only",
            "-Wno-pragma-once-outside-header",
        }
    };
    for( const std::string &arg : additional_args ) {
        args.emplace_back( arg.c_str() );
    }
    args.emplace_back( header.c_str() );

    const unsigned opts = CXTranslationUnit_None | CXTranslationUnit_SkipFunctionBodies |
                          CXTranslationUnit_Incomplete;
    tus.emplace_back( index, args, opts );

    const auto diagnostics = tus.back().get_diagnostics();
    for( const std::string &msg : diagnostics ) {
        info_message( msg );
    }
    if( !diagnostics.empty() ) {
        throw std::runtime_error("Errors / warnings while parsing header" );
    }

    for( const std::string &file : tus.back().get_includes() ) {
        visited_files.insert( file );
    }

    parse( tus.back().cursor() );
}

void Parser::parse( const Cursor &c ) {
    c.visit_children( [this]( const Cursor & cursor, const Cursor &/*parent*/ ) {
        const auto k = cursor.kind();
        if( k == CXCursor_StructDecl || k == CXCursor_ClassDecl ) {
            parse_class( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_ClassTemplate ) {
            skipped( "class template", cursor.fully_qualifid(), "not supported" );
            //@todo handle this?
            return CXChildVisit_Continue;
        } else if( k == CXCursor_ClassTemplatePartialSpecialization ) {
            skipped( "class template partial specialization", cursor.fully_qualifid(), "not supported" );
            //@todo handle this?
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UnionDecl ) {
            parse_union( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_EnumDecl ) {
            parse_enum( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypedefDecl || k == CXCursor_TypeAliasDecl ||
                   k == CXCursor_UsingDeclaration ) {
            parse_typedef( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_VarDecl ) {
            parse_variable( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FunctionTemplate ) {
            skipped( "function template", cursor.fully_qualifid(), "not supported" );
            //@todo handle this?
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FunctionDecl ) {
            parse_function( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_Namespace ) {
            parse_namespace( cursor );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypeAliasTemplateDecl ) {
            // Too complicated, and probably not needed.
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UnexposedDecl ) {
            // Don't even know what this is.
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TranslationUnit ) {
            return CXChildVisit_Recurse;
        }
        cursor.dump( "main parse function" );
        return CXChildVisit_Continue;
    } );
}

void Parser::debug_message( const std::string &message ) const
{
    std::cout << message << std::endl;
}

void Parser::info_message( const std::string &message ) const
{
    std::cout << message << std::endl;
}

void Parser::error_message( const std::string &message ) const
{
    std::cerr << message << std::endl;
}

const CppClass *Parser::get_class( const FullyQualifiedId &full_name ) const
{
    return get_from( full_name, classes );
}

bool Parser::contains_class( const FullyQualifiedId &full_name ) const
{
    return contains( full_name, classes );
}

CppClass &Parser::add_class( const Cursor &c )
{
    classes.emplace_back( *this, c );
    return classes.back();
}

CppClass &Parser::get_or_add_class( const Cursor &c )
{
    CppClass *const ptr = const_cast<CppClass *>( get_from( c.fully_qualifid(), classes ) );
    return ptr ? *ptr : add_class( c );
}

const CppEnum *Parser::get_enum( const FullyQualifiedId &full_name ) const
{
    return get_from( full_name, enums );
}

bool Parser::contains_enum( const FullyQualifiedId &full_name ) const
{
    return contains( full_name, enums );
}

void Parser::add_include_path( const std::string &path )
{
    additional_args.emplace_back( "-isystem" );
    additional_args.emplace_back( path );
}

const FullyQualifiedId Parser::cpp_standard_namespace( "std" );
