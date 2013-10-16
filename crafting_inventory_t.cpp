#include "crafting_inventory_t.h"
#include "game.h"
#include "bionics.h"
#include "player.h"
#include "item_factory.h"
#include "inventory.h"
#include "crafting.h"
#include <algorithm>

crafting_inventory_t::crafting_inventory_t(game *g, player *p)
: p(p)
{
    init(g, PICKUP_RANGE);
}

crafting_inventory_t::crafting_inventory_t(game *g, player *p, int range)
: p(p)
{
    init(g, range);
}

void crafting_inventory_t::init(game *g, int range)
{
    if(range != -1) {
        assert(range >= 0);
        form_from_map(g, point(p->posx, p->posy), range);
    }
    // iterator of all bionics of the player and grab the toolsets automaticly
    // This allows easy addition of more toolsets
    for(std::vector<bionic>::const_iterator a = p->getMyBionics().begin(); a != p->getMyBionics().end(); ++a) {
        add_bio_toolset(*a, g->turn);
    }
}

void crafting_inventory_t::add_bio_toolset(const bionic &bio, const calendar &turn) {
    static const std::string prefixIn("bio_tools_");
    static const std::string prefixOut("toolset_");
    if(bio.id.compare(0, prefixIn.length(), prefixIn) != 0) {
        return;
    }
    const std::string tool_name = prefixOut + bio.id.substr(prefixIn.length());
    item tools(item_controller->find_template(tool_name), turn);
    tools.charges = p->power_level;
    by_bionic.push_back(item_from_bionic(bio.id, tools));
}


bool crafting_inventory_t::has_all_components(std::vector< std::vector<component> > &components) const {
    return has_all(components, false);
}

bool crafting_inventory_t::has_all_tools(std::vector< std::vector<component> > &tools) const {
    return has_all(tools, true);
}

bool crafting_inventory_t::has(const requirement &req, int sources) const {
    return count(req.type, req.ctype, req.count, sources) >= req.count;
}

int crafting_inventory_t::count(const requirement &req, int sources) const {
    return count(req.type, req.ctype, -1, sources);
}

bool crafting_inventory_t::has(component &x, bool as_tool) const {
    if(has(requirement(x, as_tool))) {
        x.available = 1;
    } else {
        x.available = -1;
    }
    return x.available == 1;
}

bool crafting_inventory_t::has_any(std::vector<component> &set_of_x, bool as_tool) const {
    if(set_of_x.size() == 0) {
        return true; // no tool/component needed, we have that
    }
    bool has_x_in_set = false;
    for(std::vector<component>::iterator x_it = set_of_x.begin(); x_it != set_of_x.end();
        x_it++) {
        component &x = *x_it;
        if(has(x, as_tool)) {
            has_x_in_set = true;
        }
    }
    return has_x_in_set;
}

bool crafting_inventory_t::has_all(std::vector< std::vector<component> > &xs, bool as_tool) const {
    bool has_x_in_set = true;
    for(std::vector< std::vector<component> >::iterator x_it = xs.begin(); x_it != xs.end();
        x_it++) {
        std::vector<component> &x = *x_it;
        if(!has_any(x, as_tool)) {
            has_x_in_set = false;
        }
    }
    return has_x_in_set;
}


int crafting_inventory_t::amount_of(const itype_id &type, const item &it)
{
    int count = 0;
    if(it.matches_type(type)) {
        // check if it's a container, if so, it should be empty
        if (it.type->is_container()) {
            if (it.contents.empty()) {
                count++;
            }
        } else {
            count++;
        }
    }
    for (int k = 0; k < it.contents.size(); k++) {
        count += amount_of(type, it.contents[k]);
    }
    return count;
}

int crafting_inventory_t::charges_of(const itype_id &type, const item &it)
{
    int count = 0;
    if(it.matches_type(type)) {
        if (it.charges < 0) {
            count++;
        } else {
            count += it.get_charges_of(type);
        }
    }
    for (int k = 0; k < it.contents.size(); k++) {
        count += charges_of(type, it.contents[k]);
    }
    return count;
}

