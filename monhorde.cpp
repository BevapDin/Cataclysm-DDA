#include "game.h"
#include "mongroup.h"
#include "monhorde.h"
#include "monstergenerator.h"

#define MASTER_ZOMBIE_ID "mon_zombie_soldier"


overmap *game::getOverMap(int &omx, int &omy) {
 if (omx >= 0 && omx < OMAPX * 2 && omy >= 0 && omy < OMAPY * 2) {
  return cur_om;
 } else if ((omx < 0 || omx >= OMAPX * 2) && (omy < 0 || omy >= OMAPY * 2)) {
  if (omx < 0) omx += OMAPX * 2;
  else         omx -= OMAPX * 2;
  if (omy < 0) omy += OMAPY * 2;
  else         omy -= OMAPY * 2;
  return om_diag;
 } else if (omx < 0 || omx >= OMAPX * 2) {
  if (omx < 0) omx += OMAPX * 2;
  else         omx -= OMAPX * 2;
  return om_hori;
 } else if (omy < 0 || omy >= OMAPY * 2) {
  if (omy < 0) omy += OMAPY * 2;
  else         omy -= OMAPY * 2;
  return om_vert;
 } else {
  return NULL;
 }
}

bool monhorde::despawn(monster &m, game &g) {
	if(m.type->id == MASTER_ZOMBIE_ID) {
		pop_master++;
	} else {
		pop_normal++;
		pop_normal = std::min(pop_normal, SEEX * SEEY);
	}
	return true;
}

int monhorde::getPopulation() const {
	int sum = 0;
	for(size_t i = 0; i < sizeof(population) / sizeof(population[0]); i++) {
		sum += population[i];
	}
	return sum;
}

bool monhorde::isEmpty() const {
	return getPopulation() == 0;
}

bool game::try_despawn_to_horde(monster &m, int shiftx, int shifty) {
	// Hordes live on z-level 0 only! - at least for now
	if(levz != 0) {
		return false;
	}
	// Not a horde monster (not a zombie at all?)
	if(!MonsterGroupManager::IsMonsterInGroup("GROUP_WANDERING_ZOMBIE", m.type->id)) {
		return false;
	}
	// coords of submap-tile of the monster
	int lx = m.posx() / SEEX + levx + shiftx;
	int ly = m.posy() / SEEY + levy + shifty;
	
	add_msg("Despawn a horde member @ %d,%d", m.posx() - u.posx, m.posy() - u.posy); 
	
	typedef std::vector<monhorde> MHVec;
	MHVec &monhordes = cur_om->zh;
	for(size_t i = 0; i < monhordes.size(); i++) {
		monhorde &horde = monhordes[i];
		if(abs(horde.posx - lx) <= 1 && abs(horde.posy - ly) <= 1) {
			// Near enough
			if(horde.despawn(m, *this)) {
				return true;
			}
		}
	}
	// TODO: set the target of the monster to the target of the horde

	overmap *om = getOverMap(lx, ly);
	om->zh.push_back(monhorde("GROUP_WANDERING_ZOMBIE", lx, ly, 0, 0, (int) turn));
	om->zh.back().despawn(m, *this);
	
	return true;
}

// Note: gx, gy are global!
bool hordeNear(overmap &om, int gx, int gy, int mindist) {
	// Offset to monster horde positions, because
	// those are relativ to the overmap they are in,
	// but gx and gy are global.
	const int ox = om.pos().x;
	const int oy = om.pos().y;
	typedef std::vector<monhorde> MHVec;
	for(MHVec::iterator a = om.zh.begin(); a != om.zh.end(); ++a) {
		monhorde &horde = *a;
		// Don't spawn if there is already another horde nearby
		if(rl_dist(gx, gy, horde.posx + ox, horde.posy + oy) < mindist) {
			return true;
		}
	}
	return false;
}

void game::spawn_horde() {
	if(cur_om->zh.size() >= 1 && !one_in(HOURS(36))) {
		// Spawn a horde if there are  currently no hordes at all present
		// otherwise only spawn once about ever 12 hours
		return;
	}
	static const int min_dist_to_player = 10;
	static const int max_dist_to_player = 30;
	static const int min_dist_to_other = 12;
	// position of new horde, relativ to player!
	int x = rng(-max_dist_to_player, max_dist_to_player);
	int y = rng(-max_dist_to_player, max_dist_to_player);
	// Don't spawn if near player
	if(rl_dist(x, y, 0, 0) < min_dist_to_player) {
		return;
	}
	// Don't spawn if far away.
	if(rl_dist(x, y, 0, 0) > max_dist_to_player) {
		return;
	}
	// Now make their position relative to current overmap
	x += levx + int(MAPSIZE / 2);
	y += levy + int(MAPSIZE / 2);
	
	// global coordinates of new horde (absolut global!)
	int gx = x + cur_om->pos().x;
	int gy = y + cur_om->pos().y;
	// Check neighbour overmaps if another horde is nearby, if
	// so, cancel this new one (don't want to much hordes).
	if(hordeNear(*cur_om, gx, gy, min_dist_to_other)) {
		return;
	}
	if(hordeNear(*om_diag, gx, gy, min_dist_to_other)) {
		return;
	}
	if(hordeNear(*om_hori, gx, gy, min_dist_to_other)) {
		return;
	}
	if(hordeNear(*om_vert, gx, gy, min_dist_to_other)) {
		return;
	}

	add_msg("Horde spawned @ %d,%d (rel: %d,%d)", x, y, x - (levx + MAPSIZE / 2), y - (levy + MAPSIZE / 2));
	
	// get overmap where to place the horde in
	overmap *om = getOverMap(x, y);
	om->zh.push_back(monhorde("GROUP_WANDERING_ZOMBIE", x, y, 0, rng(10, 50), (int) turn));
}

