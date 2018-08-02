#pragma once

#include <vector>
#include <string>
#include <type_traits>
#include <memory>

class Matcher
{
    protected:
        Matcher() = default;

    public:
        virtual ~Matcher() = default;

        virtual bool match( const std::string &what ) const = 0;
};

class MultiMatcher : public Matcher
{
    protected:
        std::vector<std::unique_ptr<Matcher>> matchers;

    public:
        MultiMatcher() = default;
        ~MultiMatcher() override = default;

        template<typename T, typename ...Args>
        void emplace_back( Args &&...args ) {
            std::unique_ptr<Matcher> ptr( new T( std::forward<Args>( args )... ) );
            matchers.emplace_back( std::move( ptr ) );
        }

        bool match( const std::string &what ) const override {
            for( const auto &m : matchers ) {
                if( m->match( what ) ) {
                    return true;
                }
            }
            return false;
        }
};

class SimpleMatcher : public Matcher
{
    protected:
        std::string match_string;

    public:
        SimpleMatcher( const std::string &m ) : match_string( m ) { }
        ~SimpleMatcher() override = default;

        bool match( const std::string &what ) const override {
            return match_string == what;
        }
};

class RegexMatcher : public Matcher
{
    protected:
        std::string prefix;
        std::string postfix;

        static std::string remove_escape_sequences( std::string m ) {
            for( size_t o = m.find( '\\' ); o != std::string::npos; o = m.find( '\\', o ) ) {
                m.erase( o, 1 );
            }
            return m;
        }

    public:
        RegexMatcher( const std::string &m ) {
            const size_t p = m.find( ".*" );
            if( p == std::string::npos ) {
                throw std::runtime_error( "invalid regex, must contain '.*' sequence" );
            }
            prefix = remove_escape_sequences( m.substr( 0, p ) );
            postfix = remove_escape_sequences( m.substr( p + 2, m.length() - p - 2 ) );
        }
        ~RegexMatcher() override = default;

        bool match( const std::string &what ) const override {
            return what.length() >= prefix.length() + postfix.length() &&
                   what.compare( 0, prefix.length(), prefix ) == 0 &&
                   what.compare( what.length() - postfix.length(), postfix.length(), postfix ) == 0;
        }
};
