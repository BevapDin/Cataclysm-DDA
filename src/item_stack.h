#pragma once
#ifndef ITEM_STACK_H
#define ITEM_STACK_H

#include <list>
#include <cstddef>

class item;
namespace units
{
template<typename V, typename U>
class quantity;
class volume_in_milliliter_tag;
using volume = quantity<int, volume_in_milliliter_tag>;
} // namespace units

/**
 * A wrapper class to bundle up the references needed for a caller to safely manipulate
 * items and obtain information about items in a particular location.
 * Note this does not expose the container itself,
 * which means you cannot call e.g. vector::erase() directly.
 *
 * Pure virtual base class for a collection of items with origin information.
 * Only a subset of the functionality is callable without casting to the specific
 * subclass, e.g. not begin()/end() or range loops.
 */
class item_stack
{
    protected:
        std::list<item> *mystack;

        item_stack( std::list<item> *mystack ) : mystack( mystack ) { }

    public:
        size_t size() const;
        bool empty() const;
        virtual std::list<item>::iterator erase( std::list<item>::iterator it ) = 0;
        virtual void push_back( const item &newitem ) = 0;
        virtual item &insert_at( std::list<item>::iterator, const item &newitem ) = 0;
        virtual void clear();
        item &front();
        item &operator[]( size_t index );

        /**
         * Repeatedly calls @ref erase to remove all items in
         * this stack.
         * Postcondition:
         * `size() == 0`
         */
        void remove_all();
        /**
         * Attempts to find and remove the given item (which is compared
         * by its address).
         * @returns Whether the item was found (and therefor removed, remove
         * always succeeds).
         */
        bool remove( const item &it );
        /**
         * Removes the item with the given index in the stack.
         * The index must be valid.
         */
        void remove( size_t index );

        std::list<item>::iterator begin();
        std::list<item>::iterator end();
        std::list<item>::const_iterator begin() const;
        std::list<item>::const_iterator end() const;
        std::list<item>::reverse_iterator rbegin();
        std::list<item>::reverse_iterator rend();
        std::list<item>::const_reverse_iterator rbegin() const;
        std::list<item>::const_reverse_iterator rend() const;

        /** Maximum number of items allowed here */
        virtual int count_limit() const = 0;
        /** Maximum volume allowed here */
        virtual units::volume max_volume() const = 0;
        /** Total volume of the items here */
        units::volume stored_volume() const;
        units::volume free_volume() const;
        /**
         * Returns how many of the specified item (or how many charges if it's counted by charges)
         * could be added without violating either the volume or itemcount limits.
         *
         * @returns Value of zero or greater for all items. For items counted by charges, it is always at
         * most it.charges.
         */
        long amount_can_fit( const item &it ) const;
        /** Return the item (or nullptr) that stacks with the argument */
        item *stacks_with( const item &it );
        const item *stacks_with( const item &it ) const;

        /**
         * Add the item to this stack. If there is not enough room here
         * and the item is counted by charges, it only adds parts of it.
         * The charges of the item will be adjusted accordingly.
         * @return Whether the item has been added *completely*. If only
         * parts of it have been added, it will still return `false`.
         */
        std::list<item>::iterator add( item &itm );

    private:
        std::list<item>::iterator add_with_merge( item &itm );
};

#endif
