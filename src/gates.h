#pragma once
#ifndef GATES_H
#define GATES_H

#include "player.h"
#include "enums.h"
#include "json.h"

template<typename T>
class int_id;
struct ter_id;
using ter_id = int_id<ter_t>;

namespace gates
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

/** opens the gate via player's activity */
void open_gate( const tripoint &pos, player &p );
/** opens the gate immediately */
void open_gate( const tripoint &pos );

template<typename T>
class gate_action {
    private:
        int_id<T> source;
        int_id<T> target;

    public:
        bool is_locked() const;
        bool is_open() const;
        bool is_closed() const;
};

const gate_set &gate_set_from( ter_id t );
bool can_open( ter_id t );

};

namespace doors
{

/**
 * Handles deducting moves, printing messages (only non-NPCs cause messages), actually closing it,
 * checking if it can be closed, etc.
*/
void close_door( map &m, Character &who, const tripoint &closep );

};

#endif
