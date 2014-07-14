// Monster movement code; essentially, the AI

#include "monster.h"
#include "map.h"
#include "game.h"
#include "line.h"
#include "rng.h"
#include "pldata.h"
#include "messages.h"
#include <stdlib.h>
#include "cursesdef.h"

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#define MONSTER_FOLLOW_DIST 8

bool monster::wander()
{
 return (plans.empty());
}

bool monster::can_move_to(int x, int y) const { return can_move_to(tripoint(x, y, _posz)); }
int monster::move_to(int x, int y, bool force) { return move_to(tripoint(x, y, _posz), force); }

bool monster::can_attack_across_z(const tripoint &p) const
{
    if (p.z == _posz) {
        return true;
    }
    if (std::abs(p.z - _posz) > 1) {
        return false;
    }
    if (p.z > _posz) { // attacking upwards
        if (!has_flag(MF_ATTACKS_UPWARDS)) {
            return false;
        }
        if (g->m.blocks_vertical_air_up(pos()) && g->m.blocks_vertical_air_down(p)) {
            return false;
        }
        return true;
    }
    // attacking down
    if (g->m.blocks_vertical_air_down(pos()) && g->m.blocks_vertical_air_up(p)) {
        return false;
    }
    return true;
}

bool monster::can_move_to(const tripoint &p) const
{
    if (std::abs(p.z - _posz) > 1) {
        // TODO: Z jumping down multiple z-level
        return false;
    }
    if (p.z > _posz) { // moving upwards
        if (!g->m.has_flag_ter("GOES_UP", tripoint(p.x, p.y, _posz))) {
            return false;
        }
    } else if (p.z < _posz) { // moving downwards
        // TODO: Z allow jumping down
        if (!g->m.has_flag_ter("GOES_DOWN", tripoint(p.x, p.y, _posz))) {
            if (has_flag(MF_STUMBLES) && !g->m.blocks_vertical_air_up(p)) {
                // Fall through thin air, if we stumble and if there is nothing that blocks us
                return true;
            }
            return false;
        }
    }
    if (g->m.has_flag_ter("NOFLOOR", p)) {
        return false;
    }
    if (g->m.move_cost(p) == 0) {
        return false;
    }
    if (!can_submerge() && g->m.has_flag(TFLAG_DEEP_WATER, p)) {
        return false;
    }
    if (has_flag(MF_DIGS) && !g->m.has_flag("DIGGABLE", p)) {
        return false;
    }
    if (has_flag(MF_AQUATIC) && !g->m.has_flag("SWIMMABLE", p)) {
        return false;
    }

    if (has_flag(MF_SUNDEATH) && g->is_in_sunlight(p)) {
        return false;
    }

    // various animal behaviours
    if (has_flag(MF_ANIMAL))
    {
        // don't enter sharp terrain unless tiny, or attacking
        if (g->m.has_flag("SHARP", p) && !(attitude(&(g->u)) == MATT_ATTACK ||
                                              type->size == MS_TINY))
            return false;

        // don't enter open pits ever unless tiny or can fly
        if (!(type->size == MS_TINY || has_flag(MF_FLIES)) &&
            (g->m.ter(p) == t_pit || g->m.ter(p) == t_pit_spiked || g->m.ter(p) == t_pit_glass))
            return false;

        // don't enter lava ever
        if (g->m.ter(p) == t_lava)
            return false;

        // don't enter fire or electricity ever
        const field &local_field = g->m.field_at(p);
        if (local_field.findField(fd_fire) || local_field.findField(fd_electricity))
            return false;
    }
    return true;
}

// Resets plans (list of squares to visit) and builds it as a straight line
// to the destination (x,y). t is used to choose which eligible line to use.
// Currently, this assumes we can see (x,y), so shouldn't be used in any other
// circumstance (or else the monster will "phase" through solid terrain!)
void monster::set_dest(const tripoint &p, int &t)
{
    plans = line_to(pos(), p, t);
}

