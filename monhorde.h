#ifndef _MONHORDE_H_
#define _MONHORDE_H_

#include "mtype.h"
#include <string>

struct monhorde {
 std::string type;
 int posx, posy, posz;
 /**
  * The population sorted into different types of
  * zombies.
  */
 union {
	int population[2];
	struct {
		int pop_normal;
		int pop_master;
	};
 };

 int getPopulation() const;
 bool isEmpty() const;
 /** 
  * Target where the horde is moving towards.
  * If same as position, the horde does not move at all.
  */
 int targetx;
 int targety;
 /**
  * When the horde started to move towards the target. If
  * the target changes, this must be reset. If the horde
  * gained enough movepoint (e.g. (game::turn - last_move) > SEEX)
  * the position is changed.
  */
 int last_move;
 // See monster::wandf
 int wandf;
 enum flags_e {
 };
 int flags;
 monhorde(std::string ptype, int pposx, int pposy, int pposz,
          unsigned int ppop, int lm) {
  type = ptype;
  posx = targetx = pposx;
  posy = targety = pposy;
  posz = pposz;
  pop_normal = ppop - 1;
  pop_master = 1;
  last_move = lm;
  wandf = 0;
  flags = 0;
 }
 monhorde(std::string ptype, int x, int y, int z) : type(ptype), posx(x), posy(y), posz(z) {
  pop_normal = 0;
  pop_master = 0;
  targetx = 0;
  targety = 0;
  last_move = 0;
  wandf = 0;
  flags = 0;
 }
 /**
  * Called when the horde does not move because it
  * has no current target.
  */
 void rest_here(game &g);
 /**
  * The population of a horde may change according to the
  * terrain (overmap-terrain) it is currently on.
  * E.g. in cities the population would grow as zombies
  * from the city will merge into the horde.
  * On empty land it may not change at all.
  * In forrest or swamps it may decrese.
  */
 void change_pop_by_terrain(game &g);
 
 /**
  * Called when noise appears.
  * x and y use the same system as posx and posy (submap
  * index in current overmap).
  */
 void attract_by_sound(int x, int y, int vol);
 
 /**
  * Try to despawn the monster back into this horde.
  * May fail for farious reasons.
  * @return false if the monster has _not_ been added to the horde.
  */
 bool despawn(monster &m, game &g);
 
 /**
  * Try to merge another horde into this one. This is called
  * when two hordes appear on the same submap.
  * @return true if the other horde can be deleted.
  * Other wise the other horde must remain even if there are
  * several hordes on the same place.
  */
 bool merge(monhorde &other);
 /**
  * Let the horde move according to its target or to
  * a new, random target or whatever.
  * @return false if the horde did not move at all, otherwise true.
  */
 bool move(game &g);
};

#endif