#define COUNT_IT_(item_) \
    do { switch(what) { \
        case C_CHARGES: count += charges_of(type, (item_)); break; \
        case C_AMOUNT: count += amount_of(type, (item_)); break; \
        default: debugmsg("Invalid %d count type", (int) what); \
    } if(max > 0 && count >= max) { return count; } } while(false)

int crafting_inventory_t::count(const itype_id &type, count_type what, int max, int sources) const
{
    int count = 0;
    if(sources & S_PLAYER) {
        COUNT_IT_(p->weapon);
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
                ++stack_iter) {
                    COUNT_IT_(*stack_iter);
            }
        }
    }
    if(sources & S_MAP) {
        for(std::list<items_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
            const std::vector<item> &items = *(a->items);
            for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
                if(b->made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(*b);
            }
        }
    }
    if(sources & S_VEHICLE) {
        for(std::list<items_in_vehicle>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            const std::vector<item> &items = *(a->items);
            for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
                if(b->made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(*b);
            }
        }
        for(std::list<item_from_vpart>::const_iterator a = vpart.begin(); a != vpart.end(); ++a) {
            COUNT_IT_(a->it);
        }
    }
    if(sources & S_PLAYER) {
        for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            COUNT_IT_(a->it);
        }
    }
    if(sources & S_MAP) {
        for(std::list<item_from_souround>::const_iterator a = souround.begin(); a != souround.end(); ++a) {
            COUNT_IT_(a->it);
        }
    }
    return count;
}
#undef COUNT_IT_


void crafting_inventory_t::consume_tools(const std::vector<component> &tools, bool force_available)
{
    consume(tools, force_available ? assume_tools_force_available : assume_tools);
}

#if 0
void crafting_inventory_t::consume_tools(const std::vector<component> &tools, bool force_available)
{
    bool found_nocharge = false;
    std::vector<component> player_has;
    std::vector<component> map_has;
    // Use charges of any tools that require charges used
    for (int i = 0; i < tools.size() && !found_nocharge; i++) {
        if (!force_available && tools[i].available != 1) {
            continue;
        }
        const itype_id &type = tools[i].type;
        if (tools[i].count > 0) {
            int count = tools[i].count;
            if (player_charges_of(type) >= count) {
                player_has.push_back(tools[i]);
            }
            if (map_has_charges(type, count)) {
                map_has.push_back(tools[i]);
            }
        } else if (p->has_amount(type, 1) || map_has_amount(type, 1)) {
            found_nocharge = true;
        }
    }
    if (found_nocharge) {
        return;    // Default to using a tool that doesn't require charges
    }

    if (player_has.size() + map_has.size() == 1){
        if(map_has.empty()){
            p_use_charges(player_has[0].type, player_has[0].count);
        } else {
            g->m.use_charges(p->pos(), PICKUP_RANGE, map_has[0].type, map_has[0].count);
        }
    } else { // Variety of options, list them and pick one
        // Populate the list
        std::vector<std::string> options;
        for (int i = 0; i < map_has.size(); i++) {
            std::string tmpStr = item_controller->find_template(map_has[i].type)->name + " (nearby)";
            options.push_back(tmpStr);
        }
        for (int i = 0; i < player_has.size(); i++) {
            options.push_back(item_controller->find_template(player_has[i].type)->name);
        }

        if (options.size() == 0) { // This SHOULD only happen if cooking with a fire,
            return;    // and the fire goes out.
        }

        // Get selection via a popup menu
        int selection = menu_vec(false, "Use which tool?", options) - 1;
        if (selection < map_has.size())
            g->m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                             map_has[selection].type, map_has[selection].count);
        else {
            selection -= map_has.size();
            p_use_charges(player_has[selection].type, player_has[selection].count);
        }
    }
}
#endif

