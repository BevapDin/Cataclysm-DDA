#pragma once
#ifndef CATA_SRC_CREATURE_ATTITUDE_H
#define CATA_SRC_CREATURE_ATTITUDE_H

/**
 * Simplified attitude towards any creature:
 * hostile - hate, want to kill, etc.
 * neutral - anything between.
 * friendly - avoid harming it, maybe even help.
 * any - any of the above, used in safemode_ui
 */
enum class creature_attitude : int {
    HOSTILE,
    NEUTRAL,
    FRIENDLY,
    ANY
};

#endif // CATA_SRC_CREATURE_ATTITUDE_H
