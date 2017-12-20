#pragma once
#ifndef LOOK_AROUND_H

#include "cursesdef.h"
#include "enums.h"

class look_around_t {
    public:
        catacurses::window w_info;
        tripoint start_point;
        bool has_first_point;
        bool select_zone;

    public:
        look_around_t() = default;

        tripoint invoke();
};

#endif