// Move towards (x,y) for f more turns--generally if we hear a sound there
// "Stupid" movement; "if (wandx < posx) posx--;" etc.
void monster::wander_to(const tripoint &p, int f)
{
    wandx = p.x;
    wandy = p.y;
    wandz = p.z;
 wandf = f;
}

void monster::plan(const std::vector<int> &friendlies)
{
    int sightrange = g->light_level();
    int closest = -1;
    int dist = 1000;
    int tc = 0;
    int stc = 0;
    bool fleeing = false;

    if (friendly != 0) { // Target monsters, not the player!
        for (int i = 0, numz = g->num_zombies(); i < numz; i++) {
            monster *tmp = &(g->zombie(i));
            if (tmp->friendly == 0) {
                int d = rl_dist(pos(), tmp->pos());
                if (d < dist && g->m.sees(pos(), tmp->pos(), sightrange, tc)) {
                    closest = i;
                    dist = d;
                    stc = tc;
                }
            }
        }

        if (has_effect("docile")) {
            closest = -1;
        }

        if (closest >= 0) {
            set_dest(g->zombie(closest).pos(), stc);
        } else if (friendly > 0 && one_in(3)) {
            // Grow restless with no targets
            friendly--;
        } else if (friendly < 0 &&  sees_player( tc ) ) {
            if (rl_dist(pos(), g->u.pos()) > 2) {
                set_dest(g->u.pos(), tc);
            } else {
                plans.clear();
            }
        }
        return;
    }

    // If we can see, and we can see a character, move toward them or flee.
    if (can_see() && sees_player( tc ) ) {
        dist = rl_dist(pos(), g->u.pos());
        if (is_fleeing(g->u)) {
            // Wander away.
            fleeing = true;
            set_dest(tripoint(_posx * 2 - g->u.posx, _posy * 2 - g->u.posy, _posz), tc);
        } else {
            // Chase the player.
            closest = -2;
            stc = tc;
        }
    }

    for (size_t i = 0; i < g->active_npc.size(); i++) {
        npc *me = (g->active_npc[i]);
        int medist = rl_dist(pos(), me->pos());
        if ((medist < dist || (!fleeing && is_fleeing(*me))) &&
                (can_see() &&
                g->m.sees(pos(), me->pos(), sightrange, tc))) {
            if (is_fleeing(*me)) {
                fleeing = true;
                set_dest(tripoint(_posx * 2 - me->posx, _posy * 2 - me->posy, _posz), tc);\
            } else {
                closest = i;
                stc = tc;
            }
            dist = medist;
        }
    }

    if (!fleeing) {
        fleeing = attitude() == MATT_FLEE;
        if (can_see()) {
            for( auto &friendlie : friendlies ) {
                const int i = friendlie;
                monster *mon = &(g->zombie(i));
                int mondist = rl_dist(pos(), mon->pos());
                if (mondist < dist &&
                        g->m.sees(pos(), mon->pos(), sightrange, tc)) {
                    dist = mondist;
                    if (fleeing) {
                        wandx = posx() * 2 - mon->posx();
                        wandy = posy() * 2 - mon->posy();
                        wandf = 40;
                    } else {
                        closest = -3 - i;
                        stc = tc;
                    }
                }
            }
        }

        if (closest == -2) {
            if (one_in(2)) {//random for the diversity of the trajectory
                ++stc;
            } else {
                --stc;
            }
            set_dest(g->u.pos(), stc);
        } else if (closest <= -3) {
            set_dest(g->zombie(-3 - closest).pos(), stc);
        } else if (closest >= 0) {
            set_dest(g->active_npc[closest]->pos(), stc);
        }
    }
    // If we're not adjacent to the start of our plan path, don't act on it.
    // This is to catch when we had pre-existing invalid plans and
    // made it through the function without changing them.
    if( !plans.empty() && square_dist(pos().x, pos().y,
                                      plans.front().x, plans.front().y ) > 1 ) {
        plans.clear();
    }
}

