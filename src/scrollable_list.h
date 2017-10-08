#pragma once
#ifndef SCROLLABLE_LIST_H
#define SCROLLABLE_LIST_H

#include "cursesdef.h"
#include "color.h"

#include <string>
#include <type_traits>
#include <functional>
#include <algorithm>

/**
 * Basic interface used by @ref scrollable_list to provide the data to be displayed.
 * You probably don't want to use it directly, use @ref typed_scrollable_data
 * or @ref scrollable_data_container instead. Those classes inherit from this
 * one, but are type-safe by using a template parameter.
 */
class scrollable_data {
    public:
        virtual ~scrollable_data() = default;
        /**
         * @return The number of items to be displayed in the list. May return 0.
         */
        virtual size_t count() const = 0;
        /**
         * @return The text to print for entry with given @p index. Can assume
         * that @p index is valid (according to @ref count).
         * Returned string may contain @ref color_tags.
         * @param active Whether the item is the currently selected one.
         */
        virtual std::string get_entry_by_index( size_t index, bool active ) const = 0;
        /// Just shortcut for @ref count returning 0.
        bool empty() const { return count() == 0; }
};
/**
 * Type safe version of @ref scrollable_data. The template parameter defines the
 * type of the elements of the scrollable data (e.g. `std::string` when displaying
 * a vector of strings). It allows type safe access to the displayed data, used by
 * @ref scrollable_list to enhance its behavior, specifically to allow re-selecting
 * the same entry when the scrollable data changes.
 * You typically don't need this class diretly, you can use @ref scrollable_data_container
 * instead.
 */
template<typename ElementType>
class typed_scrollable_data : public scrollable_data {
    public:
        using element_type = ElementType;

        ~typed_scrollable_data() override = default;
        /**
         * Returns the element to be displayed at the given index, that means the
         */
        virtual const element_type &get_element( size_t index ) const = 0;
        /**
         * Returns the index that contains an element equal to the given one.
         * If no element matches the argument, or if the elements don't support
         * equality comparison, it should return 0.
         */
        virtual size_t get_index( const element_type &element ) const = 0;

        virtual std::string get_entry_by_value( const element_type &element, bool active ) const = 0;

        std::string get_entry_by_index( const size_t index, const bool active ) const override {
            return get_entry_by_value( get_element( index ), active );
        }
};
/**
 * Basic UI class to display a scrollable list. The displayed list contains entries
 * (one entry per line) taken from the @ref scrollable_data object (set via the
 * constructor or via @ref data).
 * It does not do any user interaction, but it contains function that are typically
 * called as a result of user input (e.g. @ref move_cursor_down).
 */
class scrollable_list {
    protected:
        WINDOW *w;
        int x;
        int y;
        int width;
        int height;
        int cursor_ = 0;
        int scroll_offset_ = 0;
        std::reference_wrapper<scrollable_data> data_;
        nc_color default_color_;
        nc_color active_color_;

    public:
        scrollable_list( WINDOW *const w, const int x, const int y, const int width, const int height, scrollable_data &data )
        : w( w ), x( x ), y( y ), width( width ), height( height ), data_( data ), default_color_( c_white ), active_color_( c_green ) {
        }

        void default_color( const nc_color color ) {
            default_color_ = color;
        }
        void active_color( const nc_color color ) {
            active_color_ = color;
        }

        /**
         * Set the scrollable data that is to be displayed. Resets the @ref cursor
         * and the scroll offset to 0.
         */
        void data( scrollable_data &new_data );
        /**
         * Same as other @ref data, but applies to @ref typed_scrollable_data instances,
         * and only when the contained type is copy constructible.
         * It attempts to re-select the currently selected entry in the list, but that
         * can only work if the currently used data *and* the new data are derived
         * from @ref typed_scrollable_data<T>.
         */
        // typename std::enable_if<typename std::is_copy_constructible<T>::value, void>::type
        template<typename T>
        void data( typed_scrollable_data<T> &new_data ) {
            const typed_scrollable_data<T> *const old_data = dynamic_cast<const typed_scrollable_data<T>*>( &data_.get() );
            if( !old_data ) {
                return data( static_cast<scrollable_data&>( new_data ) );
            }
            if( old_data->empty() || new_data.empty() ) {
                return data( static_cast<scrollable_data&>( new_data ) );
            }

            const T old_selected = old_data->get_element( cursor() );
            data_ = std::ref( new_data );

            // attempt to re-select the previously selected entry
            cursor( new_data.get_index( old_selected ) );
        }