#if 0
void crafting_inventory_t::consume_tools(const std::vector< std::vector<component> > &tools, bool force_available)
{
    for (int i = 0; i < tools.size(); i++) {
        crafting_inv.consume_tools(tools[i], force_available);
    }
}
#endif
void crafting_inventory_t::consume_tools(const itype_id &type, int charges, bool force_available)
{
    std::vector<component> components;
    components.push_back(component(type, charges));
    components.back().available = 1;
    consume_tools(components, force_available);
}

bool crafting_inventory_t::has_different_types(const std::vector<candidate_item> &cv) {
    for(size_t i = 0; i + 1 < cv.size(); i++) {
        if(cv[i].the_item.type != cv[i + 1].the_item.type) {
            return true;
        }
    }
    return false;
}

void crafting_inventory_t::filter(std::vector<candidate_item> &cv, source_flags sources) {
    for(size_t i = 0; i < cv.size(); ) {
        if(!cv[i].is_source(sources)) {
            cv.erase(cv.begin() + i);
        } else {
            i++;
        }
    }
}

std::list<item> crafting_inventory_t::consume_items(const std::vector<component> &components)
{
    return consume(components, assume_components);
}

std::list<item> crafting_inventory_t::consume(const std::vector<component> &components, consume_flags flags)
{
    std::list<item> ret;
    if(components.empty()) {
        return ret;
    }
    typedef std::vector<civec> civecvec;
    
    // for each component of the set, this array
    // contains a civec with all the items that can be used
    // as component (basicly those items matches_type the component::type).
    civecvec has(components.size());
     // List for the menu_vec below, contains the menu entries that should be displayed.
    std::vector<std::string> options;
    // pair<int, int> = { menu_entry_type, index into has }
    // the menu_entry_type defines what we do if the user selects that menu entry.
    // the second value is just the index into the has array that belongs
    // to this option.
    std::vector< std::pair<int, int> > optionsIndizes;
    
    for (int i = 0; i < components.size(); i++) {
        const requirement req(components[i], flags != assume_components);
        const int pc = collect_candidates(req, S_FROM_PLAYER, has[i]);
        const int mc = collect_candidates(req, S_FROM_MAP, has[i]);
        if(mc + pc < req.count) {
            // Not enough items of this type
            continue;
        }
        assert(has[i].size() > 0);
        if(has[i].size() == 1) {
            // Only one possible choice
            options.push_back(has[i].front().to_string());
            optionsIndizes.push_back(std::pair<int, int>(-1, i));
        } else if(has_different_types(has[i])) {
            // several items to choose, and they are of different types, 
            // make another menu later if the user chooses this
            options.push_back(item_controller->find_template(req.type)->name + " (different items)");
            optionsIndizes.push_back(std::pair<int, int>(-2, i));
        } else {
            // several items, but they are all of the same type, list by location,
            const std::string &name = item_controller->find_template(req.type)->name;
            // Rules here are: show entry for on person, for nearby (if any of
            // them apply
            // also show the mixed entry, but only if none of the other two
            // are shown
            if(mc >= req.count) {
                options.push_back(name + " (nearby)");
                optionsIndizes.push_back(std::pair<int, int>(-3, i));
            }
            if(pc >= req.count) {
                options.push_back(name);
                optionsIndizes.push_back(std::pair<int, int>(-4, i));
            }
            if(mc < req.count && pc < req.count) {
                assert(mc + pc >= req.count);
                options.push_back(name + " (on person & nearby)");
                optionsIndizes.push_back(std::pair<int, int>(-5, i));
            }
        }
    }
    if(options.size() == 0) {
        debugmsg("Attempted a recipe with no available components!");
        return ret;
    }
    int selection = 0;
    if(options.size() != 1) { // no user-interaction needed if only one choise
        selection = menu_vec(false, "Use which component?", options) - 1;
    }
    assert(selection >= 0 && selection < options.size());
    assert(options.size() == optionsIndizes.size());
    const int menu_entry_type = optionsIndizes[selection].first;
    const int index_of_component = optionsIndizes[selection].second;
    // The user has choosen this selection of items:
    civec &vec_to_consume_from = has[index_of_component];
    // They are based on components[index_of_component], make a requirement
    // from it.
    requirement reqToRemove(components[index_of_component], flags != assume_components);
    if(menu_entry_type == -1) {
        // only one item in list, use this anyway, their is no choice
    } else if(menu_entry_type == -3) {
        // use only items from map
        filter(vec_to_consume_from, S_MAP);
    } else if(menu_entry_type == -4) {
        // use only items from player
        filter(vec_to_consume_from, S_PLAYER);
    } else if(menu_entry_type == -5) {
        // use items from both sources, no filtering or changes needed
    } else if(menu_entry_type == -2) {
        // ask user for specific items
        // sort to make it nicer looking
        std::sort(vec_to_consume_from.begin(), vec_to_consume_from.end(), std::less<candidate_item>());
        // stores which items the user has selected (
        std::vector<bool> selected(vec_to_consume_from.size(), false);
        // the (summed up) charges (as told by requirement::get_charges_or_amount)
        // of all selected items. If this reaches the required count, we can break out
        // of this loop.
        int cntFromSelectedOnes = 0;
        int minCount = reqToRemove.get_charges_or_amount(vec_to_consume_from[0].the_item);
        for(size_t i = 1; i < vec_to_consume_from.size(); i++) {
            minCount = std::min(minCount, reqToRemove.get_charges_or_amount(vec_to_consume_from[i].the_item));
        }
        // minCount means each item of vec_to_consume_from has at least
        // that much to give. Now if minCount >= reqToRemove.count
        // than exactly one item of the vector is always enough
        // this changes the algorithm to not go to select/unselect
        // single items, to directly select the item to use.
        while(true) {
            options.clear();
            for(size_t i = 0; i < vec_to_consume_from.size(); i++) {
                if(minCount >= reqToRemove.count) {
                    options.push_back(vec_to_consume_from[i].to_string());
                } else {
                    const std::string prefix(selected[i] ? "    selected " : "not selected ");
                    options.push_back(prefix + vec_to_consume_from[i].to_string());
                }
            }
            if(minCount < reqToRemove.count) {
                if(cntFromSelectedOnes >= reqToRemove.count) {
                    options.push_back("OK");
                }
                selection = menu_vec(false, "Select the components to use?", options) - 1;
                if(selection >= vec_to_consume_from.size()) {
                    civec tmpVec;
                    for(size_t i = 0; i < vec_to_consume_from.size(); i++) {
                        if(selected[i]) {
                            tmpVec.push_back(vec_to_consume_from[i]);
                        }
                    }
                    tmpVec.swap(vec_to_consume_from);
                    break;
                }
            } else {
                selection = menu_vec(false, "Select the component to use?", options) - 1;
                civec tmpVec(1, vec_to_consume_from[selection]);
                tmpVec.swap(vec_to_consume_from);
                break;
            }
            if(!selected[selection] && cntFromSelectedOnes >= reqToRemove.count) {
                popup("You can't slect more items, deselect some before selecting more");
            } else {
                selected[selection] = !selected[selection];
                if(selected[selection]) {
                    cntFromSelectedOnes += reqToRemove.get_charges_or_amount(vec_to_consume_from[selection].the_item);
                } else {
                    cntFromSelectedOnes -= reqToRemove.get_charges_or_amount(vec_to_consume_from[selection].the_item);
                }
            }
        }
    } else {
        assert(false);
    }
    while(reqToRemove.count > 0 && !vec_to_consume_from.empty()) {
        vec_to_consume_from[0].consume(g, p, reqToRemove, ret);
        vec_to_consume_from.erase(vec_to_consume_from.begin());
    }
    if(reqToRemove.count > 0) {
        debugmsg("Their are still %d charges/amounts missing (not used) for %s", reqToRemove.count, reqToRemove.type.c_str());
    }
    return ret;
}
#if 0
std::list<item> crafting_inventory_t::consume_items(const std::vector< std::vector<component> > &components)
{
    std::list<item> ret;
    for (int i = 0; i < components.size(); i++) {
        std::list<item> tmp = consume_items(components[i]);
        used.splice(used.end(), tmp);
    }
    return ret;
}
#endif
std::list<item> crafting_inventory_t::consume_items(const itype_id &type, int count)
{
    std::vector<component> components;
    components.push_back(component(type, count));
    components.back().available = 1;
    return consume_items(components);
}