// General movement.
// Currently, priority goes:
// 1) Special Attack
// 2) Sight-based tracking
// 3) Scent-based tracking
// 4) Sound-based tracking
void monster::move()
{
    // We decrement wandf no matter what.  We'll save our wander_to plans until
    // after we finish out set_dest plans, UNLESS they time out first.
    if (wandf > 0) {
        wandf--;
    }

    //Hallucinations have a chance of disappearing each turn
    if (is_hallucination() && one_in(25)) {
        die( nullptr );
        return;
    }

    //The monster can consume objects it stands on. Check if there are any.
    //If there are. Consume them.
    if( !is_hallucination() && has_flag( MF_ABSORBS ) ) {
        if(!g->m.i_at(pos()).empty()) {
            add_msg(_("The %s flows around the objects on the floor and they are quickly dissolved!"), name().c_str());
            std::vector<item> items_absorbed = g->m.i_at(pos());
            for( auto &elem : items_absorbed ) {
                hp += elem.volume(); // Yeah this means it can get more HP than normal.
            }
            g->m.i_clear(pos());
        }
    }

    // First, use the special attack, if we can!
    for (size_t i = 0; i < sp_timeout.size(); ++i) {
        if (sp_timeout[i] > 0) {
            sp_timeout[i]--;
        }

        if( sp_timeout[i] == 0 && (friendly == 0 || has_flag(MF_FRIENDLY_SPECIAL)) &&
            !has_effect("pacified") ) {
            mattack ma;
            if(!is_hallucination()) {
                (ma.*type->sp_attack[i])(this, i);
            }
        }
    }
    if (moves < 0) {
        return;
    }
    if (!move_effects()) {
        moves = 0;
        return;
    }
    if (has_flag(MF_IMMOBILE)) {
        moves = 0;
        return;
    }
    if (has_effect("stunned")) {
        stumble(false);
        moves = 0;
        return;
    }
    if (friendly != 0) {
        if (friendly > 0) {
            friendly--;
        }
        friendly_move();
        return;
    }

    bool moved = false;
    tripoint next;

    monster_attitude current_attitude = attitude();
    if (friendly == 0) {
        current_attitude = attitude(&(g->u));
    }
    // If our plans end in a player, set our attitude to consider that player
    if (!plans.empty()) {
        if (plans.back() == g->u.pos()) {
            current_attitude = attitude(&(g->u));
        } else {
            for (auto &i : g->active_npc) {
                if (plans.back() == i->pos()) {
                    current_attitude = attitude(i);
                }
            }
        }
    }

    if (current_attitude == MATT_IGNORE ||
          (current_attitude == MATT_FOLLOW && plans.size() <= MONSTER_FOLLOW_DIST)) {
        moves -= 100;
        stumble(false);
        return;
    }

    if (can_follow_plan()) {
        // CONCRETE PLANS - Most likely based on sight
        next = plans[0];
        moved = true;
    } else if (has_flag(MF_SMELLS)) {
        // No sight... or our plans are invalid (e.g. moving through a transparent, but
        //  solid, square of terrain).  Fall back to smell if we have it.
        plans.clear();
        tripoint tmp = scent_move();
        if (tmp != pos()) {
            next = tmp;
            moved = true;
        }
    }
    if (wandf > 0 && !moved) { // No LOS, no scent, so as a fall-back follow sound
        plans.clear();
        tripoint tmp = wander_next();
        if (tmp != pos()) {
            next = tmp;
            moved = true;
        }
    }

    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if (moved) { // Actual effects of moving to the square we've chosen
        // Note: The below works because C++ in A() || B() won't call B() if A() is true
        bool did_something = attack_at(next) || bash_at(next) || move_to(next);
        if(!did_something) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
    } else {
        moves -= 100;
    }

    // If we're close to our target, we get focused and don't stumble
    if ((has_flag(MF_STUMBLES) && (plans.size() > 3 || plans.empty())) ||
          !moved) {
        stumble(moved);
    }
}