template<typename T> inline T sign(const T &a) { return a < 0 ? -1 : (a > 0 ? +1 : 0); }

bool monhorde::merge(monhorde &other) {
	pop_master += other.pop_master;
	pop_normal += other.pop_normal;
	if(targetx != other.targetx || targety != other.targety) {
		wandf = 0; // reset to force a new target!
	}
	return true;
}

bool merge(std::vector<monhorde> &monhordes, int p) {
	// This horde should be merged into another one.
	monhorde &hordeX = monhordes[p];
	for(size_t i = 0; i < p; i++) {
		monhorde &horde = monhordes[i];
		if(horde.posx != hordeX.posx || horde.posy != hordeX.posy) {
			continue;
		}
		if(horde.merge(hordeX)) {
			g->add_msg("Horde merged @ (rel: %d,%d): %d", horde.posx - (g->levx + MAPSIZE / 2), horde.posy - (g->levy + MAPSIZE / 2), horde.getPopulation());
			return true;
		}
	}
	return false;
}

void monhorde::rest_here(game &g) {
	if(pop_normal > 0 && pop_master == 0) {
		pop_normal--;
	}
	change_pop_by_terrain(g);
	if(pop_normal == 0 && pop_master > 0) {
		pop_master--;
	}
}

void monhorde::change_pop_by_terrain(game &g) {
	// Out of bounds?
	if(posx < 0 || posy < 0 || posx >= OMAPX * 2 || posy >= OMAPY * 2) {
		return;
	}
	oter_id ter = g.cur_om->ter(posx / 2, posy / 2, posz);
	if(ter == ot_crater) {
		if(one_in(10)) {
			pop_normal++;
		}
	} else if(ter == ot_field) {
		// Nothing for now
	} else if(ter == ot_forest) {
		if(one_in(40)) { // Ups - we lost one
			pop_normal--;
		}
	} else if(ter == ot_forest_thick) {
		if(one_in(20)) { // has someone seen Charlie?
			pop_normal--;
		}
	} else if(ter == ot_forest_water) {
		if(one_in(10)) {
			pop_normal--;
		}
	} else if(ter >= ot_river_center && ter <= ot_river_nw) {
		if(one_in(20)) {
			pop_normal -= rng(0, 4);
		}
	} else if(ter >= ot_house_north && ter <= ot_shelter_under) {
		if(one_in(20)) {
			pop_normal += rng(0, 10) / 10; // 10% chance of +1
			if(pop_master > 0) {
				pop_normal += rng(0, 3);
			}
		}
	}
	if(pop_normal < 0) { pop_normal = 0; }
	pop_normal = std::min(pop_normal, SEEX * SEEY);
}

void game::move_hordes() {
	typedef std::vector<monhorde> MHVec;
	MHVec &monhordes = cur_om->zh;
	for(size_t i = 0; i < monhordes.size(); i++) {
		monhorde &horde = monhordes[i];
		if(!horde.move(*this)) {
			// Has not moved, has not changed
			continue;
		}
		if(horde.isEmpty()) {
			// If the horde has lost all its members, delete it
			monhordes.erase(monhordes.begin() + i);
			i--;
		} else if(merge(monhordes, i)) {
			// If the horde could be merged into another horde, delete this one
			monhordes.erase(monhordes.begin() + i);
			i--;
			continue;
		}
		// Check if the horde is still on the same overmap, otherwise move it
		overmap *om = getOverMap(horde.posx, horde.posy);
		if(om == 0) {
			// TODO: what now, the horde has been gone into the unknown
			// we might load an existing overmap, but we may as well just
			// remove this horde, a new one will be created sooner or later.
			monhordes.erase(monhordes.begin() + i);
			i--;
		} else if(om != cur_om) {
			// In this case the coords posx, posy have already been updated.
			// But the target is still wrong!
			// Simply reset the target
			horde.wandf = 0;
			horde.targetx = horde.posx;
			horde.targety = horde.posy;
			om->zh.push_back(horde);
			monhordes.erase(monhordes.begin() + i);
			i--;
		}
	}
}

