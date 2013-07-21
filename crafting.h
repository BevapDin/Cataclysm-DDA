#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include "itype.h"
#include "skill.h"
#include "rng.h"
#include "item.h"

#define MAX_DISPLAYED_RECIPES 18

typedef std::string craft_cat;

struct component
{
 itype_id type;
 int count;
 int available; // -1 means the player doesn't have the item, 1 means they do,
            // 0 means they have item but not enough for both tool and component
 component() { type = "null"; count = 0; available = -1;}
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT), available(-1) {}
};

struct recipe {
  std::string ident;
  int id;
  itype_id result;
  craft_cat cat;
  Skill *sk_primary;
  Skill *sk_secondary;
  int difficulty;
  int time;
  bool reversible; // can the item be disassembled?
  bool autolearn; // do we learn it just by leveling skills?
  int learn_by_disassembly; // what level (if any) do we learn it by disassembly?

  std::vector<std::vector<component> > tools;
  std::vector<std::vector<component> > components;

  recipe() {
    id = 0;
    result = "null";
    sk_primary = NULL;
    sk_secondary = NULL;
    difficulty = 0;
    time = 0;
    reversible = false;
    autolearn = false;
  }

recipe(std::string pident, int pid, itype_id pres, craft_cat pcat, std::string &p1,
       std::string &p2, int pdiff, int ptime, bool preversible, bool pautolearn,
       int plearn_dis) :
  ident (pident), id (pid), result (pres), cat(pcat), difficulty (pdiff), time (ptime),
  reversible (preversible), autolearn (pautolearn), learn_by_disassembly (plearn_dis)
  {
    sk_primary = p1.size()?Skill::skill(p1):NULL;
    sk_secondary = p2.size()?Skill::skill(p2):NULL;
  }
};

class vehicle;
class crafting_inventory_t {
public:
	struct item_on_map {
		point origin;
		std::vector<item> *items;
		item_on_map(const point &p, std::vector<item> *i) : origin(p), items(i) { }
	};
	struct item_in_vehicle {
		vehicle *veh;
		int part_num;
		std::vector<item> *items;
		item_in_vehicle(vehicle *v, int p, std::vector<item> *i) : veh(v), part_num(p), items(i) { }
	};
	/*
	struct item_from_bionic {
		item it;
	};
	*/
	typedef item item_from_bionic;
	struct item_from_souround {
		point origin;
		item it;
		item_from_souround(const point &p, const item &i) : origin(p), it(i) { }
	};

	player *p;

	std::list<item_on_map> on_map;
	std::list<item_in_vehicle> in_veh;
	std::list<item_from_bionic> bionic;
	std::list<item_from_souround> souround;


	std::list<item> use_amount(const itype_id &type, const int amount, const bool use_container = false);
	std::list<item> use_charges(const itype_id &type, const int amount);

	bool has_amount (const itype_id &type, int quantity) const;
	bool has_charges(const itype_id &type, int quantity) const;

	int charges_of(const itype_id &type) const;
	int amount_of(const itype_id &type) const;

	bool map_has_amount (const itype_id &type, int quantity) const;
	bool map_has_charges(const itype_id &type, int quantity) const;

	int map_charges_of(const itype_id &type) const;
	int map_amount_of(const itype_id &type) const;

	std::list<item> consume_items(std::vector<component> components);
	void consume_tools(std::vector<component> tools, bool force_available);
	std::list<item> consume_items(const itype_id &type, int count);
	void consume_tools(const itype_id &type, int charges, bool force_available);

	// See veh_interact.cpp
	item consume_vpart_item (game *g, const itype_id &itid);

	std::vector<const item*> all_items_by_type(const itype_id &itid) const;

	crafting_inventory_t(game *g, player *p);

protected:
	typedef int(*CountFunction)(const itype_id &, const item &);

	enum {
		S_PLAYER  = 1<< 0,
		S_MAP     = 1<< 1,
		S_VEHICLE = 1<< 2,
		S_ALL     = 0xFFFFFFFF
	} source_flags;

	/** Returns the number of charges of this item type. Stops counting when reaching max charges */
	int count(const itype_id &type, CountFunction function, int max = -1, int sources = S_ALL) const;

	static int charges_of(const itype_id &type, const item &it);
	static int amount_of(const itype_id &type, const item &it);

	void add_bio_toolset(const std::string &tool, calendar &turn);
	void form_from_map(game *g, point origin, int distance);
};


typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

#endif