bool monster::can_follow_plan() {
    if (plans.empty()) {
        return false;
    }
    const tripoint &next = plans[0];
    const int mondex = g->mon_at(next);
    if (mondex != -1 && g->zombie(mondex).friendly == 0 && !has_flag(MF_ATTACKMON)) {
        // There is a hostile (towards the player) monster there
        // and this monster does not have the ATTACKMON flag.
        return false;
    }
    if (next == g->u.pos() && can_attack_across_z(next)) {
        if (next.z == _posz) {
            return true;
        }
        // nope, player is above / below us, check if we can attack it
    }
    if( ( has_flag(MF_BASHES) || has_flag(MF_BORES) ) && g->m.bash_rating(bash_estimate(), next) > 0) {
        if (next.z == _posz) {
            return true;
        }
    }
    if (can_move_to(next)) {
        return true;
    }
    if (next.z != _posz) {
        // We attempt to make a z-level change, but we can not move there
        // (mayby there are no stairs?), try to move on the same z-level
        plans[0].z = _posz;
        if (plans[0] == pos()) {
            plans.erase(plans.begin());
        }
        return can_follow_plan();
    }
    return false;
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps(const tripoint &p)
{
 if (made_footstep)
  return;
 if (has_flag(MF_FLIES))
  return; // Flying monsters don't have footsteps!
 made_footstep = true;
 int volume = 6; // same as player's footsteps
 if (digging())
  volume = 10;
 switch (type->size) {
  case MS_TINY:
   return; // No sound for the tinies
  case MS_SMALL:
   volume /= 3;
   break;
  case MS_MEDIUM:
   break;
  case MS_LARGE:
   volume *= 1.5;
   break;
  case MS_HUGE:
   volume *= 2;
   break;
  default: break;
 }
 int dist = rl_dist(p, g->u.pos());
 g->add_footstep(p, volume, dist, this);
 return;
}

/*
Function: friendly_move
The per-turn movement and action calculation of any friendly monsters.
*/
void monster::friendly_move()
{
    tripoint next;
    bool moved = false;
    //If we successfully calculated a plan in the generic monster movement function, begin executing it.
    if (!plans.empty() && (plans[0] != g->u.pos()) &&
        (can_move_to(plans[0]) ||
         ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
          g->m.bash_rating(bash_estimate(), plans[0]) >= 0))) {
        next = plans[0];
        plans.erase(plans.begin());
        moved = true;
    } else {
        //Otherwise just stumble around randomly until we formulate a plan.
        moves -= 100;
        stumble(moved);
    }
    if (moved) {
        bool did_something = attack_at(next) || bash_at(next) || move_to(next);

        //If all else fails in our plan (an issue with pathfinding maybe) stumble around instead.
        if(!did_something) {
            stumble(moved);
            moves -= 100;
        }
    }
}

tripoint monster::scent_move()
{
    std::vector<tripoint> smoves;

    int maxsmell = 10; // Squares with smell 0 are not eligible targets.
    int smell_threshold = 200; // Squares at or above this level are ineligible.
    if (has_flag(MF_KEENNOSE)) {
        maxsmell = 1;
        smell_threshold = 400;
    }
    int minsmell = 9999;
    int smell;
    const bool fleeing = is_fleeing(g->u);
    if( !fleeing && g->scent( pos() ) > smell_threshold ) {
        return pos();
    }
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            const tripoint np(_posx + x, _posy + y, _posz);
            smell = g->scent(np);
            int mon = g->mon_at(np);
            if( (mon == -1 || g->zombie(mon).friendly != 0 || has_flag(MF_ATTACKMON)) &&
                (can_move_to(np) || np == g->u.pos() ||
                 ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                 g->m.bash_rating(bash_estimate(), np) >= 0))) {
                if ((!fleeing && smell > maxsmell) || (fleeing && smell < minsmell)) {
                    smoves.clear();
                    smoves.push_back(np);
                    maxsmell = smell;
                    minsmell = smell;
                } else if ((!fleeing && smell == maxsmell) || (fleeing && smell == minsmell)) {
                    smoves.push_back(np);
                }
            }
        }
    }
    if (!smoves.empty()) {
        int nextsq = rng(0, smoves.size() - 1);
        return smoves[nextsq];
    }
    return pos();
}

bool monster::can_move_to_or_bash_or_is_u(const tripoint &p) const {
    const int bashskill = bash_estimate();
    return can_move_to(p) || p == g->u.pos() || ((has_flag(MF_BASHES) || has_flag(MF_BORES)) && g->m.bash_rating(bashskill, p) > 0);
}

