#pragma once
#ifndef LOOK_AROUND_H

#include "cursesdef.h"
#include "enums.h"

#include <vector>
#include <string>

class Creature;
class map_item_stack;

class look_around_t {
    public:
        catacurses::window w_info;
        tripoint start_point;
        bool has_first_point;
        bool select_zone;

        bool bVMonsterLookFire;

    private:
        std::string sFilter;
        std::string list_item_upvote;
        std::string list_item_downvote;

        enum class vmenu_ret : int {
            CHANGE_TAB,
            QUIT,
            FIRE,
        };

        vmenu_ret list_items( const std::vector<map_item_stack> &item_list );
        std::vector<map_item_stack> find_nearby_items( int iRadius );
        void reset_item_list_state( WINDOW *window, int height, bool bRadiusSort );

        vmenu_ret list_monsters( const std::vector<Creature *> &monster_list );

    public:
        look_around_t() = default;

        tripoint invoke();

        void list_items_monsters();
};

#endif
