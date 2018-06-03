#pragma once
#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <string>
#include <sstream>

//@todo hide this
class lua_iuse_wrapper;

template<typename T>
class int_id;
class map;
class time_point;
struct mapgendata;
struct oter_t;
using oter_id = int_id<oter_t>;

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

        /// @throws when initialization fails
        void init();
        void loadmod( const std::string &base_path, const std::string &main_file_name );

        void run_console();

        int call( const std::string &script );
        void callback( const char *name );

        // This is a legacy function, ideally code would use `catalua::call` instead,
        // but for mapgen we do a bit more than just call the script.
        int mapgen( map *m, const oter_id &terrain_type, const mapgendata &, const time_point &t, float, const std::string &scr );
};

#endif