tripoint monster::wander_next()
{
    bool xbest = abs(wandy - posy()) > abs(wandx - posx());
    int x = posx(), x2 = posx() - 1, x3 = posx() + 1;
    int y = posy(), y2 = posy() - 1, y3 = posy() + 1;
    if (wandx < posx()) {
        x--; x2++;
    }
    if (wandx > posx()) {
        x++; x2++; x3 -= 2;
    }
    if (wandy < posy()) {
        y--; y2++;
    }
    if (wandy > posy()) {
        y++; y2++; y3 -= 2;
    }

    const tripoint n1(x, y, _posz);
    const tripoint n2(x, y2, _posz);
    const tripoint n3(x2, y, _posz);
    const tripoint n4(x, y3, _posz);
    const tripoint n5(x3, y, _posz);
    if (can_move_to_or_bash_or_is_u(n1)) {
        return n1;
    }
    // Note: different order of checks depending on xbest
    if (xbest) {
        if (can_move_to_or_bash_or_is_u(n2)) {
            return n2;
        } else if (can_move_to_or_bash_or_is_u(n3)) {
            return n3;
        } else if (can_move_to_or_bash_or_is_u(n4)) {
            return n4;
        } else if (can_move_to_or_bash_or_is_u(n5)) {
            return n5;
        }
    } else {
        if (can_move_to_or_bash_or_is_u(n3)) {
            return n3;
        } else if (can_move_to_or_bash_or_is_u(n2)) {
            return n2;
        } else if (can_move_to_or_bash_or_is_u(n5)) {
            return n5;
        } else if (can_move_to_or_bash_or_is_u(n4)) {
            return n4;
        }
    }
    return pos();
}

int monster::calc_movecost(const tripoint &p1, const tripoint &p2) const
{
    int movecost = 0;
    float diag_mult = (trigdist && p1.x != p2.x && p1.y != p2.y) ? 1.41 : 1;

    // Digging and flying monsters ignore terrain cost
    if (has_flag(MF_FLIES) || (digging() && g->m.has_flag("DIGGABLE", p2))) {
        movecost = 100 * diag_mult;
    // Swimming monsters move super fast in water
    } else if (has_flag(MF_SWIMS)) {
        if (g->m.has_flag("SWIMMABLE", p1)) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost(p1);
        }
        if (g->m.has_flag("SWIMMABLE", p2)) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost(p2);
        }
        movecost *= diag_mult;
    // No-breathe monsters have to walk underwater slowly
    } else if (can_submerge()) {
        if (g->m.has_flag("SWIMMABLE", p1)) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost(p1);
        }
        if (g->m.has_flag("SWIMMABLE", p2)) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost(p2);
        }
        movecost *= diag_mult / 2;
    // All others use the same calculation as the player
    } else {
        movecost = (g->m.combined_movecost(p1, p2));
    }

    return movecost;
}

/*
 * Return points of an area extending 1 tile to either side and
 * (maxdepth) tiles behind basher.
 */
std::vector<point> get_bashing_zone( const point &bashee, const point &basher, int maxdepth ) {
    std::vector<point> direction;
    direction.push_back(bashee);
    direction.push_back(basher);
    // Draw a line from the target through the attacker.
    std::vector<point> path = continue_line( direction, maxdepth );
    // Remove the target.
    path.insert( path.begin(), basher );
    std::vector<point> zone;
    // Go ahead and reserve enough room for all the points since
    // we know how many it will be.
    zone.reserve( 3 * maxdepth );
    point previous = bashee;
    for( point p : path ) {
        std::vector<point> swath = squares_in_direction( previous.x, previous.y, p.x, p.y );
        for( point q : swath ) {
            zone.push_back( q );
        }
        previous = p;
    }
    return zone;
}

