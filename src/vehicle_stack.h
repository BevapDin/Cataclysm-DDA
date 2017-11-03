#pragma once
#ifndef VEHICLE_STACK_H
#define VEHICLE_STACK_H

#include "item_stack.h"
#include "enums.h"
#include "game_constants.h"

class vehicle;

// everything from here is implemented in vehicle.cpp
class vehicle_stack : public item_stack
{
    private:
        tripoint location;
        vehicle *myorigin;
        int part_num;
    public:
        vehicle_stack( std::list<item> *newstack, tripoint newloc, vehicle *neworigin, int part ) :
            item_stack( newstack ), location( newloc ), myorigin( neworigin ), part_num( part ) {};
        std::list<item>::iterator erase( std::list<item>::iterator it ) override;
        void push_back( const item &newitem ) override;
        void insert_at( std::list<item>::iterator index, const item &newitem ) override;
        int count_limit() const override {
            return MAX_ITEM_IN_VEHICLE_STORAGE;
        }
        units::volume max_volume() const override;
};

#endif
