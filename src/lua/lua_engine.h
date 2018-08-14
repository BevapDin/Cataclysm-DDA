#pragma once
#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <string>
#include <sstream>

//@todo hide this
class lua_iuse_wrapper;

class lua_engine
{
    private:
        friend class lua_iuse_wrapper;

        // Wrapper for anonymous pointer so we can store
        // lua_state without having a definition of it in this header.
        class pointer_wrapper
        {
            private:
                void *ptr;
            public:
                pointer_wrapper( void *const p ): ptr( p ) { }
                template<typename T>
                operator T *() const {
                    return static_cast<T *>( ptr );
                }
                explicit operator bool() const {
                    return ptr;
                }
        } state;

    public://@todo make private
        std::string lua_file_path;
        std::stringstream output_stream;
        std::stringstream error_stream;

    public:
        lua_engine();
        lua_engine( const lua_engine & ) = delete;

        ~lua_engine();

        /// @throws upon syntax or similar errors
        void run_file( const std::string &path );

        /// @throws when initialization fails
        void init();
        /// @throws upon syntax or similar errors
        void loadmod( const std::string &base_path, const std::string &main_file_name );

        /// May throw, but syntax errors from user input is handled by the console itself.
        void run_console();

        /// @throws Given a Lua error code, throw if it's not ERR_OK.
        void throw_upon_lua_error( int err, const char *path ) const;

        bool enabled() const;
};

#endif