int monster::bash_at(const tripoint &p) {

    if (has_effect("pacified")) return 0;

    //Hallucinations can't bash stuff.
    if(is_hallucination()) {
       return 0;
    }
    bool try_bash = !can_move_to(p) || one_in(3);
    bool can_bash = g->m.is_bashable(p) && (has_flag(MF_BASHES) || has_flag(MF_BORES));
    if( try_bash && can_bash ) {
        int bashskill = group_bash_skill( p );

        g->m.bash( p, bashskill );
        moves -= 100;
        return 1;
    }
    return 0;
}

int monster::bash_estimate() const
{
    int estimate = bash_skill();
    if( has_flag(MF_GROUP_BASH) ) {
        // Right now just give them a boost so they try to bash a lot of stuff.
        // TODO: base it on number of nearby friendlies.
        estimate += 20;
    }
    return estimate;
}

int monster::bash_skill() const
{
    int ret = type->melee_dice * type->melee_sides; // IOW, the critter's max bashing damage
    if (has_flag(MF_BORES)) {
        ret *= 15; // This is for stuff that goes through solid rock: minerbots, dark wyrms, etc
    } else if (has_flag(MF_DESTROYS)) {
        ret *= 2.5;
    }
    return ret;
}

int monster::group_bash_skill( const tripoint &target )
{
    if( !has_flag(MF_GROUP_BASH) ) {
        return bash_skill();
    }
    int bashskill = 0;

    // pileup = more bashskill, but only help bashing mob directly infront of target
    const int max_helper_depth = 5;
    const std::vector<point> bzone = get_bashing_zone( point( target.x, target.x ), point( xpos(), ypos() ), max_helper_depth );

    for( point candidate : bzone ) {
        // Drawing this line backwards excludes the target and includes the candidate.
        std::vector<point> path_to_target = line_to( target.x, target.y,
                                                     candidate.x, candidate.y, 0);
        bool connected = true;
        int mondex = -1;
        for( point in_path : path_to_target ) {
            // If any point in the line from zombie to target is not a cooperating zombie,
            // it can't contribute.
            mondex = g->mon_at( in_path );
            if( mondex == -1 ) {
                connected = false;
                break;
            }
            monster &helpermon = g->zombie( mondex );
            if( !helpermon.has_flag(MF_GROUP_BASH) || helpermon.is_hallucination() ) {
                connected = false;
                break;
            }
        }
        if( !connected ) {
            continue;
        }
        // If we made it here, the last monster checked was the candidate.
        monster &helpermon = g->zombie( mondex );
        // Contribution falls off rapidly with distance from target.
        bashskill += helpermon.bash_skill() / rl_dist( candidate, target );
    }

    return bashskill;
}

int monster::attack_at(const tripoint &p) {

    if (has_effect("pacified")) return 0;

    int mondex = g->mon_at(p);
    int npcdex = g->npc_at(p);

    if(p == g->u.pos()) {
        melee_attack(g->u);
        return 1;
    }

    if(mondex != -1) {
        // Currently, there are only pro-player and anti-player groups,
        // this makes it easy for us.
        monster& mon = g->zombie(mondex);

        // Don't attack yourself.
        if(&mon == this) {
            return 0;
        }

        // Special case: Target is hallucination
        if(mon.is_hallucination()) {
            mon.die( nullptr );

            // We haven't actually attacked anything, i.e. we can still do things.
            // Hallucinations(obviously) shouldn't affect the way real monsters act.
            return 0;
        }

        // With no melee dice, we can't attack, but we had to process until here
        // because hallucinations require no melee dice to destroy.
        if(type->melee_dice <= 0) {
            return 0;
        }

        bool is_enemy = mon.friendly != friendly;
        is_enemy = is_enemy || has_flag(MF_ATTACKMON); // I guess the flag means all monsters are enemies?

        if(is_enemy) {
            hit_monster(mon);
            return 1;
        }
    } else if(npcdex != -1  && type->melee_dice > 0) {
        // For now we're always attacking NPCs that are getting into our
        // way. This is consistent with how it worked previously, but
        // later on not hitting allied NPCs would be cool.
        melee_attack(*g->active_npc[npcdex]);
        return 1;
    }

    // Nothing to attack.
    return 0;
}

