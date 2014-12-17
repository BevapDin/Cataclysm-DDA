#include "creature_tracker.h"
#include "mongroup.h"
#include "output.h"
#include "debug.h"

Creature_tracker::Creature_tracker()
{
}

Creature_tracker::~Creature_tracker()
{
    clear();
}

monster &Creature_tracker::find(int index)
{
    return *(_old_monsters_list[index]);
}

int Creature_tracker::mon_at(const tripoint &coords) const
{
    const auto iter = _old_monsters_by_location.find(coords);
    if (iter != _old_monsters_by_location.end()) {
        const int critter_id = iter->second;
        if (!_old_monsters_list[critter_id]->is_dead()) {
            return critter_id;
        }
    }
    return -1;
}

int Creature_tracker::dead_mon_at(const tripoint &coords) const
{
    const auto iter = _old_monsters_by_location.find(coords);
    if (iter != _old_monsters_by_location.end()) {
        const int critter_id = iter->second;
        if (_old_monsters_list[critter_id]->is_dead()) {
            return critter_id;
        }
    }
    return -1;
}

bool Creature_tracker::add(monster &critter)
{
    if (critter.type->id == "mon_null") { // Don't wanna spawn null monsters o.O
        return false;
    }
    if (-1 != mon_at(critter.pos())) {
        debugmsg("add_zombie: there's already a monster at %d,%d,%d", critter.xpos(), critter.ypos(), critter.zpos());
        return false;
    }
    if (monster_is_blacklisted(critter.type)) {
        return false;
    }
    _old_monsters_by_location[critter.pos()] = _old_monsters_list.size();
    _old_monsters_list.push_back(new monster(critter));
    return true;
}
size_t Creature_tracker::size() const
{
    return _old_monsters_list.size();
}

bool Creature_tracker::update_pos(const monster &critter, const tripoint &p)
{
    if (critter.pos() == p) {
        return true; // success?
    }
    bool success = false;
    const int dead_critter_id = dead_mon_at(critter.pos());
    const int live_critter_id = mon_at(critter.pos());
    const int critter_id = critter.is_dead() ? dead_critter_id : live_critter_id;
    const int new_critter_id = mon_at(p);
    if (new_critter_id >= 0 && !_old_monsters_list[new_critter_id]->is_dead()) {
        debugmsg("update_zombie_pos: new location %d,%d,%d already has zombie %d",
                 p.x, p.y, p.z, new_critter_id);
    } else if (critter_id >= 0) {
        if (&critter == _old_monsters_list[critter_id]) {
            _old_monsters_by_location.erase(critter.pos());
            _old_monsters_by_location[p] = critter_id;
            success = true;
        } else {
            debugmsg("update_zombie_pos: old location %d,%d,%d had zombie %d instead",
                     critter.xpos(), critter.ypos(), critter.zpos(), critter_id);
        }
    } else {
        // We're changing the x/y coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update _old_monsters_by_location for us.
        debugmsg("update_zombie_pos: no such zombie at %d,%d,%d (moving to %d,%d,%d)",
                 critter.xpos(), critter.ypos(), critter.zpos(), p.x, p.y, p.z);
    }
    return success;
}

void Creature_tracker::remove(const int idx)
{
    monster &m = *_old_monsters_list[idx];
    const auto oldloc = m.pos();
    const auto i = _old_monsters_by_location.find(oldloc);
    const int prev = (i == _old_monsters_by_location.end() ? -1 : i->second);

    if (prev == idx) {
        _old_monsters_by_location.erase(oldloc);
    }

    delete _old_monsters_list[idx];
    _old_monsters_list.erase(_old_monsters_list.begin() + idx);

    // Fix indices in _old_monsters_by_location for any zombies that were just moved down 1 place.
    for( auto &elem : _old_monsters_by_location ) {
        if( elem.second > idx ) {
            --elem.second;
        }
    }
}

void Creature_tracker::clear()
{
    for( auto monster_ptr : _old_monsters_list ) {
        delete monster_ptr;
    }
    _old_monsters_list.clear();
    _old_monsters_by_location.clear();
}

void Creature_tracker::rebuild_cache()
{
    _old_monsters_by_location.clear();
    for (int ii = 0, max_ii = size(); ii < max_ii; ii++) {
        monster &critter = *_old_monsters_list[ii];
        _old_monsters_by_location[critter.pos()] = ii;
    }
}

const std::vector<monster>& Creature_tracker::list() const
{
    static std::vector<monster> for_now;
    for_now.clear();
    for( const auto monster_ptr : _old_monsters_list ) {
        for_now.push_back( *monster_ptr );
    }
    return for_now;
}
