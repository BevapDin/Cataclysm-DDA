#pragma once
#ifndef CATA_SRC_MAP_STACK_H
#define CATA_SRC_MAP_STACK_H

#include "item_stack.h"
#include "coordinates.h"
#include "game_constants.h"
#include "units_fwd.h"

class map;

class map_stack : public item_stack
{
    private:
        tripoint_bub_ms location;
        map *myorigin;
    public:
        map_stack( cata::colony<item> *newstack, tripoint_bub_ms newloc, map *neworigin ) :
            item_stack( newstack ), location( newloc ), myorigin( neworigin ) {}
        void insert( map &, const item &newitem ) override;
        void insert( const item &newitem );
        iterator erase( const_iterator it ) override;
        int count_limit() const override {
            return MAX_ITEM_IN_SQUARE;
        }
        units::volume max_volume() const override;
};

#endif // CATA_SRC_MAP_STACK_H
