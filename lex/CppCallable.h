#pragma once

#include "Cursor.h"

#include <vector>
#include <string>

class Exporter;
class FullyQualifiedId;
class Type;

/**
 * Generic callable thing, it contains an argument list and can export the list.
 */
class CppCallable
{
    protected:
        Cursor cursor;
        std::vector<Cursor> arguments_;

    private:
        static size_t min_arguments( const Cursor &c );
        static size_t max_arguments( const Cursor &c );

    public:
        CppCallable( const Cursor &c );
        virtual ~CppCallable();

        /**
         * Exports the list of arguments suitable for the "args = ..." entry
         * in the Lua table of functions.
         * Example results: '{ }', '{ "int", "point" }'
         */
        std::string export_arguments( Exporter &p ) const;
        /**
         * Exports the return type (if any) suitable for the "rval = ..." entry
         * in the Lua table of functions.
         * Example results: 'nil', '"string"', '"point"'
         */
        std::string result_type_as_lua_string( Exporter &p ) const;
        /**
         * Returns the name of the operator that this function represents.
         * Returns an empty string if this is not an operator.
         * The return value does *not* include the "operator" prefix.
         */
        std::string operator_name() const;
        /// Returns whether this function represents a C++ operator.
        bool is_operator() const;
        /**
         * The name of the function (without arguments). Inheriting classes
         * may add their own prefix if needed.
         */
        virtual FullyQualifiedId full_name() const;
        /**
         * Returns the @ref full_name plus the list of argument types.
         * This identifies the function unambiguously (and distinctive
         * from any overloads).
         */
        std::string full_name_with_args() const;
        /**
         * Check whether the types (and order) of the arguments is identical.
         * This is used to detect function declarations that declare the very
         * same C++ function.
         */
        bool has_same_arguments( const CppCallable &other ) const;

        const std::vector<Cursor> &arguments() const {
            return arguments_;
        }
        Type result_type() const;
        /**
         * Simulate function overloading instead of having default parameter values.
         * Lua does not have default parameter values, but it has (some kind of)
         * overloading. This function adds multiple instances of the given
         * function declaration ( @p c ) to the @p container, each with a different
         * argument count.
         * Example: given a function declaration "void f( int = 0 )", it adds
         * - a function "void f()" with 0 arguments, and
         * - a function "void f(int)" with 1 argument.
         */
        template<typename Container, typename ...Args>
        static void create_multiple_from( const Cursor &c, Container &container, Args &&... args  ) {
            for( size_t i = min_arguments( c ), e = max_arguments( c ); i <= e; ++i ) {
                container.emplace_back( std::forward<Args>( args )..., c, i );
            }
        }
};