#if 0
bool crafting_inventory_t::has_amount(const std::vector<std::pair<itype_id, int> > &items) const
{
    for(size_t i = 0; i < items.size(); i++) {
        if(!has_amount(items[i].first, items[i].second)) {
            return false;
        }
    }
    return true;
}
#endif

#if 0
int crafting_inventory_t::player_charges_of(const itype_id &type) const
{
    if(type.compare(0, 8, "toolset_") == 0) {
        if(p->has_bionic(std::string("bio_tools_") + type.substr(8))) {
            return p->power_level;
        }
        return 0; // Don't have that bionic
    }
    int quantity = 0;
    quantity += p->weapon.get_charges_of(type);
    if(p->weapon.ammo_type() == type) {
        quantity += p->weapon.charges;
    }
    for(int i = 0; i < p->weapon.contents.size(); i++) {
        quantity += p->weapon.contents[i].get_charges_of(type);
    }
    quantity += p->inv.charges_of(type);
    return quantity;
}
#endif


#define CMP_IF(_w) \
    do { if(_w != other._w) { return _w < other._w; } } while(false)
bool crafting_inventory_t::candidate_item::operator<(const candidate_item &other) const {
    CMP_IF(location);
    switch(location) {
        case in_inventory:
            CMP_IF(invlet);
            break;
        case on_map:
            CMP_IF(mapx);
            CMP_IF(mapy);
            CMP_IF(mindex);
            break;
        case in_vehicle:
            CMP_IF(veh);
            CMP_IF(part_num);
            CMP_IF(vindex);
            break;
        case weapon:
            break;
        case from_bionic:
            CMP_IF(bionic);
            break;
        case from_souround:
            CMP_IF(souroundings);
            break;
    }
    return ::memcmp(this, &other, sizeof(*this));
}
#undef CMP_IF

