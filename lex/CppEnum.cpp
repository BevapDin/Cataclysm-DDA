#include "CppEnum.h"

#include "FullyQualifiedId.h"
#include "Parser.h"
#include "Exporter.h"
#include "TokenRange.h"

CppEnum::CppEnum( Parser &/*p*/, const Cursor &cursor ) : cursor_( cursor )
{
    cursor.visit_children( [this]( const Cursor & c, const Cursor &/*parent*/ ) {
        if( c.kind() == CXCursor_EnumConstantDecl ) {
            values.emplace_back( c.spelling() );
            return CXChildVisit_Continue;
        }
        c.dump( "Cursor kind not handled upon parsing enumeration definition" );
        return CXChildVisit_Continue;
    } );
}

cata::optional<std::string> CppEnum::underlying_cpp_type() const
{
    // clang does not expose this directly
    bool found_colon = false;
    for( const auto &t : cursor_.tokens() ) {
        if( t.spelling() == ":" ) {
            found_colon = true;
        } else if( t.spelling() == "{" ) {
            break;
        } else if( found_colon ) {
            return t.spelling();
        }
    }
    return {};
}

bool CppEnum::is_enum_class() const
{
    // clang does not expose this directly
    for( const auto &t : cursor_.tokens() ) {
        if( t.spelling() == "class" ) {
            return true;
        } else if( t.spelling() == "{" || t.spelling() == "{" ) {
            break;
        }
    }
    return {};
}

std::string CppEnum::forward_declaration() const
{
    std::string result = is_enum_class() ? "enum class " : "enum ";
    result += cursor_.spelling();
    if( const cata::optional<std::string> ut = underlying_cpp_type() ) {
        result += " : " + *ut;
    }
    result += ";";
    Cursor c = cursor_;
    while( true ) {
        const Cursor parent = c.get_semantic_parent();
        const CXCursorKind k = parent.kind();
        if( k == CXCursor_Namespace ) {
            result = "namespace " + parent.spelling() + " {" + result + "}";
            c = parent;
        } else if( k == CXCursor_ClassDecl || k == CXCursor_StructDecl ) {
            // *this is an inner enum, we can not forward declare it, so just include the header containing it.
            return "#include \"" + parent.location_file() + "\"\n";
        } else {
            break;
        }
    }
    return result + "\n";
}

std::string CppEnum::export_( Exporter &p ) const
{
    static const std::string tab( 4, ' ' );
    const std::string lua_name = p.lua_name( full_name() );
    std::string r;
    r = r + "register_enum(\"" + lua_name + "\", {\n";
    if( lua_name != full_name().as_string() ) {
        r = r + tab + "cpp_name = \"" + Exporter::escape_to_lua_string( full_name().as_string() ) + "\",\n";
    }
    r = r + tab + "forward_declaration = \"" + Exporter::escape_to_lua_string( forward_declaration() ) + "\",\n";
    r = r + tab + "code_prepend = \"" + Exporter::escape_to_lua_string( "#include \"" + cursor_.location_file() + "\"" ) + "\",\n";
    r = r + tab + "values = {\n";
    for( const std::string &a : values ) {
        r = r + tab + "    \"" + a + "\",\n";
    }
    r = r + tab + "}\n";
    r = r + "} )\n";
    return r;
}

FullyQualifiedId CppEnum::full_name() const
{
    return cursor_.fully_qualifid();
}
