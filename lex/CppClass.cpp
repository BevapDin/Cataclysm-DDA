#include "CppClass.h"

#include "Parser.h"
#include "Exporter.h"
#include "CppFunction.h"
#include "CppAttribute.h"
#include "CppConstructor.h"
#include "Type.h"
#include "Cursor.h"

#include <algorithm>
#include <vector>

CppClass::CppClass( Parser &p, const Cursor &cursor ) : full_name_(cursor.fully_qualifid()), cpp_name_( cursor.spelling()), source_file_(cursor.location())
{
    source_file_.erase(0, source_file_.find_last_of("\\//") + 1);
    source_file_.erase(source_file_.find_first_of(":"), source_file_.length());
    
    p.debug_message( "Parsing class " + full_name() + " at " + cursor.location() );
    cursor.visit_children( [this, &p]( const Cursor &c, const Cursor &/*parent*/ ) {
        if( !c.is_public() ) {
            return CXChildVisit_Continue;
        }

        const CXCursorKind k = c.kind();
        if( k == CXCursor_StructDecl || k == CXCursor_ClassDecl ) {
            // Inner classes.
            p.parse_class( c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FunctionDecl ) {
            // A free function declared inside the class (friend function most likely).
            p.parse_function( c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_CXXMethod ) {
            functions.emplace_back( *this, c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FieldDecl ) {
            attributes.emplace_back( *this, c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_Constructor ) {
            constructors.emplace_back( *this, c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_CXXBaseSpecifier ) {
            parents.emplace_back( p.get_or_add_class( c.get_definition() ) );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_CXXAccessSpecifier ) {
            return CXChildVisit_Continue;
        } else if( k == CXCursor_ConversionFunction ) {
            p.skipped( "conversion function", c.fully_qualifid(), "not supported" );
            //@todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypeAliasDecl ) {
            p.skipped( "type alias declaration", c.fully_qualifid(), "not supported" );
            //@todo maybe give to the Parser to handle it?
            return CXChildVisit_Continue;
        } else if( k == CXCursor_VarDecl ) {
            p.parse_variable( c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FunctionTemplate ) {
            p.skipped( "function template", c.fully_qualifid(), "not supported" );
            //@todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_FriendDecl ) {
            // Not useful for exporting at all
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UnexposedDecl ) {
            // I don't even know what this could be
            return CXChildVisit_Continue;
        } else if( k == CXCursor_UsingDeclaration ) {
            p.skipped( "using declaration in class", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_EnumDecl ) {
            p.parse_enum( c );
            return CXChildVisit_Continue;
        } else if( k == CXCursor_EnumConstantDecl ) {
            p.skipped( "enum constant decl", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_TypedefDecl ) {
            p.skipped( "typedef in class", c.fully_qualifid(), "not supported" );
            // @todo
            return CXChildVisit_Continue;
        } else if( k == CXCursor_Destructor ) {
            // We don't need to know about this. The wrapper can call
            // the implicitly constructed one anyway.
            return CXChildVisit_Continue;
        }
        c.dump( "Cursor kind not handled upon parsing class definition" );
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
        } else if( f.operator_name() == "==") {
            // Note: this assumes any `operator==` compares with the same class type,
            // and not with a different type.
            // @todo: check the arguments for this assumption.
            has_equal = true;
            // Remove the function as it is exported via Lua's own comparison system
            iter = functions.erase( iter );
            //@todo handle more operators
        } else {
            ++iter;
        }
    }
}

CppClass::CppClass( const std::string &ns, const std::string &n ) :full_name_(Parser::fully_qualifid(ns, n)), cpp_name_( n )
{
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

template<typename T>
static std::string print_objects( Exporter &p, const T &objects, const std::string &prefix,
                                  const std::string &postfix )
{
    std::list<std::string> lines;
    for( const auto &o : objects ) {
        for( const auto &line : o->export_( p ) ) {
            if( line.compare( 0, 2, "--" ) == 0 ) {
                lines.push_back( line );
                //@todo print the comment?
                // fprintf( stdout, "%s", line.c_str() );
            } else {
                lines.push_back( line );
            }
        }
    }

    std::string r = prefix;
    for( const std::string &l : std::set<std::string>( lines.begin(), lines.end() ) ) {
        r = r + std::string( 12, ' ' ) + l + ",\n";
    }
    r = r + postfix;
    return r;
}

void CppClass::gather_parent( Exporter &e, std::vector<const CppFunction*> &functions,
                              std::vector<const CppAttribute*> &attributes ) const
{
    //@todo
    // Special construct to find templated classes like `visitable<T>`
    #if 0
    pc.visit_children( [&]( const Cursor &c, const Cursor &/*p*/ ) {
        if( c.kind() == CXCursor_TemplateRef ) {
            definition = c.get_definition();
            return CXChildVisit_Break;
        }
        return CXChildVisit_Continue;
    } );
    #endif
    for( const CppFunction &f : this->functions ) {
        functions.push_back( &f );
    }
    for( const CppAttribute &a : this->attributes ) {
        attributes.push_back( &a );
    }
    for( const CppClass &pc : this->parents ) {
        pc.gather_parent( e, functions, attributes );
    }
}

std::string CppClass::export_( Exporter &p ) const
{
    p.debug_message( "Exporting class " + cpp_name() );
    static const std::string tab( 8, ' ' );
    // Both lists will include the functions of parent classes
    std::vector<const CppFunction*> functions;
    for( const CppFunction &f : this->functions ) {
        functions.push_back( &f );
    }
    std::vector<const CppAttribute*> attributes;
    for( const CppAttribute &a : this->attributes ) {
        attributes.push_back( &a );
    }
    // this is only for compatibility with the print_objects function
    std::vector<const CppConstructor*> constructors;
    for( const CppConstructor &c : this->constructors ) {
        constructors.push_back( &c );
    }

    std::string r;
    //@todo lua name
    r = r + "    " + cpp_name() + " = {\n";

    // @todo properly pick all required headers
    r = r + "        headers = { \"" + source_file_ + "\" },\n";
	// @todo export forward declarations

    for( const CppClass &pc : parents ) {
        p.debug_message("Parent of " + full_name() + " is " + pc.full_name());
        //@todo should be full_name, not cpp_name
        if( p.export_enabled( pc.cpp_name() ) ) {
            r = r + tab + "parent = \"" + pc.full_name() + "\",\n";
        } else {
            // Parent class is not exported directly, but we still have to include its
            // functions and attributes
            pc.gather_parent( p, functions, attributes );
        }
    }

    // Exporting the constructor only makes sense for types that can have by-value semantic
    if( p.export_by_value( cpp_name() ) ) {
        r = r + print_objects( p, constructors, tab + "new = {\n", tab + "},\n" );
    }

    const std::string sid = p.get_string_id_for( cpp_name() );
    if( !sid.empty() ) {
        r = r + tab + "string_id = \"" + sid + "\",\n";
    }
    const std::string iid = p.get_int_id_for( cpp_name() );
    if( !iid.empty() ) {
        r = r + tab + "int_id = \"" + iid + "\",\n";
    }

    if( p.export_by_value( cpp_name() ) && p.export_by_reference( cpp_name() ) ) {
        r = r + tab + "by_value_and_reference = true,\n";
    } else if( p.export_by_value( cpp_name() ) ) {
        r = r + tab + "by_value = true,\n";
    } else {
        // by reference is the default.
    }

    if( has_equal ) {
        r = r + tab + "has_equal = true,\n";
    }

    r = r + print_objects( p, attributes, tab + "attributes = {\n", tab + "},\n" );
    r = r + print_objects( p, functions, tab + "functions = {\n", tab + "}\n" );

    r = r + "    }";

    return r;
}

std::string CppClass::full_name() const
{
    return full_name_;
}
        std::string CppClass::cpp_name() const {
            return cpp_name_;
        }