bool crafting_inventory_t::candidate_item::is_source(source_flags sources) const {
    switch(location) {
        case in_vehicle: return sources & S_VEHICLE;
        case in_vpart: return sources & S_VEHICLE;
        case on_map: return sources & S_MAP;
        case from_souround: return sources & S_MAP;
        case weapon: return sources & S_PLAYER;
        case in_inventory: return sources & S_PLAYER;
        case from_bionic: return sources & S_PLAYER;
//        default: return false;
    }
    assert(false);
    return false;
}

std::string crafting_inventory_t::candidate_item::to_string() const {
    switch(location) {
        case in_vehicle:
        case in_vpart:
            return const_cast<item&>(the_item).tname() + " (in car)";
        case on_map:
        case from_souround:
            return const_cast<item&>(the_item).tname() + " (nearby)";
        case weapon:
        case in_inventory:
        case from_bionic:
            return const_cast<item&>(the_item).tname();
//        default:
//            return const_cast<item&>(the_item).tname();
    }
    assert(false);
    return "";
}

int crafting_inventory_t::candidate_item::drainVehicle(const std::string &ftype, int amount, std::list<item> &ret) {
    item tmp = item_controller->create(ftype, g->turn);
    tmp.charges = veh->drain(ftype, amount);
    amount -= tmp.charges;
    ret.push_back(tmp);
    return amount;
}

