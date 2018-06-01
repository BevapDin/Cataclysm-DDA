#pragma once

#include "Cursor.h"

#include <functional>
#include <list>
#include <vector>
#include <string>

class Exporter;

/**
 * Generic callable thing, it contains an argument list and can export the list.
 */
class CppCallable
{
    protected:
        Cursor cursor;
        std::vector<Cursor> arguments;
        // The min number of arguments. May differ from total number of
        // arguments some parameters are optional.
        size_t min_arguments = 0;

        virtual ~CppCallable();

        CppCallable( const Cursor &c );

    public:
        std::string export_argument_list( Exporter &p, size_t m ) const;
        /**
         * Export all versions of the callable (multiple versions if( there are optional parameters).
         * Result is an array. Each entry is created by calling the callback with exported
         * list of arguments (as string, ready to be printed) as parameter.
         */
        std::list<std::string> export_cb( Exporter &p, const std::function<std::string( const std::string & )> &callback ) const;
        /**
         * Returns the name of the operator that this function represents.
         * Returns an empty string if this is not an operator.
         * The return value does *not* include the "operator" prefix.
         */
        std::string operator_name() const;
        bool is_operator() const {
            return !operator_name().empty();
        }

        virtual std::string full_name() const;
        std::string full_name_with_args() const;
        std::string cpp_name() const;

        bool has_same_arguments( const CppCallable &other ) const;
};
