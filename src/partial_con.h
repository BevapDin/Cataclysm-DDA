#pragma once
#ifndef CATA_SRC_PARTIAL_CON
#define CATA_SRC_PARTIAL_CON

#include <list>

#include "type_id.h"
#include "item.h"

struct partial_con {
    int counter = 0;
    std::list<item> components;
    construction_id id = construction_id( -1 );
};

#endif // CATA_SRC_PARTIAL_CON