int monster::move_to(const tripoint &p, bool force)
{
    // Make sure that we can move there, unless force is true.
    if(!force) if(!g->is_empty(p) || !can_move_to(p)) {
        return 0;
    }

    if (!plans.empty()) {
        plans.erase(plans.begin());
    }

    if (!force) {
        moves -= calc_movecost(pos(), p);
    }

    //Check for moving into/out of water
    bool was_water = g->m.is_divable(pos());
    bool will_be_water = g->m.is_divable(p);

    if(was_water && !will_be_water && g->u_see(p)) {
        //Use more dramatic messages for swimming monsters
        add_msg(m_warning, _("A %s %s from the %s!"), name().c_str(),
                   has_flag(MF_SWIMS) || has_flag(MF_AQUATIC) ? _("leaps") : _("emerges"),
                   g->m.tername(pos()).c_str());
    } else if(!was_water && will_be_water && g->u_see(p)) {
        add_msg(m_warning, _("A %s %s into the %s!"), name().c_str(),
                   has_flag(MF_SWIMS) || has_flag(MF_AQUATIC) ? _("dives") : _("sinks"),
                   g->m.tername(p).c_str());
    }

    setpos(p);
    footsteps(p);
    if(is_hallucination()) {
        //Hallucinations don't do any of the stuff after this point
        return 1;
    }
    if (type->size != MS_TINY && g->m.has_flag("SHARP", pos()) && !one_in(4)) {
        apply_damage( nullptr, bp_torso, rng( 2, 3 ) );
    }
    if (type->size != MS_TINY && g->m.has_flag("ROUGH", pos()) && one_in(6)) {
        apply_damage( nullptr, bp_torso, rng( 1, 2 ) );
    }
    if (g->m.has_flag("UNSTABLE", p)) {
        add_effect("bouldering", 1, num_bp, true);
    } else if (has_effect("bouldering")) {
        remove_effect("bouldering");
    }
    if (!digging() && !has_flag(MF_FLIES) &&
          g->m.tr_at(pos()) != tr_null) { // Monster stepped on a trap!
        trap* tr = traplist[g->m.tr_at(pos())];
        if (dice(3, type->sk_dodge + 1) < dice(3, tr->get_avoidance())) {
            tr->trigger(this, pos());
        }
    }
    // Diggers turn the dirt into dirtmound
    if (digging()){
        int factor = 0;
        switch (type->size) {
        case MS_TINY:
            factor = 100;
            break;
        case MS_SMALL:
            factor = 30;
            break;
        case MS_MEDIUM:
            factor = 6;
            break;
        case MS_LARGE:
            factor = 3;
            break;
        case MS_HUGE:
            factor = 1;
            break;
        }
        if (has_flag(MF_VERMIN)) {
            factor *= 100;
        }
        if (one_in(factor)) {
            g->m.ter_set(pos(), t_dirtmound);
        }
    }
    // Acid trail monsters leave... a trail of acid
    if (has_flag(MF_ACIDTRAIL)){
        g->m.add_field(pos(), fd_acid, 3, 0);
    }

    if (has_flag(MF_SLUDGETRAIL)) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                const int fstr = 3 - (abs(dx) + abs(dy));
                if (fstr >= 2) {
                    g->m.add_field(tripoint(_posx + dx, _posy + dy, _posz), fd_sludge, fstr, 0);
                }
            }
        }
    }
    if (has_flag(MF_LEAKSGAS)){
        if (one_in(6)){
        g->m.add_field(posx() + rng(-1,1), posy() + rng(-1, 1), fd_toxic_gas, 3);
        }
    }

    return 1;
}

/* Random walking even when we've moved
 * To simulate zombie stumbling and ineffective movement
 * Note that this is sub-optimal; stumbling may INCREASE a zombie's speed.
 * Most of the time (out in the open) this effect is insignificant compared to
 * the negative effects, but in a hallway it's perfectly even
 */
