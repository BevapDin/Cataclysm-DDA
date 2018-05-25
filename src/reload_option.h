#pragma once
#ifndef RELOAD_OPTION_H
#define RELOAD_OPTION_H

#include "item_location.h"

class player;
class item;

class reload_option
{
    public:
        reload_option( const reload_option & );
        reload_option &operator=( const reload_option & );

        reload_option( const player &who, const item &target, const item &parent, item_location &&ammo );

        const player *who;
        const item *target;
        item_location ammo;

        long qty() const {
            return qty_;
        }
        void qty( long val );

        int moves() const;

    private:
        long qty_;
        long max_qty;
        const item *parent;
};

#endif
