#include "flag.h"

#include "debug.h"
#include "json.h"

#include <map>
#include <algorithm>

template<typename T>
const json_flag<T> &json_flag_manager<T>::get( const std::string &id )
{
    static json_flag<T> null_flag;
    auto iter = all_.find( id );
    return iter != all_.end() ? iter->second : null_flag;
}

template<typename T>
void json_flag_manager<T>::load( JsonObject &jo )
{
    auto id = jo.get_string( "id" );
    auto &f = all_.emplace( id, json_flag<T>( id ) ).first->second;

    jo.read( "info", f.info_ );
    jo.read( "conflicts", f.conflicts_ );
    jo.read( "inherit", f.inherit_ );
}

template<typename T>
void json_flag_manager<T>::check_consistency()
{
    std::vector<std::string> flags;
    std::transform( all_.begin(), all_.end(), std::back_inserter( flags ),
    []( const std::pair<std::string, json_flag<T>> &e ) {
        return e.first;
    } );

    for( const auto &e : all_ ) {
        const auto &f = e.second;
        if( !std::includes( flags.begin(), flags.end(), f.conflicts_.begin(), f.conflicts_.end() ) ) {
            debugmsg( "flag definition %s specifies unknown conflicting field", f.id().c_str() );
        }
    }
}

template<typename T>
void json_flag_manager<T>::reset()
{
    all_.clear();
}

class item;
template class json_flag_manager<item>;
