#include "item_stack.h"

#include "units.h"
#include "item.h"

#include <list>
#include <algorithm>

size_t item_stack::size() const
{
    return mystack->size();
}

bool item_stack::empty() const
{
    return mystack->empty();
}

void item_stack::clear()
{
    // An acceptable implementation for list; would be bad for vector
    while( !empty() ) {
        erase( begin() );
    }
}

std::list<item>::iterator item_stack::begin()
{
    return mystack->begin();
}

std::list<item>::iterator item_stack::end()
{
    return mystack->end();
}

std::list<item>::const_iterator item_stack::begin() const
{
    return mystack->cbegin();
}

std::list<item>::const_iterator item_stack::end() const
{
    return mystack->cend();
}

std::list<item>::reverse_iterator item_stack::rbegin()
{
    return mystack->rbegin();
}

std::list<item>::reverse_iterator item_stack::rend()
{
    return mystack->rend();
}

std::list<item>::const_reverse_iterator item_stack::rbegin() const
{
    return mystack->crbegin();
}

std::list<item>::const_reverse_iterator item_stack::rend() const
{
    return mystack->crend();
}

item &item_stack::front()
{
    return mystack->front();
}

item &item_stack::operator[]( size_t index )
{
    return *( std::next( mystack->begin(), index ) );
}

units::volume item_stack::stored_volume() const
{
    units::volume ret = 0;
    for( const item &it : *mystack ) {
        ret += it.volume();
    }
    return ret;
}

long item_stack::amount_can_fit( const item &it ) const
{
    // Without stacking charges, would we violate the count limit?
    const bool violates_count = size() >= ( size_t )count_limit();
    const item *here = it.count_by_charges() ? stacks_with( it ) : nullptr;

    if( violates_count && !here ) {
        return 0l;
    }
    // Call max because a tile may have been overfilled to begin with (e.g. #14115)
    long ret = std::max( 0l, it.charges_per_volume( free_volume() ) );
    return it.count_by_charges() ? std::min( ret, it.charges ) : ret;
}

item *item_stack::stacks_with( const item &it )
{
    for( item &here : *mystack ) {
        if( here.stacks_with( it ) ) {
            return &here;
        }
    }
    return nullptr;
}

const item *item_stack::stacks_with( const item &it ) const
{
    for( const item &here : *mystack ) {
        if( here.stacks_with( it ) ) {
            return &here;
        }
    }
    return nullptr;
}

units::volume item_stack::free_volume() const
{
    return max_volume() - stored_volume();
}

void item_stack::remove_all()
{
    while( !empty() ) {
        erase( begin() );
    }
}

bool item_stack::remove( const item &it )
{
    const auto iter = std::find_if( begin(), end(), [&]( const item & in_stack ) {
        return &in_stack == &it;
    } );
    if( iter == end() ) {
        return false;
    }
    erase( iter );
    return true;
}

void iem_stack::remove( const size_t index )
{
    if( index >= size() ) {
        // @todo write a debgmsg
        return;
    }
    erase( std::next( begin(), index ) );
}

std::list<item>::iterator item_stack::add_with_merge( const item &itm )
{
    assert( itm.count_by_charges() ); // `charges` does mean "number of items"
    assert( itm.charges > 0 ); // does not makes sense to add 0 charges

    for( auto iter = mystack->begin(); iter != mystack->end(); ++iter ) {
        if( iter->merge_charges( itm ) ) {
            return iter;
        }
    }
    return mystack->end();
}

std::list<item>::iterator item_stack::add( item &itm )
{
    if( itm.count_by_charges() ) {
        const long charges_to_add = std::min( itm.charges, amount_can_fit( itm ) );
        if( charges_to_add <= 0 ) {
            return false;
        }
        const long old_charges = itm.charges;
        itm.charges = charges_to_add;
        const auto iter = add_with_merge( itm );
        if( iter != mystack->end() ) {
            itm.charges = old_charges - charges_to_add;
            return iter;
        }
        // fall through to adding the whole item
    }

    return mystack->insert( mystack->end(), itm );
}