void monster::stumble(bool moved)
{
 // don't stumble every turn. every 3rd turn, or 8th when walking.
 if((moved && !one_in(8)) || !one_in(3))
 {
     return;
 }

 std::vector <tripoint> valid_stumbles;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   tripoint np(_posx + i, _posy + j, _posz);
   if ((i || j) && can_move_to(np) &&
       //Stop zombies and other non-breathing monsters wandering INTO water
       //(Unless they can swim/are aquatic)
       //But let them wander OUT of water if they are there.
       !(has_flag(MF_NO_BREATHE) && !has_flag(MF_SWIMS) && !has_flag(MF_AQUATIC)
           && g->m.has_flag("SWIMMABLE", np)
           && !g->m.has_flag("SWIMMABLE", pos()))
           && np != g->u.pos() &&
       (g->mon_at(np) == -1)) {
    valid_stumbles.push_back(np);
   }
  }
 }
 if (valid_stumbles.empty()) //nowhere to stumble?
 {
     return;
 }

 int choice = rng(0, valid_stumbles.size() - 1);
 const tripoint cc = valid_stumbles[choice];

 moves -= calc_movecost(pos(), cc);
 setpos(cc);

 // Here we have to fix our plans[] list,
 // acquiring a new path to the previous target.
 // target == either end of current plan, or the player.
 int tc;
 if (!plans.empty()) {
  if (g->m.sees(pos(), plans.back(), -1, tc))
   set_dest(plans.back(), tc);
  else if (sees_player( tc ))
   set_dest(g->u.pos(), tc);
  else //durr, i'm suddenly calm. what was i doing?
   plans.clear();
 }
}

// TODO: Z (parameters)
void monster::knock_back_from(int x, int y)
{
 if (x == posx() && y == posy())
  return; // No effect
    if( is_hallucination() ) {
        die( nullptr );
        return;
    }
 tripoint to = pos();
 if (x < posx())
  to.x++;
 if (x > posx())
  to.x--;
 if (y < posy())
  to.y++;
 if (y > posy())
  to.y--;

 bool u_see = g->u_see(to);

// First, see if we hit another monster
 int mondex = g->mon_at(to);
    if( mondex != -1 && g->zombie( mondex ).is_hallucination() ) {
        // hallucinations should not affect real monsters. If they interfer, just remove them.
        g->zombie( mondex ).die( this );
        mondex = -1;
    }
 if (mondex != -1) {
  monster *z = &(g->zombie(mondex));
  apply_damage( z, bp_torso, z->type->size );
  add_effect("stunned", 1);
  // TODO: Z
  if (type->size > 1 + z->type->size) {
   z->knock_back_from(posx(), posy()); // Chain reaction!
   z->apply_damage( this, bp_torso, type->size );
   z->add_effect("stunned", 1);
  } else if (type->size > z->type->size) {
   z->apply_damage( this, bp_torso, type->size );
   z->add_effect("stunned", 1);
  }

  if (u_see)
   add_msg(_("The %s bounces off a %s!"), name().c_str(), z->name().c_str());

  return;
 }

 int npcdex = g->npc_at(to);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  apply_damage( p, bp_torso, 3 );
  add_effect("stunned", 1);
  p->deal_damage( this, bp_torso, damage_instance( DT_BASH, type->size ) );
  if (u_see)
   add_msg(_("The %s bounces off %s!"), name().c_str(), p->name.c_str());

  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.ter_at(to).has_flag(TFLAG_DEEP_WATER)) {
  if (g->m.has_flag("LIQUID", to) && can_drown()) {
   die( nullptr );
   if (u_see) {
    add_msg(_("The %s drowns!"), name().c_str());
   }

  } else if (has_flag(MF_AQUATIC)) { // We swim but we're NOT in water
   die( nullptr );
   if (u_see) {
    add_msg(_("The %s flops around and dies!"), name().c_str());
   }
  }
 }

 if (g->m.move_cost(to) == 0) {

   // It's some kind of wall.
   apply_damage( nullptr, bp_torso, type->size );
   add_effect("stunned", 2);
   if (u_see) {
    add_msg(_("The %s bounces off a %s."), name().c_str(),
               g->m.tername(to).c_str());
   }

 } else { // It's no wall
  setpos(to);
 }
}