void crafting_inventory_t::candidate_item::consume(game *g, player *p, requirement &req, std::list<item> &ret) {
    assert(req.count > 0);
    if(req.ctype == C_AMOUNT) {
        switch(location) {
            case in_inventory:
                ret.push_back(p->inv.remove_item_by_letter(invlet));
                break;
            case in_vehicle:
                ret.push_back(veh->parts[part_num].items[vindex]);
                veh->remove_item(part_num, vindex);
                break;
            case on_map:
                ret.push_back(g->m.i_at(mapx, mapy)[mindex]);
                g->m.i_rem(mapx, mapy, mindex);
                break;
            case weapon:
                ret.push_back(p->weapon);
                p->remove_weapon();
                break;
            case in_vpart:
                debugmsg("attempted to consume a pseudo vehicle part item %s", the_item.tname().c_str());
                break; // can not remove souroundings pseudo-item
            case from_souround:
                debugmsg("attempted to consume a pseudo sourounding item %s", the_item.tname().c_str());
                break; // can not remove souroundings pseudo-item
            case from_bionic:
                debugmsg("attempted to consume a pseudo bionic item %s", the_item.tname().c_str());
                break; // can not remove bionic pseudo-item
            default:
                debugmsg("dont know what to consume!");
                break;
        }
        req.count--;
        return;
    }
    switch(location) {
        case in_inventory:
            if(p->inv.item_by_letter(invlet).use_charges(req.type, req.count, ret, true)) {
                p->inv.remove_item_by_letter(invlet);
            }
            break;
        case in_vehicle:
            if(veh->parts[part_num].items[vindex].use_charges(req.type, req.count, ret, true)) {
                veh->remove_item(part_num, vindex);
            }
            break;
        case on_map:
            if(g->m.i_at(mapx, mapy)[vindex].use_charges(req.type, req.count, ret, true)) {
                g->m.i_rem(mapx, mapy, mindex);
            }
            break;
        case weapon:
            if(p->weapon.use_charges(req.type, req.count, ret, true)) {
                p->remove_weapon();
            }
            break;
        case in_vpart:
            if(veh->part_flag(part_num, "KITCHEN")) {
                if(req.type == "func:hotplate") {
                    const int modi = the_item.type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, ret);
                    req.count -= remains / modi;
                } else if(req.type == "water_clean") {
                    drainVehicle("water", req.count, ret);
                }
            }
            if(veh->part_flag(part_num, "WELDRIG")) {
                if(req.type == "func:hotplate" || req.type == "func:soldering_iron") {
                    const int modi = the_item.type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, ret);
                    req.count -= remains / modi;
                }
            }
            break;
        case from_souround:
            // Basicly this is an inifinte amount of things
            // like fire, or a water source, in this case we can ignore it.
            break;
        case from_bionic:
            if(req.count >= p->power_level) {
                req.count -= p->power_level;
                p->power_level = 0;
            } else {
                p->power_level -= req.count;
                req.count = 0;
            }
            break;
        default:
            debugmsg("dont know what to consume!");
            break;
    }
}

bool crafting_inventory_t::all_equal(const std::vector<candidate_item> &candidates) {
    for(int i = 0; i + 1 < candidates.size(); i++) {
        const candidate_item &prev = candidates[i];
        const candidate_item &cur = candidates[i + 1];
        if(prev.the_item.type != cur.the_item.type) {
            return false;
        }
        if(prev.the_item.damage != cur.the_item.damage) {
            return false;
        }
        if(prev.the_item.burnt != cur.the_item.burnt) {
            return false;
        }
        if(prev.the_item.bigness != cur.the_item.bigness) {
            return false;
        }
    }
    return true;
}


// basicly: ignore the item if we count by charges (not amount) and the item has no charges left
#define XMATCH(item_) \
    ((item_).matches_type(req.type) && (req.ctype == C_AMOUNT || (item_).charges > 0))

