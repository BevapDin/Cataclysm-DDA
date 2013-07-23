#ifndef _MONGROUP_H_
#define _MONGROUP_H_

#include "mtype.h"
#include <vector>
#include <map>

typedef std::map<mon_id, std::pair<int,int> > FreqDef;
typedef FreqDef::iterator FreqDef_iter;

struct MonsterGroup
{
    std::string name;
    mon_id defaultMonster;
    FreqDef  monsters;
};

/*enum MonsterGroupType
{
    GROUP_NULL = 0,
    GROUP_FOREST,
    GROUP_ANT,
    GROUP_BEE,
    GROUP_WORM,
    GROUP_ZOMBIE,
    GROUP_TRIFFID,
    GROUP_FUNGI,
    GROUP_GOO,
    GROUP_CHUD,
    GROUP_SEWER,
    GROUP_SWAMP,
    GROUP_LAB,
    GROUP_NETHER,
    GROUP_SPIRAL,
    GROUP_VANILLA,
    GROUP_SPIDER,
    GROUP_ROBOT,
    GROUP_COUNT
};*/

struct mongroup {
 std::string type;
 int posx, posy, posz;
 unsigned char radius;
 unsigned int population;
 bool dying;
 bool diffuse;   // group size ind. of dist. from center and radius invariant
 mongroup(std::string ptype, int pposx, int pposy, int pposz, unsigned char prad,
          unsigned int ppop) {
  type = ptype;
  posx = pposx;
  posy = pposy;
  posz = pposz;
  radius = prad;
  population = ppop;
  dying = false;
  diffuse = false;
 }
 bool is_safe() { return (type == "GROUP_NULL" || type == "GROUP_FOREST"); };
};

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

class MonsterGroupManager
{
    public:
        static void LoadJSONGroups() throw (std::string);
        static mon_id GetMonsterFromGroup(std::string, std::vector <mtype*> *,
                                          int *quantity = 0, int turn = -1);
        static bool IsMonsterInGroup(std::string, mon_id);
        static std::string Monster2Group(mon_id);
        static std::vector<mon_id> GetMonstersFromGroup(std::string);
        static MonsterGroup GetMonsterGroup(std::string group);

    private:
        static std::map<std::string, MonsterGroup> monsterGroupMap;
};
#endif
