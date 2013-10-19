#include "crafting_inventory_t.h"
#include "game.h"
#include "bionics.h"
#include "player.h"
#include "item_factory.h"
#include "inventory.h"
#include "crafting.h"
#include <algorithm>

bool operator==(const point &a, const point &b) {
    return a.x == b.x && a.y == b.y;
}

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


int crafting_inventory_t::amount_of(const itype_id &type, const item &the_item)
{
    int count = 0;
    if(the_item.matches_type(type)) {
        // check if the_item's a container, if so, the_item should be empty
        if (the_item.type->is_container()) {
            if (the_item.contents.empty()) {
                count++;
            }
        } else {
            count++;
        }
    }
    for (int k = 0; k < the_item.contents.size(); k++) {
        count += amount_of(type, the_item.contents[k]);
    }
    return count;
}

int crafting_inventory_t::charges_of(const itype_id &type, const item &the_item)
{
    int count = 0;
    if(the_item.matches_type(type)) {
        if (the_item.charges < 0) {
            count++;
        } else {
            count += the_item.get_charges_of(type);
        }
    }
    for (int k = 0; k < the_item.contents.size(); k++) {
        count += charges_of(type, the_item.contents[k]);
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
        for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            const std::vector<item> &items = *(a->items);
            for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
                if(b->made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(*b);
            }
        }
        for(std::list<item_from_vpart>::const_iterator a = vpart.begin(); a != vpart.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    if(sources & S_PLAYER) {
        for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    if(sources & S_MAP) {
        for(std::list<item_from_surrounding>::const_iterator a = surround.begin(); a != surround.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    return count;
}
#undef COUNT_IT_

bool crafting_inventory_t::candidate_item::valid() const {
    switch(location) {
        case LT_MAP:
            return mapitems != NULL && mindex < mapitems->items->size();
        case LT_SURROUNDING:
            return surroundings != NULL;
        case LT_VEHICLE_CARGO:
            return vitems != NULL && iindex < vitems->items->size();
        case LT_VPART:
            return vpartitem != NULL;
        case LT_WEAPON:
            return true;
        case LT_INVENTORY:
            return invlet > ' '; // Or something else?
        case LT_BIONIC:
            return bionic != NULL;
        default:
            return false;
    }
}

#include "picojson.h"
void crafting_inventory_t::candidate_item::deserialize(crafting_inventory_t &cinv, const std::string &data) {
    picojson::value pjv;
    const char *s = data.c_str();
    std::string err = picojson::parse(pjv, s, s + data.length());
    assert(err.empty());
    deserialize(cinv, pjv);
}

void crafting_inventory_t::candidate_item::deserialize(crafting_inventory_t &cinv, const picojson::value &pjv) {
    using namespace picojson;
    assert(pjv.is<object>());
    assert(pjv.contains("location"));
    assert(pjv.get("location").is<int>());
    location = (LocationType) pjv.get("location").get<double>();
    point tmppnt;
    std::string tmpstr;
    switch(location) {
        case LT_MAP:
            tmppnt.x = (int) pjv.get("x").get<double>();
            tmppnt.y = (int) pjv.get("y").get<double>();
            mapitems = NULL;
            for(std::list<items_on_map>::iterator a = cinv.on_map.begin(); a != cinv.on_map.end(); ++a) {
                if(a->position == tmppnt) {
                    mapitems = &(*a);
                    break;
                }
            }
            mindex = (int) pjv.get("index").get<double>();
            break;
        case LT_SURROUNDING:
            tmppnt.x = (int) pjv.get("x").get<double>();
            tmppnt.y = (int) pjv.get("y").get<double>();
            tmpstr = pjv.get("type").get<std::string>();
            surroundings = NULL;
            for(std::list<item_from_surrounding>::iterator a = cinv.surround.begin(); a != cinv.surround.end(); ++a) {
                if(a->position == tmppnt
                    && a->the_item.type->id == tmpstr
                ) {
                    surroundings = &(*a);
                }
            }
            break;
        case LT_VEHICLE_CARGO:
            tmppnt.x = (int) pjv.get("vehptr").get<double>();
            tmppnt.y = (int) pjv.get("part").get<double>();
            vitems = NULL;
            for(std::list<items_in_vehicle_cargo>::iterator a = cinv.in_veh.begin(); a != cinv.in_veh.end(); ++a) {
                if(reinterpret_cast<int>(a->veh) == tmppnt.x
                    && a->part_num == tmppnt.y
                ) {
                    vitems = &(*a);
                    break;
                }
            }
            iindex = (int) pjv.get("index").get<double>();
            break;
        case LT_VPART:
            tmppnt.x = (int) pjv.get("vehptr").get<double>();
            tmppnt.y = (int) pjv.get("part").get<double>();
            vpartitem = NULL;
            tmpstr = pjv.get("type").get<std::string>();
            for(std::list<item_from_vpart>::iterator a = cinv.vpart.begin(); a != cinv.vpart.end(); ++a) {
                if(reinterpret_cast<int>(a->veh) == tmppnt.x
                    && a->part_num == tmppnt.y
                    && a->the_item.type->id == tmpstr
                ) {
                    vpartitem = &(*a);
                    break;
                }
            }
            break;
        case LT_WEAPON:
            break;
        case LT_INVENTORY:
            invlet = (char) ((int) pjv.get("invlet").get<double>());
            break;
        case LT_BIONIC:
            bionic = NULL;
            tmpstr = pjv.get("bio_id").get<std::string>();
            for(std::list<item_from_bionic>::iterator a = cinv.by_bionic.begin(); a != cinv.by_bionic.end(); ++a) {
                if(a->bio_id == tmpstr) {
                    bionic = &(*a);
                    break;
                }
            }
            break;
        default:
            debugmsg("Unknown %d location in candidate_item::deserialize", (int) location);
            break;
    }
}


std::string crafting_inventory_t::candidate_item::serialize() const {
    picojson::value v;
    serialize(v);
    return v.serialize();
}

#define pv(x) picojson::value(x)
void crafting_inventory_t::candidate_item::serialize(picojson::value &v) const {
    picojson::object pjmap;
    pjmap["location"] = pv((int) location);
    switch(location) {
        case LT_MAP:
            pjmap["x"] = pv(mapitems->position.x);
            pjmap["y"] = pv(mapitems->position.y);
            pjmap["index"] = pv(mindex);
            break;
        case LT_SURROUNDING:
            pjmap["x"] = pv(surroundings->position.x);
            pjmap["y"] = pv(surroundings->position.y);
            pjmap["type"] = pv(surroundings->the_item.type->id);
            break;
        case LT_VEHICLE_CARGO:
            // Note: this casts a pointer to int, this is not stable after
            // e.g. restarting.
            // BUT: this value is read in and searched for in the crafting_inventory_t,
            // if there is no matching vehicle found, than we reset the_item to NULL.
            pjmap["vehptr"] = pv(reinterpret_cast<int>(vitems->veh));
            pjmap["part"] = pv(vitems->part_num);
            pjmap["index"] = pv(iindex);
            break;
        case LT_VPART:
            // Note: see above case LT_VEHICLE_CARGO
            pjmap["vehptr"] = pv(reinterpret_cast<int>(vitems->veh));
            pjmap["part"] = pv(vitems->part_num);
            pjmap["type"] = pv(vpartitem->the_item.type->id);
            break;
        case LT_WEAPON:
            break;
        case LT_INVENTORY:
            pjmap["invlet"] = pv(static_cast<int>(invlet));
            break;
        case LT_BIONIC:
            pjmap["bio_id"] = pv(bionic->bio_id);
            break;
        default:
            debugmsg("Unknown %d location in candidate_item", (int) location);
            break;
    }
    v = picojson::value(pjmap);
}
#undef pv


void crafting_inventory_t::consume_tools(const std::vector< std::vector<component> > &tools, bool force_available)
{
    for(size_t i = 0; i < tools.size(); i++) {
        consume_tools(tools[i], force_available);
    }
}

void crafting_inventory_t::consume_tools(const std::vector<component> &tools, bool force_available)
{
    std::list<item> tmplist;
    consume(tools, force_available ? assume_tools_force_available : assume_tools, tmplist);
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

void crafting_inventory_t::consume_tools(const itype_id &type, int charges, bool force_available)
{
    std::vector<component> components;
    components.push_back(component(type, charges));
    components.back().available = 1;
    consume_tools(components, force_available);
}

bool crafting_inventory_t::has_different_types(const civec &cv) {
    for(size_t i = 0; i + 1 < cv.size(); i++) {
        if(cv[i].the_item.type != cv[i + 1].the_item.type) {
            return true;
        }
    }
    return false;
}

void crafting_inventory_t::filter(civec &cv, source_flags sources) {
    for(size_t i = 0; i < cv.size(); ) {
        if(!cv[i].is_source(sources)) {
            cv.erase(cv.begin() + i);
        } else {
            i++;
        }
    }
}

std::list<item> crafting_inventory_t::consume_gathered(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec)
{
    std::list<item> used_items;
    size_t indexInStrVec = 0;
    for (int i = 0; i < components.size(); i++) {
        const std::vector<component> &cv = components[i];
        if(cv.empty()) {
            continue;
        }
        civec vec_to_consume_from;
        int index_of_component;
        if(indexInStrVec < strVec.size()) {
            index_of_component = deserialize(strVec[indexInStrVec], vec_to_consume_from);
            if(index_of_component == -1) {
                strVec.clear();
            }
            indexInStrVec++;
        }
        if(vec_to_consume_from.empty()) {
            index_of_component = select_items_to_use(cv, flags, vec_to_consume_from);
        }
        if(index_of_component >= 0) {
            const requirement reqToRemove(cv[index_of_component], flags != assume_components);
            consume(reqToRemove, vec_to_consume_from, used_items);
        } else {
            debugmsg("no suitable component recovered");
        }
    }
    if(indexInStrVec == strVec.size() && indexInStrVec > 0) {
        strVec.clear();
    } else if(indexInStrVec > 0 && indexInStrVec < strVec.size()) {
        strVec.erase(strVec.begin(), strVec.begin() + indexInStrVec);
    }
    return used_items;
}

std::list<item> crafting_inventory_t::consume_gathered(const recipe &making, std::vector<std::string> &strVec) {
    consume_gathered(making.tools, assume_tools, strVec);
    std::list<item> used_items = consume_gathered(making.components, assume_components, strVec);
    return used_items;
}

int crafting_inventory_t::deserialize(const std::string &data, civec &vec) {
    using namespace picojson;
    picojson::value pjv;
    const char *s = data.c_str();
    std::string err = picojson::parse(pjv, s, s + data.length());
    assert(err.empty());
    assert(pjv.is<picojson::array>());
    assert(pjv.contains(0));
    const int index = (int) pjv.get(0).get<double>();
    vec.clear();
    for(size_t i = 1; pjv.contains(i); i++) {
        candidate_item ci(*this, pjv.get(i));
        if(!ci.valid()) {
            debugmsg("failed to deserialize");
            vec.clear();
            return -1;
        }
        vec.push_back(ci);
    }
    return index;
}

std::string crafting_inventory_t::serialize(int index, const civec &vec) {
    assert(!vec.empty());
    std::ostringstream buffer;
    buffer << "[" << index;
    for(size_t i = 0; i < vec.size(); i++) {
        buffer << "," << vec[i].serialize();
    }
    buffer << "]";
    return buffer.str();
}

void crafting_inventory_t::gather_inputs(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec)
{
    for (int i = 0; i < components.size(); i++) {
        const std::vector<component> &cv = components[i];
        if(cv.empty()) {
            continue;
        }
        civec vec_to_consume_from;
        const int index_of_component = select_items_to_use(cv, flags, vec_to_consume_from);
        if(index_of_component >= 0) {
            strVec.push_back(serialize(index_of_component, vec_to_consume_from));
        } else {
            strVec.push_back(std::string(""));
        }
    }
}

void crafting_inventory_t::gather_input(const recipe &making, std::vector<std::string> &strVec) {
    gather_inputs(making.tools, assume_tools, strVec);
    gather_inputs(making.components, assume_components, strVec);
}

std::list<item> crafting_inventory_t::consume_items(const std::vector< std::vector<component> > &components)
{
    std::list<item> result;
    for (int i = 0; i < components.size(); i++) {
        consume(components[i], assume_components, result);
    }
    return result;
}

std::list<item> crafting_inventory_t::consume_items(const std::vector<component> &components)
{
    std::list<item> tmplist;
    consume(components, assume_components, tmplist);
    return tmplist;
}

typedef enum {
    single_choise,
    ask_again,
    nearby,
    person,
    mixed
} menu_entry_type;

int crafting_inventory_t::select_items_to_use(const std::vector<component> &components, consume_flags flags, civec &selected_items)
{
    if(components.empty()) {
        return -1;
    }
    typedef std::vector<civec> civecvec;
    
    // for each component of the set, this array
    // contains a civec with all the candidate items that can be used
    // as component (basicly those items for witch
    // matches_type(component::type) return true).
    civecvec available_items(components.size());
    // List for the menu_vec below, contains the menu entries
    // that should be displayed. This is used to display possible
    // choises to the user.
    std::vector<std::string> options;
    // the menu_entry_type defines what we do if the user selects that menu entry.
    // the second value is just the index into the available_items array that belongs
    // to this option.
    std::vector< std::pair<menu_entry_type, int> > optionsIndizes;
    
    for (int i = 0; i < components.size(); i++) {
        // stores the candidates that can be used as components[i]
        civec &candids = available_items[i];
        // Requirement for components[i]
        const requirement req(components[i], flags != assume_components);
        // Collect candidates from map and from player
        // note: collect_candidates appends to the vector
        const int pc = collect_candidates(req, S_FROM_PLAYER, candids);
        const int mc = collect_candidates(req, S_FROM_MAP, candids);
        if(mc + pc < req.count) {
            // Not enough items of this type, the requirement can not
            // be fullfilled, so skip this component
            continue;
        }
        assert(candids.size() > 0); // must have at least one, otherwise mc+pc==0
        if(candids.size() == 1) {
            // Only one possible choice
            options.push_back(candids.front().to_string());
            optionsIndizes.push_back(std::make_pair(single_choise, i));
        } else if(has_different_types(candids)) {
            // several items to choose, and they are of different types, 
            // make another menu later if the user chooses this
            options.push_back(item_controller->find_template(req.type)->name + " (different items)");
            optionsIndizes.push_back(std::make_pair(ask_again, i));
        } else {
            // several items, but they are all of the same type, list by location,
            const std::string &name = item_controller->find_template(req.type)->name;
            // Rules here are: show entry for "on person",
            // for "nearby" (if any of them apply).
            // Also show the mixed entry, but only if none of the other two
            // are shown
            if(mc >= req.count) {
                options.push_back(name + " (nearby)");
                optionsIndizes.push_back(std::make_pair(nearby, i));
            }
            if(pc >= req.count) {
                options.push_back(name);
                optionsIndizes.push_back(std::make_pair(person, i));
            }
            if(mc < req.count && pc < req.count) {
                assert(mc + pc >= req.count);
                options.push_back(name + " (on person & nearby)");
                optionsIndizes.push_back(std::make_pair(mixed, i));
            }
        }
    }
    if(options.size() == 0) {
        debugmsg("Attempted to select_items_to_use with no available components!");
        return -1;
    }
    int selection = 0;
    if(options.size() != 1) {
        // no user-interaction needed if only one choise
        selection = menu_vec(false, "Use which component?", options) - 1;
    }
    assert(selection >= 0 && selection < options.size());
    assert(options.size() == optionsIndizes.size());
    const int index_of_component = optionsIndizes[selection].second;
    // The user has choosen this selection of items:
    civec &vec_to_consume_from = available_items[index_of_component];
    // They are based on components[index_of_component], make a requirement
    // from the_item.
    requirement reqToRemove(components[index_of_component], flags != assume_components);
    switch(optionsIndizes[selection].first) {
        case single_choise:
            // only one item in list, use this anyway, their is no choice
            reduce(reqToRemove, vec_to_consume_from);
            break;
        case nearby:
            // use only items from map
            filter(vec_to_consume_from, S_MAP);
            reduce(reqToRemove, vec_to_consume_from);
            break;
        case person:
            // use only items from player
            filter(vec_to_consume_from, S_PLAYER);
            reduce(reqToRemove, vec_to_consume_from);
            break;
        case mixed:
            // use items from both sources, no filtering or changes needed
            // This essential means use all items available
            reduce(reqToRemove, vec_to_consume_from);
            break;
        case ask_again:
            ask_for_items_to_use(reqToRemove, vec_to_consume_from);
            selected_items.swap(vec_to_consume_from);
            break;
        default:
            assert(false);
    }
    selected_items.swap(vec_to_consume_from);
    return index_of_component;
}

void crafting_inventory_t::reduce(requirement req, civec &candidates) {
    for(size_t i = 0; i < candidates.size(); i++) {
        if(req.count <= 0) {
            candidates.erase(candidates.begin() + i, candidates.end());
            break;
        }
    }
}

int crafting_inventory_t::consume(const std::vector<component> &x, consume_flags flags, std::list<item> &used_items) {
    civec vec_to_consume_from;
    const int index_of_component = select_items_to_use(x, flags, vec_to_consume_from);
    if(index_of_component >= 0) {
        const requirement reqToRemove(x[index_of_component], flags != assume_components);
        consume(reqToRemove, vec_to_consume_from, used_items);
    }
    return index_of_component;
}

void crafting_inventory_t::consume(requirement req, const civec &selected_items, std::list<item> &used_items) {
    for(civec::const_iterator a = selected_items.begin(); req.count > 0 && a != selected_items.end(); ++a) {
        // use up components and change the requirement according
        a->consume(g, p, req, used_items);
    }
    if(req.count > 0) {
        debugmsg("Their are still %d charges/amounts missing (not used) for %s", req.count, req.type.c_str());
    }
}

crafting_inventory_t::candidate_item crafting_inventory_t::ask_for_single_item(const civec &candidates)
{
    assert(candidates.size() != 0);
    if(candidates.size() == 1) {
        return candidates[0];
    } else if(all_equal(candidates)) {
        return candidates[0];
    }
    std::vector<std::string> options;
    for(civec::const_iterator a = candidates.begin(); a != candidates.end(); ++a) {
        options.push_back(a->to_string());
    }
    const int selection = menu_vec(false, _("Use which gizmo?"), options) - 1;
    assert(selection >= 0 && selection < candidates.size());
    return candidates[selection];
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, civec &candidates) {
    civec tmpVec;
    ask_for_items_to_use(req, candidates, tmpVec);
    tmpVec.swap(candidates);
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, const civec &candidates, civec &selected_candidates) {
    assert(candidates.size() > 0);
    int minCount = req(candidates[0].the_item);
    for(civec::const_iterator a = candidates.begin(); a != candidates.end(); ++a) {
        minCount = std::min(minCount, req(a->the_item));
    }
    // minCount means each item of candidates has at least
    // that much to give. Now if minCount >= req.count
    // than exactly one item of the vector is always enough
    // this changes the algorithm to not go to select/unselect
    // single items, but to directly select the item to use.
    if(minCount >= req.count) {
        const candidate_item ci = ask_for_single_item(candidates);
        selected_candidates.push_back(ci);
        return;
    }
    
    // sort to make the_item nicer looking
    // sorts by location first
    std::sort(const_cast<civec&>(candidates).begin(), const_cast<civec&>(candidates).end(), std::less<candidate_item>());
    // stores which items the user has selected
    std::vector<bool> selected(candidates.size(), false);
    // the (summed up) charges (as told by requirement::get_charges_or_amount)
    // of all selected items. If this reaches the required count, we can break out
    // of this loop.
    int cntFromSelectedOnes = 0;
    // FIXME make this better, like a real menu that is not regenerated in each loop
    while(true) {
        // FIXME condens identical lines (e.g. 10 x "2x4 (on person)" -> "10 x 2x4 (on person)")
        std::vector<std::string> options;
        for(size_t i = 0; i < candidates.size(); i++) {
            const std::string prefix(selected[i] ? "    using " : "not using ");
            options.push_back(prefix + candidates[i].to_string());
        }
        if(cntFromSelectedOnes >= req.count) {
            options.push_back("OK");
        }
        const int selection = menu_vec(false, "Select the components to use?", options) - 1;
        if(selection >= candidates.size()) {
            for(size_t i = 0; i < candidates.size(); i++) {
                if(selected[i]) {
                    selected_candidates.push_back(candidates[i]);
                }
            }
            return;
        }
        if(!selected[selection] && cntFromSelectedOnes >= req.count) {
            popup("You can't select more items, deselect some before selecting more");
        } else {
            selected[selection] = !selected[selection];
            if(selected[selection]) {
                cntFromSelectedOnes += req(candidates[selection].the_item);
            } else {
                cntFromSelectedOnes -= req(candidates[selection].the_item);
            }
        }
    }
}

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
        case LT_INVENTORY:
            CMP_IF(invlet);
            break;
        case LT_MAP:
            CMP_IF(mapitems->position.x);
            CMP_IF(mapitems->position.y);
            CMP_IF(mindex);
            break;
        case LT_VEHICLE_CARGO:
            CMP_IF(vitems->veh);
            CMP_IF(vitems->part_num);
            CMP_IF(iindex);
            break;
        case LT_VPART:
            CMP_IF(vpartitem->veh);
            CMP_IF(vpartitem->part_num);
            break;
        case LT_WEAPON:
            break;
        case LT_BIONIC:
            CMP_IF(bionic->bio_id);
            break;
        case LT_SURROUNDING:
            CMP_IF(mapitems->position.x);
            CMP_IF(mapitems->position.y);
            CMP_IF(surroundings->the_item.type->name);
            break;
    }
    if(the_item.type != other.the_item.type) {
        return the_item.type->name < other.the_item.type->name;
    }
    return ::memcmp(this, &other, sizeof(*this));
}
#undef CMP_IF

bool crafting_inventory_t::candidate_item::is_source(source_flags sources) const {
    switch(location) {
        case LT_VEHICLE_CARGO: return sources & S_VEHICLE;
        case LT_VPART: return sources & S_VEHICLE;
        case LT_MAP: return sources & S_MAP;
        case LT_SURROUNDING: return sources & S_MAP;
        case LT_WEAPON: return sources & S_PLAYER;
        case LT_INVENTORY: return sources & S_PLAYER;
        case LT_BIONIC: return sources & S_PLAYER;
//        default: return false;
    }
    assert(false);
    return false;
}

std::string crafting_inventory_t::candidate_item::to_string() const {
    switch(location) {
        case LT_VEHICLE_CARGO:
        case LT_VPART:
            return const_cast<item&>(the_item).tname() + " (in car)";
        case LT_MAP:
        case LT_SURROUNDING:
            return const_cast<item&>(the_item).tname() + " (nearby)";
        case LT_WEAPON:
        case LT_INVENTORY:
        case LT_BIONIC:
            return const_cast<item&>(the_item).tname();
//        default:
//            return const_cast<item&>(the_item).tname();
    }
    assert(false);
    return "";
}

int crafting_inventory_t::candidate_item::drainVehicle(const std::string &ftype, int amount, std::list<item> &ret) const {
    vehicle *veh = NULL;
    switch(location) {
        case LT_VEHICLE_CARGO:
            veh = vitems->veh;
            break;
        case LT_VPART:
            veh = vpartitem->veh;
            break;
        default:
            assert(false);
            return 0;
    }
    item tmp = item_controller->create(ftype, g->turn);
    tmp.charges = veh->drain(ftype, amount);
    amount -= tmp.charges;
    ret.push_back(tmp);
    return amount;
}

void crafting_inventory_t::candidate_item::consume(game *g, player *p, requirement &req, std::list<item> &used_items) const {
    assert(req.count > 0);
    if(req.ctype == C_AMOUNT) {
        switch(location) {
            case LT_INVENTORY:
                used_items.push_back(p->inv.remove_item_by_letter(invlet));
                break;
            case LT_VEHICLE_CARGO:
                used_items.push_back(vitems->veh->parts[vitems->part_num].items[iindex]);
                vitems->veh->remove_item(vitems->part_num, iindex);
                break;
            case LT_MAP:
                used_items.push_back(g->m.i_at(mapitems->position.x, mapitems->position.y)[mindex]);
                g->m.i_rem(mapitems->position.x, mapitems->position.y, mindex);
                break;
            case LT_WEAPON:
                used_items.push_back(p->weapon);
                p->remove_weapon();
                break;
            case LT_VPART:
                debugmsg("attempted to consume a pseudo vehicle part item %s", the_item.name.c_str());
                break; // can not remove surroundings pseudo-item
            case LT_SURROUNDING:
                debugmsg("attempted to consume a pseudo surrounding item %s", the_item.name.c_str());
                break; // can not remove surroundings pseudo-item
            case LT_BIONIC:
                debugmsg("attempted to consume a pseudo bionic item %s", the_item.name.c_str());
                break; // can not remove bionic pseudo-item
            default:
                debugmsg("dont know what to consume!");
                break;
        }
        req.count--;
        return;
    }
    switch(location) {
        case LT_INVENTORY:
            if(p->inv.item_by_letter(invlet).use_charges(req.type, req.count, used_items, true)) {
                p->inv.remove_item_by_letter(invlet);
            }
            break;
        case LT_VEHICLE_CARGO:
            if(vitems->veh->parts[vitems->part_num].items[iindex].use_charges(req.type, req.count, used_items, true)) {
                vitems->veh->remove_item(vitems->part_num, iindex);
            }
            break;
        case LT_MAP:
            if(g->m.i_at(mapitems->position.x, mapitems->position.y)[iindex].use_charges(req.type, req.count, used_items, true)) {
                g->m.i_rem(mapitems->position.x, mapitems->position.y, mindex);
            }
            break;
        case LT_WEAPON:
            if(p->weapon.use_charges(req.type, req.count, used_items, true)) {
                p->remove_weapon();
            }
            break;
        case LT_VPART:
            if(vpartitem->veh->part_flag(vpartitem->part_num, "KITCHEN")) {
                if(req.type == "func:hotplate") {
                    const int modi = the_item.type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, used_items);
                    req.count -= remains / modi;
                } else if(req.type == "water_clean") {
                    drainVehicle("water", req.count, used_items);
                }
            }
            if(vpartitem->veh->part_flag(vpartitem->part_num, "WELDRIG")) {
                if(req.type == "func:hotplate" || req.type == "func:soldering_iron") {
                    const int modi = the_item.type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, used_items);
                    req.count -= remains / modi;
                }
            }
            break;
        case LT_SURROUNDING:
            // Basicly this is an inifinte amount of things
            // like fire, or a water source, in this case we can ignore the_item.
            break;
        case LT_BIONIC:
            if(req.count >= p->power_level) {
                req.count -= p->power_level;
                p->power_level = 0;
            } else {
                p->power_level -= req.count;
                req.count = 0;
            }
            // FIXME: return and used_up item
            break;
        default:
            debugmsg("dont know what to consume!");
            break;
    }
}

bool crafting_inventory_t::all_equal(const civec &candidates) {
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


// basicly: ignore the item if we count by charges (not amount) and the item available_items no charges left
#define XMATCH(item_) \
    ((item_).matches_type(req.type) && (req.ctype == C_AMOUNT || (item_).charges > 0))

int crafting_inventory_t::collect_candidates(const requirement &req, int sources, civec &candidates) {
    int count = 0;
    if(sources & S_MAP) {
        for(std::list<items_on_map>::iterator a = on_map.begin(); a != on_map.end(); ++a) {
            std::vector<item> &items = *(a->items);
            for(std::vector<item>::iterator b = items.begin(); b != items.end(); ++b) {
                if(XMATCH(*b)) {
                    count += req(*b);
                    candidates.push_back(candidate_item(*a, b - items.begin(), *b));
                }
            }
        }
    }
    if(sources & S_VEHICLE) {
        for(std::list<items_in_vehicle_cargo>::iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            std::vector<item> &items = *a->items;
            for(size_t b = 0; b < items.size(); b++) {
                if(XMATCH(items[b])) {
                    count += req(items[b]);
                    candidates.push_back(candidate_item(*a, b, items[b]));
                }
            }
        }
        for(std::list<item_from_vpart>::iterator a = vpart.begin(); a != vpart.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
                candidates.push_back(candidate_item(*a));
            }
        }
    }
    if(sources & S_PLAYER) {
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
                ++stack_iter) {
                if(XMATCH(*stack_iter)) {
                    count += req(*stack_iter);
                    candidates.push_back(candidate_item(stack_iter->invlet, *stack_iter));
                }
            }
        }
        if(XMATCH(p->weapon)) {
            count += req(p->weapon);
            candidates.push_back(candidate_item(p->weapon));
        }
        for(std::list<item_from_bionic>::iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
                candidates.push_back(candidate_item(*a));
            }
        }
    }
    if(sources & S_MAP) {
        for(std::list<item_from_surrounding>::iterator a = surround.begin(); a != surround.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
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

int crafting_inventory_t::requirement::get_charges_or_amount(const item &the_item) const {
    if(ctype == C_CHARGES) {
        return the_item.get_charges_of(type);
    } else if(the_item.matches_type(type)) {
        return 1;
    } else {
        return 0;
    }
}


#if 0
std::list<item> crafting_inventory_t::p_use_charges(const itype_id &type, int amount) {
    for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
        if(a->the_item.matches_type(type)) {
            return p->use_charges(a->the_item.type->id, amount);
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
        // the_item does not matter if one of those is super-mega-good,
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
    const item *the_item = 0;
    const double modi = compute_time_modi(tools, the_item);
	// Debug info, ignore for now
	/*
    if(the_item != 0) {
        debugmsg("tool that got used: %s - modi is %f", const_cast<item*>(the_item)->tname().c_str(), modi);
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
        const item *the_item = 0;
        const double modi = compute_time_modi(type, a->count, the_item);
        if(modi > 0.0 && (best_modi == 0.0 || best_modi > modi)) {
            best_modi = modi;
            item_that_got_used = the_item;
        }
    }
    if(best_modi > 0.0) {
        return best_modi;
    }
    return 1.0;
}

void use_modi(const item &the_item, const itype_id &type, int count, double &best_modi, const item *&item_that_got_used) {
    if(!the_item.matches_type(type)) {
        return;
    }
    if(the_item.get_charges_of(type) < count) {
        return;
    }
//    g->add_msg("use %s?", const_cast<item&>(the_item).tname().c_str());
    const double modi = the_item.get_functionality_time_modi(type);
    if(modi > 0.0 && (best_modi == 0.0 || best_modi > modi)) {
        best_modi = modi;
        item_that_got_used = &the_item;
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
    for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
        const std::vector<item> &items = *(a->items);
        for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
            ::use_modi(*b, type, count, best_modi, item_that_got_used);
        }
    }
    for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
        ::use_modi(a->the_item, type, count, best_modi, item_that_got_used);
    }
    for(std::list<item_from_surrounding>::const_iterator a = surround.begin(); a != surround.end(); ++a) {
        ::use_modi(a->the_item, type, count, best_modi, item_that_got_used);
    }
    if(best_modi > 0.0) {
        return best_modi;
    }
    return 1.0;
}

bool hasQuality(const item &the_item, const std::string &name, int level)
{
    const std::map<std::string, int> &qualities = the_item.type->qualities;
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
    for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
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
        const item &the_item = a->the_item;
        if(::hasQuality(the_item, name, level)) {
            amount--;
            if(amount <= 0) {
                return true;
            }
        }
    }
    for(std::list<item_from_surrounding>::const_iterator a = surround.begin(); a != surround.end(); ++a) {
        if(::hasQuality(a->the_item, name, level)) {
            amount--;
            if(amount <= 0) {
                return true;
            }
        }
    }
    return amount <= 0;
}