bool monhorde::move(game &g) {
	int t = (int) g.turn;
	const int move_points = t - last_move;
	if(move_points < SEEX * 2) {
		if(move_points < 0) {
			last_move = t;
		}
		return false;
	}
	rest_here(g);
	last_move = t;
	if(!one_in(10)) { // zombies are slow, the skip this move.
		return false;
	}
	if(wandf > 0) {
		// Following a sound?
		wandf--;
	} else if((posx == targetx && posy == targety) || one_in(10)) {
		// 10% chance of changing the direction.
		targetx = posx + rng(-10, +10);
		targety = posy + rng(-10, +10);
		wandf = rng(1, 5); // Just stay with this target for a while
	}
	if(posx == targetx && posy == targety) {
		// Nothing to do, try to increase/decrease the population.
		return false;
	}
	if(abs(posx - targetx) > abs(posy - targety)) {
		posx += sign(targetx - posx);
	} else {
		posy += sign(targety - posy);
	}
	if(posx == targetx && posy == targety) {
		wandf = 0;
	}
	return true;
}

void monhorde::attract_by_sound(int x, int y, int vol) {
	if(g->levz != posz) {
		return;
	}
	vol = vol / SEEX;
	if(vol <= 0) {
		return;
	}
	const int dist = rl_dist(posx, posy, x, y);
	if(dist > vol || dist == 0) {
		return;
	}
	targetx = x;
	targety = y;
	wandf = vol;
	g->add_msg("Horde (rel: %d,%d) sound(%d) towards %d,%d", posx - (g->levx + MAPSIZE / 2), posy - (g->levy + MAPSIZE / 2), vol, targetx - (g->levx + MAPSIZE / 2), targety - (g->levy + MAPSIZE / 2));
}

void game::attract_hordes(int x, int y, int vol) {
	if(vol < SEEX) {
		return;
	}
	typedef std::vector<monhorde> MHVec;
	// submap coordinates
	x = (x / SEEX) + levx;
	y = (y / SEEY) + levy;
	MHVec &monhordes = cur_om->zh;
	for(size_t i = 0; i < monhordes.size(); i++) {
		monhorde &horde = monhordes[i];
		horde.attract_by_sound(x, y, vol);
	}
}

void game::spawn_horde_members() {
	/*
	if((turn % MINUTES(1)) != 0) {
		return; // Spawn every minute
	}
	*/
	monster zom;
	typedef std::vector<monhorde> MHVec;
	MHVec &monhordes = cur_om->zh;
	int t;
	for(size_t i = 0; i < monhordes.size(); i++) {
		monhorde &horde = monhordes[i];
		// Check if we are in the area of the loaded map
		if(horde.posx < levx || horde.posx >= levx + MAPSIZE) {
			continue;
		}
		if(horde.posy < levy || horde.posy >= levy + MAPSIZE) {
			continue;
		}
		int tarx = (horde.targetx - levx) * SEEX;
		int tary = (horde.targety - levy) * SEEY;
		int posx = (horde.posx - levx) * SEEX;
		int posy = (horde.posy - levy) * SEEY;
		// determine how many we spawn,
		// as this function is called every turn, this will quickly
		// reduce the population to 0 (all monsters are than spawned)
		int spawn_count = rng(1, std::min<int>(20, horde.getPopulation()));
		for(int j = 0; j < spawn_count && !horde.isEmpty(); j++) {
			std::string type;
			if(horde.pop_normal == 0) {
				type = MASTER_ZOMBIE_ID;
			} else {
                MonsterGroupResult res = MonsterGroupManager::GetResultFromGroup(horde.type, &spawn_count, (int)turn);
				// Special check: each horde has one master to begin with, if this one is lost, the group may shrink over time
				if(res.name == MASTER_ZOMBIE_ID && horde.pop_master == 0) {
					continue; // try again
				}
				type = res.name;
			}
			zom = monster(GetMType(type));
			zom.no_extra_death_drops = true;
			for(int iter = 0; iter < 10; iter++) {
				int monx = rng(0, SEEX - 1) + posx;
				int mony = rng(0, SEEY - 1) + posy;
				if(!zom.can_move_to(this, monx, mony) || !is_empty(monx, mony) ||
					m.sees(u.posx, u.posy, monx, mony, SEEX, t) || !m.is_outside(monx, mony)) {
					continue;
				}
				zom.spawn(monx, mony);
				add_zombie(zom);
				if(tarx != posx || tary != posy) {
					int mondex = mon_at(monx, mony);
					if (mondex != -1) {
						monster &z = _active_monsters[mondex];
						z.wander_to(tarx + rng(0, SEEX - 1), tary + rng(0, SEEY - 1), MAPSIZE * SEEX * MAPSIZE * SEEY);
					}
				}
				add_msg("Horde-Zombie (rel: %d,%d) spawned at (rel: %d,%d)", horde.posx - (levx + MAPSIZE / 2), horde.posy - (levy + MAPSIZE / 2), monx - u.posx, mony - u.posy);
				if(type == MASTER_ZOMBIE_ID) {
					horde.pop_master--;
				} else {
					horde.pop_normal--;
				}
				break;
			}
		}
		if(horde.isEmpty()) {
			monhordes.erase(monhordes.begin() + i);
			i--;
		}
	}
}