int crafting_inventory_t::collect_candidates(const requirement &req, int sources, std::vector<candidate_item> &candidates) {
    int count = 0;
    if(sources & S_MAP) {
        for(std::list<items_on_map>::iterator a = on_map.begin(); a != on_map.end(); ++a) {
            std::vector<item> &items = *(a->items);
            for(std::vector<item>::iterator b = items.begin(); b != items.end(); ++b) {
                if(XMATCH(*b)) {
                    count += req.get_charges_or_amount(*b);
                    candidates.push_back(candidate_item(a->origin.x, a->origin.y, b - items.begin(), *b));
                }
            }
        }
    }
    if(sources & S_VEHICLE) {
        for(std::list<items_in_vehicle>::iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            std::vector<item> &items = *a->items;
            for(size_t b = 0; b < items.size(); b++) {
                if(XMATCH(items[b])) {
                    count += req.get_charges_or_amount(items[b]);
                    candidates.push_back(candidate_item(a->veh, a->part_num, b, items[b]));
                }
            }
        }
        for(std::list<item_from_vpart>::iterator a = vpart.begin(); a != vpart.end(); ++a) {
            if(XMATCH(a->it)) {
                count += req.get_charges_or_amount(a->it);
                candidates.push_back(candidate_item(a->veh, a->part_num, *a));
            }
        }
    }
    if(sources & S_PLAYER) {
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
                ++stack_iter) {
                if(XMATCH(*stack_iter)) {
                    count += req.get_charges_or_amount(*stack_iter);
                    candidates.push_back(candidate_item(stack_iter->invlet, *stack_iter));
                }
            }
        }
        if(XMATCH(p->weapon)) {
            count += req.get_charges_or_amount(p->weapon);
            candidates.push_back(candidate_item(p->weapon));
        }
        for(std::list<item_from_bionic>::iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            if(XMATCH(a->it)) {
                count += req.get_charges_or_amount(a->it);
                candidates.push_back(candidate_item(*a));
            }
        }
    }
    if(sources & S_MAP) {
        for(std::list<item_from_souround>::iterator a = souround.begin(); a != souround.end(); ++a) {
            if(XMATCH(a->it)) {
                count += req.get_charges_or_amount(a->it);
                candidates.push_back(candidate_item(*a));
            }
        }
    }
    return count;
}
#undef XMATCH



crafting_inventory_t::requirement::requirement(const component &comp, bool as_tool) {
    const itype_id &type = comp.type;
    const int req = comp.count;
    this->type = type;
    if(as_tool) {
        if(req > 0) {
            this->count = req;
            this->ctype = C_CHARGES;
        } else {
            this->count = 1;
            this->ctype = C_AMOUNT;
        }
    } else {
        if(req > 0 && item_controller->find_template(type)->count_by_charges()) {
            this->count = req;
            this->ctype = C_CHARGES;
        } else {
            this->count = req < 0 ? -req : req;
            this->ctype = C_AMOUNT;
        }
    }
}

int crafting_inventory_t::requirement::get_charges_or_amount(const item &it) const {
    if(ctype == C_CHARGES) {
        return it.get_charges_of(type);
    } else {
        return 1;
    }
}


#if 0
std::list<item> crafting_inventory_t::p_use_charges(const itype_id &type, int amount) {
    for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
        if(a->it.matches_type(type)) {
            return p->use_charges(a->it.type->id, amount);
        }
    }
    return p->use_charges(type, amount);
}
#endif







double crafting_inventory_t::compute_time_modi(const std::vector<component> *begin, size_t count) const {
    double worst_modi = 0.0;
    for(size_t j = 0; j < count; j++, begin++) {
        // tools: we need one (any) tool from this vector:
        const std::vector<component> &tools = *begin;
        if(tools.empty()) {
            continue;
        }
        const double modi = compute_time_modi(tools);
        // We must take the worst (largest) modi here, because
        // if several tools are required (think hammer+screwdriver)
        // it does not matter if one of those is super-mega-good,
        // if the other is bad and damaged
        if(modi > 0.0 && worst_modi < modi) {
            worst_modi = modi;
        }
    }
    if(worst_modi == 0.0) {
        return 1.0;
    }
    return worst_modi;
}

double crafting_inventory_t::compute_time_modi(const recipe &making) const {
    if(making.tools.empty()) {
        return 1.0;
    }
    return compute_time_modi(&making.tools.front(), making.tools.size());
}

double crafting_inventory_t::compute_time_modi(const construction_stage &stage) const {
    return compute_time_modi(stage.tools, sizeof(stage.tools) / sizeof(stage.tools[0]));
}

double crafting_inventory_t::compute_time_modi(const std::vector<component> &tools) const
{
    const item *it = 0;
    const double modi = compute_time_modi(tools, it);
	// Debug info, ignore for now
	/*
    if(it != 0) {
        debugmsg("tool that got used: %s - modi is %f", const_cast<item*>(it)->tname().c_str(), modi);
    }
	*/
    return modi;
}

