#pragma once
#ifndef RELOAD_OPTION_H
#define RELOAD_OPTION_H

#include "item_location.h"

#include <climits>

class player;
class item;

class reload_option
{
    public:
        reload_option( const reload_option & );
        reload_option &operator=( const reload_option & );

        reload_option( const player *who, const item *target, const item *parent, item_location &&ammo );

        const player *who = nullptr;
        const item *target = nullptr;
        item_location ammo;

        long qty() const {
            return qty_;
        }
        void qty( long val );

        int moves() const;

    private:
        long qty_ = 0;
        long max_qty = LONG_MAX;
        const item *parent = nullptr;
};

#endif
