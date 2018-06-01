#pragma once

#include <stdexcept>
#include <string>

/**
 * Thrown by the translate functions below to indicate the C++ type does not
 * have a matching Lua type and can therefor(  not be used in the Lua wrapper.
 */
class TypeTranslationError : public std::runtime_error
{
    public:
        TypeTranslationError( const std::string &message ) : runtime_error( message ) { }
        ~TypeTranslationError() override = default;
};

/**
 * Raised when an object from C++ is not exported to Lua.
 */
class SkippedObjectError : public std::runtime_error
{
    public:
        SkippedObjectError( const std::string &message ) : runtime_error( message ) { }
        ~SkippedObjectError() override = default;
};