double crafting_inventory_t::compute_time_modi(const std::vector<component> &tools, const item *&item_that_got_used) const
{
    if(tools.empty()) {
        return 1.0; // No tool needed, no influnce by tools => 1 is neutral
    }
    double best_modi = 0.0;
    for(std::vector<component>::const_iterator a = tools.begin(); a != tools.end(); a++) {
        const itype_id &type = a->type;
        const item *it = 0;
        const double modi = compute_time_modi(type, a->count, it);
        if(modi > 0.0 && (best_modi == 0.0 || best_modi > modi)) {
            best_modi = modi;
            item_that_got_used = it;
        }
    }
    if(best_modi > 0.0) {
        return best_modi;
    }
    return 1.0;
}

void use_modi(const item &it, const itype_id &type, int count, double &best_modi, const item *&item_that_got_used) {
    if(!it.matches_type(type)) {
        return;
    }
    if(it.get_charges_of(type) < count) {
        return;
    }
//    g->add_msg("use %s?", const_cast<item&>(it).tname().c_str());
    const double modi = it.get_functionality_time_modi(type);
    if(modi > 0.0 && (best_modi == 0.0 || best_modi > modi)) {
        best_modi = modi;
        item_that_got_used = &it;
    }
}

double crafting_inventory_t::compute_time_modi(const itype_id &type, int count, const item *&item_that_got_used) const
{
    double best_modi = 0.0;
    for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
        ++iter) {
        for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
            ++stack_iter) {
            ::use_modi(*stack_iter, type, count, best_modi, item_that_got_used);
        }
    }
    for(std::list<items_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
        const std::vector<item> &items = *(a->items);
        for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
            ::use_modi(*b, type, count, best_modi, item_that_got_used);
        }
    }
    for(std::list<items_in_vehicle>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
        const std::vector<item> &items = *(a->items);
        for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
            ::use_modi(*b, type, count, best_modi, item_that_got_used);
        }
    }
    for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
        ::use_modi(a->it, type, count, best_modi, item_that_got_used);
    }
    for(std::list<item_from_souround>::const_iterator a = souround.begin(); a != souround.end(); ++a) {
        ::use_modi(a->it, type, count, best_modi, item_that_got_used);
    }
    if(best_modi > 0.0) {
        return best_modi;
    }
    return 1.0;
}

bool hasQuality(const item &it, const std::string &name, int level)
{
    const std::map<std::string, int> &qualities = it.type->qualities;
    const std::map<std::string, int>::const_iterator quality_iter = qualities.find(name);
    return (quality_iter != qualities.end() && level >= quality_iter->second);
}

bool crafting_inventory_t::has_items_with_quality(const std::string &name, int level,
        int amount) const
{
    for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
        ++iter) {
        for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
            ++stack_iter) {
            if(::hasQuality(*stack_iter, name, level)) {
                amount--;
                if(amount <= 0) {
                    return true;
                }
            }
        }
    }
    for(std::list<items_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
        const std::vector<item> &items = *(a->items);
        for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
            if(b->made_of(LIQUID)) {
                continue;
            }
            if(::hasQuality(*b, name, level)) {
                amount--;
                if(amount <= 0) {
                    return true;
                }
            }
        }
    }
    for(std::list<items_in_vehicle>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
        const std::vector<item> &items = *(a->items);
        for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
            if(::hasQuality(*b, name, level)) {
                amount--;
                if(amount <= 0) {
                    return true;
                }
            }
        }
    }
    for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
        const item &it = a->it;
        if(::hasQuality(it, name, level)) {
            amount--;
            if(amount <= 0) {
                return true;
            }
        }
    }
    for(std::list<item_from_souround>::const_iterator a = souround.begin(); a != souround.end(); ++a) {
        if(::hasQuality(a->it, name, level)) {
            amount--;
            if(amount <= 0) {
                return true;
            }
        }
    }
    return amount <= 0;
}
