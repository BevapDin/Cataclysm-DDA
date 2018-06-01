#include "CppClass.h"

#include "Parser.h"
#include "Exporter.h"
#include "CppFunction.h"
#include "CppAttribute.h"
#include "CppConstructor.h"
#include "Type.h"
#include "Cursor.h"
#include "common.h"
#include "exceptions.h"
#include "FullyQualifiedId.h"

#include <algorithm>
#include <vector>

CppClass::CppClass( Parser &p, const Cursor &cursor ) : cursor_( cursor )
{
    cursor.visit_children( [this, &p]( const Cursor & c, const Cursor &/*parent*/ ) {
        const CXCursorKind k = c.kind();
        if( k == CXCursor_StructDecl || k == CXCursor_ClassDecl ) {
            // Inner classes.
            // @todo
            // p.parse_class( c );
            p.skipped( "inner class", c.fully_qualifid(), "not supported" );
            return CXChildVisit_Continue;

        } else if( k == CXCursor_FunctionDecl ) {
            // A free function declared inside the class (friend function most likely).
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_VarDecl ) {
            // A static variable (therefor global).
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_EnumDecl ) {
            // @todo
            return CXChildVisit_Continue;

        } else if( k == CXCursor_CXXMethod ) {
            CppCallable::create_multiple_from( c, functions, *this );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FieldDecl ) {
            attributes.emplace_back( *this, c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_Constructor ) {
            CppCallable::create_multiple_from( c, constructors, *this );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_CXXBaseSpecifier ) {
            parents.emplace_back( p.get_or_add_class( c.get_definition() ) );
            return CXChildVisit_Continue;

        } else if( k == CXCursor_ConversionFunction ) {
            p.skipped( "conversion function", c.fully_qualifid(), "not supported" );
            //@todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypeAliasDecl ) {
            p.skipped( "type alias declaration", c.fully_qualifid(), "not supported" );
            //@todo maybe give to the Parser to handle it?
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FunctionTemplate ) {
            p.skipped( "function template", c.fully_qualifid(), "not supported" );
            //@todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UsingDeclaration ) {
            p.skipped( "using declaration in class", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_EnumConstantDecl ) {
            p.skipped( "enum constant decl", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypedefDecl ) {
            p.skipped( "typedef in class", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;

        } else if( k == CXCursor_CXXAccessSpecifier ) {
            // Ignored, we check for this manually whenever needed.
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FriendDecl ) {
            // Not useful for exporting at all
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UnexposedDecl ) {
            // I don't even know what this could be
            return CXChildVisit_Continue;
        } else if( k == CXCursor_Destructor ) {
            // We don't need to know about this. The wrapper can call
            // the implicitly constructed one anyway.
            return CXChildVisit_Continue;
        }
        return CXChildVisit_Recurse;
    } );

    // Overloads that only differ on the const of the method are merged into one
    // because Lua doesn't know about const and handles everything as non-const.
    // Example `bar &foo::get(int)` and `const bar &foo:get(int) const` are the
    // same to Lua. For convenience we only export the non-const version.
    for( auto iter = functions.begin(); iter != functions.end(); ) {
        const CppFunction &f = *iter;
        if( f.is_const_method() && has_non_const_overload( f ) ) {
            iter = functions.erase( iter );
        } else {
            ++iter;
        }
    }
}

CppClass::~CppClass() = default;

bool CppClass::has_non_const_overload( const CppFunction &func ) const
{
    for( const CppFunction &f : functions ) {
        if( &f == &func ) {
            continue;
        }
        if( f.is_const_method() || f.full_name() != func.full_name() ) {
            continue;
        }
        if( f.has_same_arguments( func ) ) {
            return true;
        }
    }
    for( const CppClass &p : parents ) {
        if( p.has_non_const_overload( func ) ) {
            return true;
        }
    }
    return false;
}
template<typename O>
static std::set<std::string> print_objects( Exporter &p,
        const std::vector<std::reference_wrapper<const O>> &objects )
{
    std::set<std::string> lines;
    for( const O &o : objects ) {
        if( !o.is_public() ) {
            continue;
        }
        try {
            lines.insert( o.export_( p ) );
        } catch( const TypeTranslationError &e ) {
            lines.insert( "-- " + o.full_name() + " ignored because: " + e.what() );
        } catch( const SkippedObjectError &e ) {
            lines.insert( "-- " + o.full_name() + " ignored because: " + e.what() );
        }
    }
    return lines;
}

static std::set<std::string> print_objects( Exporter &p,
        const std::vector<std::reference_wrapper<const CppAttribute>> &objects )
{
    std::set<std::string> lines;
    for( const CppAttribute &o : objects ) {
        if( !o.is_public() ) {
            continue;
        }
        try {
            lines.insert( o.export_( p ) );
        } catch( const TypeTranslationError &e ) {
            lines.insert( "-- " + o.full_name() + " ignored because: " + e.what() );
        } catch( const SkippedObjectError &e ) {
            lines.insert( "-- " + o.full_name() + " ignored because: " + e.what() );
        }
    }
    return lines;
}

static std::string print_set( const std::set<std::string> &lines, const std::string &prefix,
                              const std::string &postfix )
{
    std::string r = prefix;
    for( const std::string &l : lines ) {
        r = r + std::string( 12, ' ' ) + l + ",\n";
    }
    r = r + postfix;
    return r;
}

void CppClass::gather_parent( Exporter &e,
                              std::vector<std::reference_wrapper<const CppFunction>> &functions,
                              std::vector<std::reference_wrapper<const CppAttribute>> &attributes ) const
{
    //@todo
    // Special construct to find templated classes like `visitable<T>`
#if 0
    pc.visit_children( [&]( const Cursor & c, const Cursor &/*p*/ ) {
        if( c.kind() == CXCursor_TemplateRef ) {
            definition = c.get_definition();
            return CXChildVisit_Break;
        }
        return CXChildVisit_Continue;
    } );
#endif
    for( const CppFunction &f : this->functions ) {
        functions.emplace_back( f );
    }
    for( const CppAttribute &a : this->attributes ) {
        attributes.emplace_back( a );
    }
    for( const CppClass &pc : this->parents ) {
        pc.gather_parent( e, functions, attributes );
    }
}

std::string CppClass::export_( Exporter &p ) const
{
    static const std::string tab( 8, ' ' );
    // Both lists will include the functions of parent classes
    std::vector<std::reference_wrapper<const CppFunction>> functions;
    for( const CppFunction &f : this->functions ) {
        functions.emplace_back( f );
    }
    std::vector<std::reference_wrapper<const CppAttribute>> attributes;
    for( const CppAttribute &a : this->attributes ) {
        attributes.emplace_back( a );
    }
    // this is only for compatibility with the print_objects function
    std::vector<std::reference_wrapper<const CppConstructor>> constructors;
    for( const CppConstructor &c : this->constructors ) {
        constructors.push_back( c );
    }

    for( const CppClass &pc : parents ) {
        if( !p.export_enabled( pc.cursor_.type(), pc.cursor_.location_path() ) ) {
            // Parent class is not exported directly, but we still have to include its
            // functions and attributes
            pc.gather_parent( p, functions, attributes );
        }
    }

    for( auto iter = functions.begin(); iter != functions.end(); ) {
        const CppFunction &f = *iter;
        if( f.is_overriden() ) {
            iter = functions.erase( iter );
        } else {
            ++iter;
        }
    }

    std::string r;
    const std::string lua_name = p.lua_name( full_name() );
    r = r + "    " + lua_name + " = {\n";
    if( lua_name != full_name().as_string() ) {
        r = r + tab + "cpp_name = \"" + full_name() + "\",\n";
    }

    const auto printed_constructors = print_objects<CppConstructor>( p, constructors );
    const auto printed_functions = print_objects<CppFunction>( p, functions );

    for( const CppClass &pc : parents ) {
        // @todo handle multiple parents
        if( p.export_enabled( pc.cursor_.type(), pc.cursor_.location_path() ) ) {
            r = r + tab + "parent = \"" + p.lua_name( pc.full_name() ) + "\",\n";
        }
    }

    // Exporting the constructor only makes sense for types that can have by-value semantic
    if( p.export_by_value( full_name() ) ) {
        r = r + print_set( printed_constructors, tab + "new = {\n", tab + "},\n" );
    }

    if( const auto sid = p.get_string_id_for( full_name() ) ) {
        r = r + tab + "string_id = \"" + *sid + "\",\n";
    }
    if( const auto iid = p.get_int_id_for( full_name() ) ) {
        r = r + tab + "int_id = \"" + *iid + "\",\n";
    }

    if( p.export_by_value( full_name() ) && p.export_by_reference( full_name() ) ) {
        r = r + tab + "by_value_and_reference = true,\n";
    } else if( p.export_by_value( full_name() ) ) {
        r = r + tab + "by_value = true,\n";
    } else {
        // by reference is the default.
    }

    if( has_equal() ) {
        r = r + tab + "has_equal = true,\n";
    }

    r = r + print_set( print_objects( p, attributes ), tab + "attributes = {\n", tab + "},\n" );
    r = r + print_set( printed_functions, tab + "functions = {\n", tab + "}\n" );

    r = r + "    }";

    return r;
}

FullyQualifiedId CppClass::full_name() const
{
    return cursor_.fully_qualifid();
}

bool CppClass::has_equal() const
{
    for( const CppFunction &f : functions ) {
        if( f.operator_name() == "==" ) {
            return true;
        }
    }
    return false;
}
