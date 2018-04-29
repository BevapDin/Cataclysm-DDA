#pragma once
#ifndef POLY_PIMPL_H
#define POLY_PIMPL_H

#include <memory>
#include <type_traits>
#include <cassert>

template<typename T>
class poly_pimpl;

template<typename T>
class poly_pimpl_creator
{
    public:
        std::unique_ptr<T> ptr;

        template<typename ...Args>
        poly_pimpl_creator( Args &&... args ) : ptr( new T( std::forward<Args>( args )... ) ) { }

        poly_pimpl_creator() = delete;
        poly_pimpl_creator( poly_pimpl_creator && ) = default;

        poly_pimpl_creator &operator=( const poly_pimpl_creator & ) = delete;
};

template<typename T, typename ...Args>
poly_pimpl_creator<T> make_poly_pimpl( Args && ... args )
{
    return poly_pimpl_creator<T>( std::forward<Args>( args )... );
}

/**
 * Similar to @ref pimpl, this provides a "pointer to implementation" wrapper,
 * with the additional feature of allowing polymorphic types.
 * Where @ref pimpl can only store value of the type given as its template
 * argument, this class can store any *derived* type.
 *
 * This has further consequences:
 * - poly_pimpl is not nullable (it always contains a *valid* pointer) and
 *   must be initialized with a value (or @tp must be default constructible).
 * - Move construction (from another poly_pimpl) is not allowed, it would
 *   leave the other instance with a null pointer. Move assignment is allowed
 *   as it just swaps the pointers.
 */
template<typename T>
class poly_pimpl : private std::unique_ptr<T>
{
    public:
//        template<typename = typename std::enable_if<std::is_default_constructible<T>::value>::type>
//        poly_pimpl() : std::unique_ptr<T>( new T() ) { }

        template<typename O, typename = typename std::enable_if<std::is_base_of<T, O>::value>::type>
        poly_pimpl( poly_pimpl_creator<O> && other ) : std::unique_ptr<T>( std::move( other.ptr ) ) {
            assert( this->get() != nullptr );
        }

        poly_pimpl( const poly_pimpl<T> &other ) : std::unique_ptr<T>( other->clone() ) { }

        // A poly_pimpl must *always* contain a valid pointer.
        poly_pimpl( decltype( nullptr ) ) = delete;
        // Allowing unique_ptr is not safe (as it can be null)
        template<typename O>
        poly_pimpl( std::unique_ptr<O> ) = delete;

        poly_pimpl &operator=( const poly_pimpl<T> &rhs ) {
            poly_pimpl new_one( rhs );
            swap( new_one );
            return *this;
        }
        poly_pimpl &operator=( poly_pimpl<T> &&rhs ) {
            swap( rhs );
            return *this;
        }

        using std::unique_ptr<T>::operator*;
        using std::unique_ptr<T>::operator->;

        void swap( poly_pimpl<T> &other ) {
            using std::swap;
            swap( static_cast<std::unique_ptr<T>&>( *this ), static_cast<std::unique_ptr<T>&>( other ));
        }

        bool operator==( const poly_pimpl<T> &rhs ) const {
            return operator*() == *rhs;
        }
        bool operator!=( const poly_pimpl<T> &rhs ) const {
            return !operator==( rhs );
        }
};

#endif