        void erase();
        void draw() const;

        size_t cursor() const {
            return static_cast<size_t>( cursor_ );
        }
        int page_size() const {
            return height;
        }
        int count() const {
            return data_.get().count();
        }

        /**@{*/
        void cursor( const int new_cursor );
        void move_cursor_by( const int offset ) {
            cursor( cursor_ + offset );
        }
        void move_cursor_up() {
            return move_cursor_by( -1 );
        }
        void move_cursor_down() {
            return move_cursor_by( +1 );
        }
        void move_cursor_page_up() {
            return move_cursor_by( -page_size() );
        }
        void move_cursor_page_down() {
            return move_cursor_by( +page_size() );
        }
        /**@}*/

        void draw_scrollbar( int offset_y = 0, int offset_x = 0, nc_color bar_color = c_white ) const;
};

/**
 * A generic wrapper wrapper for a container of elements that need to be displayed
 * as a scrollable list.
 * @tparam ContainerType The type of the container that contains the elements.
 * The class will store a reference to an object of that type. The type must
 * support iterators and must have a `size` function.
 * @tparam ElementType The type of the contained elements, see @ref typed_scrollable_data.
 */
template<typename ContainerType, typename ElementType = typename ContainerType::value_type>
class scrollable_data_container : public typed_scrollable_data<ElementType> {
    public:
        using container_type = ContainerType;
        using element_type = typename typed_scrollable_data<ElementType>::element_type;
        container_type &data_;

        scrollable_data_container( container_type &d ) : data_( d ) { }
        ~scrollable_data_container() override = default;

        const element_type &get_element( const size_t index ) const override {
            auto iter = data_.begin();
            std::advance( iter, index );
            return *iter;
        }
        size_t get_index( const element_type &element ) const override {
            const auto iter = std::find( data_.begin(), data_.end(), element );
            return iter == data_.end() ? 0 : iter - data_.begin();
        }
        size_t count() const override {
            return data_.size();
        }
};

template<typename C, typename O>
void sort( const O &comparator, C &container, scrollable_list &list ) {
    const auto old_selected = container.get_element( list.cursor() );
    std::stable_sort( container.data_.begin(), container.data_.end(), comparator );
    // attempt to re-select the previously selected entry
    list.cursor( container.get_index( old_selected ) );
}

template<typename ContainerType, typename ElementType = typename ContainerType::value_type>
class filterable_scroll_data : public scrollable_data_container<ContainerType, ElementType> {
    public:
        using parent_type = scrollable_data_container<ContainerType, ElementType>;
        using container_type = typename parent_type::container_type;
        using element_type = typename parent_type::element_type;

        std::vector<size_t> filtered;

        filterable_scroll_data( container_type &data ) : parent_type( data ) { }
        ~filterable_scroll_data() override = default;

        bool filter_by( const std::function<bool( const element_type & )> &predicate ) {
            filtered.clear();
            for( size_t i = 0; i < this->data_.size(); ++i ) {
                if( predicate( this->data_[i] ) ) {
                    filtered.push_back( i );
                }
            }
            return !filtered.empty();
        }
        bool filter_by( const std::function<bool( const element_type & )> &predicate, scrollable_list &list ) {
            const size_t old_cursor = list.cursor();
            const bool result = filter_by( predicate );
            const auto iter = std::find( filtered.begin(), filtered.end(), old_cursor );
            list.cursor( iter != filtered.end() ? iter - filtered.begin() : 0 );
            return result;
        }

        void allow_all() {
            filtered.clear();
        }

        size_t count() const override {
            return filtered.empty() ? parent_type::count() : filtered.size();
        }
        std::string get_entry_by_index( const size_t index, const bool active ) const override {
            if( filtered.empty() ) {
                return parent_type::get_entry_by_index( index, active );
            } else {
                return parent_type::get_entry_by_index( filtered[index], active );
            }
        }
};

#endif
