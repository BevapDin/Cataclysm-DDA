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

bool crafting_inventory_t::has_all_requirements(recipe &making) {
    bool isSomethingMissing = false;
    for(size_t i = 0; i < making.components.size(); i++) {
        if(!has_any_components(making.components[i])) {
            isSomethingMissing = true;
        }
    }
    for(size_t i = 0; i < making.tools.size(); i++) {
        if(!has_any_tools(making.tools[i])) {
            isSomethingMissing = true;
        }
    }
    return !isSomethingMissing;
}

bool crafting_inventory_t::has(const requirement &req, source_flags sources) const {
    return count(req.type, req.ctype, req.count, sources) >= req.count;
}

int crafting_inventory_t::count(const requirement &req, source_flags sources) const {
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

int amount_of(const itype_id &type, const item &the_item)
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

int charges_of(const itype_id &type, const item &the_item)
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
    if(sources & LT_WEAPON) {
        COUNT_IT_(p->weapon);
    }
    if(sources & LT_INVENTORY) {
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
                ++stack_iter) {
                    COUNT_IT_(*stack_iter);
            }
        }
    }
    if(sources & LT_MAP) {
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
    if(sources & LT_VEHICLE_CARGO) {
        for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            const std::vector<item> &items = *(a->items);
            for(std::vector<item>::const_iterator b = items.begin(); b != items.end(); ++b) {
                if(b->made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(*b);
            }
        }
    }
    if(sources & LT_VPART) {
        for(std::list<item_from_vpart>::const_iterator a = vpart.begin(); a != vpart.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    if(sources & LT_BIONIC) {
        for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    if(sources & LT_SURROUNDING) {
        for(std::list<item_from_surrounding>::const_iterator a = surround.begin(); a != surround.end(); ++a) {
            COUNT_IT_(a->the_item);
        }
    }
    return count;
}
#undef COUNT_IT_

crafting_inventory_t::candidate_t::candidate_t(player *p, const itype_id &type):
location(LT_WEAPON),
weapon(&(p->weapon)),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(items_on_map &ifm, int i, const item &vpitem, const itype_id &type):
location(LT_MAP),
mapitems(&ifm),
mindex(i),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(player *p, char ch, const item &vpitem, const itype_id &type):
location(LT_INVENTORY),
the_player(p),
invlet(ch),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(items_in_vehicle_cargo &ifv, int i, const item &vpitem, const itype_id &type):
location(LT_VEHICLE_CARGO),
vitems(&ifv),
iindex(i),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(item_from_vpart &ifv, const itype_id &type):
location(LT_VPART),
vpartitem(&ifv),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(item_from_bionic &ifb, const itype_id &type):
location(LT_BIONIC),
bionic(&ifb),
usageType(type)
{ postInit(); }

crafting_inventory_t::candidate_t::candidate_t(item_from_surrounding &ifs, const itype_id &type):
location(LT_SURROUNDING),
surroundings(&ifs),
usageType(type)
{ postInit(); }

bool crafting_inventory_t::candidate_t::valid() const {
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
            return weapon != NULL;
        case LT_INVENTORY:
            return the_player != NULL && !the_player->inv.item_by_letter(invlet).is_null();
        case LT_BIONIC:
            return bionic != NULL;
        default:
            return false;
    }
}

#include "picojson.h"
void crafting_inventory_t::candidate_t::deserialize(crafting_inventory_t &cinv, const std::string &data) {
    picojson::value pjv;
    const char *s = data.c_str();
    std::string err = picojson::parse(pjv, s, s + data.length());
    assert(err.empty());
    deserialize(cinv, pjv);
}

void crafting_inventory_t::candidate_t::postInit() {
    assert(valid());
    timeModi = get_item().type->getTimeModi(usageType);
}

void crafting_inventory_t::candidate_t::deserialize(crafting_inventory_t &cinv, const picojson::value &pjv) {
    using namespace picojson;
    assert(pjv.is<object>());
    assert(pjv.contains("location"));
    assert(pjv.get("location").is<int>());
    location = (LocationType) pjv.get("location").get<double>();
    assert(pjv.contains("utype"));
    assert(pjv.get("utype").is<std::string>());
    assert(pjv.contains("tmodi"));
    assert(pjv.get("tmodi").is<double>());
    usageType = pjv.get("utype").get<std::string>();
    timeModi = pjv.get("tmodi").get<double>();
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
                    break;
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
            weapon = &(cinv.p->weapon);
            break;
        case LT_INVENTORY:
            the_player = cinv.p;
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
            debugmsg("Unknown %d location in candidate_t::deserialize", (int) location);
            break;
    }
    if(valid()) {
        postInit();
    }
}

const item &crafting_inventory_t::candidate_t::get_item() const {
    static const item null_item;
    if(!valid()) {
        return null_item;
    }
    switch(location) {
        case LT_MAP:
            return (*mapitems->items)[mindex];
            break;
        case LT_SURROUNDING:
            return surroundings->the_item;
            break;
        case LT_VEHICLE_CARGO:
            return (*vitems->items)[iindex];
            break;
        case LT_VPART:
            return vpartitem->the_item;
            break;
        case LT_WEAPON:
            return *weapon;
        case LT_INVENTORY:
            return the_player->inv.item_by_letter(invlet);
        case LT_BIONIC:
            return bionic->the_item;
        default:
            debugmsg("Unknown %d location in candidate_t::get_item", (int) location);
            return null_item;
    }
}

std::string crafting_inventory_t::candidate_t::serialize() const {
    picojson::value v;
    serialize(v);
    return v.serialize();
}

#define pv(x) picojson::value(x)
void crafting_inventory_t::candidate_t::serialize(picojson::value &v) const {
    picojson::object pjmap;
    pjmap["location"] = pv((int) location);
    pjmap["utype"] = pv(usageType);
    pjmap["tmodi"] = pv(timeModi);
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
            debugmsg("Unknown %d location in candidate_t", (int) location);
            break;
    }
    v = picojson::value(pjmap);
}
#undef pv

bool crafting_inventory_t::has_different_types(const candvec &cv) {
    for(size_t i = 0; i + 1 < cv.size(); i++) {
        if(cv[i].get_item().type != cv[i + 1].get_item().type) {
            return true;
        }
    }
    return false;
}

void crafting_inventory_t::filter(candvec &cv, source_flags sources) {
    for(size_t i = 0; i < cv.size(); ) {
        if(!cv[i].is_source(sources)) {
            cv.erase(cv.begin() + i);
        } else {
            i++;
        }
    }
}

void crafting_inventory_t::consume_gathered(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec, std::list<item> &used_items) {
    size_t indexInStrVec = 0;
    for (int i = 0; i < components.size(); i++) {
        const std::vector<component> &cv = components[i];
        if(cv.empty()) {
            continue;
        }
        candvec vec_to_consume_from;
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
            consume(reqToRemove, flags, vec_to_consume_from, used_items);
        } else {
            debugmsg("no suitable component recovered");
        }
    }
    if(indexInStrVec == strVec.size() && indexInStrVec > 0) {
        strVec.clear();
    } else if(indexInStrVec > 0 && indexInStrVec < strVec.size()) {
        strVec.erase(strVec.begin(), strVec.begin() + indexInStrVec);
    }
}

void crafting_inventory_t::consume_gathered(
    const recipe &making,
    player_activity &activity,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    consume_gathered(making.components, assume_components, activity.str_values, used_items);
    consume_gathered(making.tools, assume_tools, activity.str_values, used_tools);
}

int crafting_inventory_t::deserialize(const std::string &data, candvec &vec) {
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
        candidate_t ci(*this, pjv.get(i));
        if(!ci.valid()) {
            debugmsg("failed to deserialize");
            vec.clear();
            return -1;
        }
        vec.push_back(ci);
    }
    return index;
}

std::string crafting_inventory_t::serialize(int index, const candvec &vec) {
    assert(!vec.empty());
    std::ostringstream buffer;
    buffer << "[" << index;
    for(size_t i = 0; i < vec.size(); i++) {
        buffer << "," << vec[i].serialize();
    }
    buffer << "]";
    return buffer.str();
}

void crafting_inventory_t::gather_inputs(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec, double *timeModi)
{
    for (int i = 0; i < components.size(); i++) {
        gather_inputs(components[i], flags, strVec, timeModi);
    }
}

void crafting_inventory_t::gather_inputs(const std::vector<component> &cv, consume_flags flags, std::vector<std::string> &strVec, double *timeModi)
{
    if(cv.empty()) {
        return;
    }
    candvec selected_items;
    const int index_of_component = select_items_to_use(cv, flags, selected_items);
    if(index_of_component < 0) {
        assert(index_of_component == -1); // Is a fixed value, not calculated!
        strVec.push_back(std::string(""));
        return;
    }
    assert(index_of_component < cv.size());
    strVec.push_back(serialize(index_of_component, selected_items));
    if(timeModi == NULL) {
        return;
    }
    const double modi = calc_time_modi(cv[index_of_component].type, selected_items);
    if(modi == 1.0 || modi == 0.0) {
        return;
    }
    assert(modi > 0);
    // We must take the worst (largest) modi here, because
    // if several tools are required (think hammer+screwdriver)
    // it does not matter if one of those is super-mega-good,
    // if the other is bad and damaged
    if(*timeModi == 0.0 || *timeModi < modi) {
        *timeModi = modi;
    }
}

double crafting_inventory_t::calc_time_modi(const itype_id &type, const candvec &tools) const {
    double worst_modi = 0.0;
    for(candvec::const_iterator a = tools.begin(); a != tools.end(); a++) {
        const double modi = a->timeModi;
        if(modi == 1.0 || modi == 0.0) {
            continue;
        }
        assert(modi > 0);
        if(worst_modi == 0.0 || worst_modi < modi) {
            g->add_msg("Tool %s has time modi %f", a->get_item().name.c_str(), modi);
            worst_modi = modi;
        }
    }
    return worst_modi;
}

void crafting_inventory_t::gather_input(const recipe &making, player_activity &activity) {
    double toolfactor = 0.0;
    gather_inputs(making.tools, assume_tools, activity.str_values, &toolfactor);
    gather_inputs(making.components, assume_components, activity.str_values, NULL);
    if(toolfactor > 0.0 && toolfactor != 1.0) {
        int move_points = activity.moves_left;
        move_points = static_cast<int>(move_points * toolfactor);
        g->add_msg("craft-factors: tool: %f", toolfactor);
        activity.moves_left = move_points;
    }
}

void copy(const std::vector<component> *src, std::vector< std::vector<component> > &dest, size_t count) {
    dest.clear();
    for(size_t i = 0; i < count; i++, src++) {
        if(!src->empty()) {
            dest.push_back(*src);
        }
    }
}

void crafting_inventory_t::gather_input(const construction_stage &stage, player_activity &activity) {
    double toolfactor = 0.0;
    std::vector< std::vector<component> > tmpvec;
    copy(stage.tools, tmpvec, sizeof(stage.tools) / sizeof(stage.tools[0]));
    gather_inputs(tmpvec, assume_tools, activity.str_values, &toolfactor);
    copy(stage.components, tmpvec, sizeof(stage.components) / sizeof(stage.components[0]));
    gather_inputs(tmpvec, assume_components, activity.str_values, NULL);
    if(toolfactor > 0.0 && toolfactor != 1.0) {
        int move_points = activity.moves_left;
        move_points = static_cast<int>(move_points * toolfactor);
        g->add_msg("construction-factors: tool: %f", toolfactor);
        activity.moves_left = move_points;
    }
}

void crafting_inventory_t::consume_gathered(
    const construction_stage &stage,
    player_activity &activity,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    std::vector< std::vector<component> > tmpvec;
    copy(stage.tools, tmpvec, sizeof(stage.tools) / sizeof(stage.tools[0]));
    consume_gathered(tmpvec, assume_components, activity.str_values, used_items);
    copy(stage.components, tmpvec, sizeof(stage.components) / sizeof(stage.components[0]));
    consume_gathered(tmpvec, assume_tools, activity.str_values, used_tools);
}

typedef enum {
    single_choise,
    ask_again,
    nearby,
    person,
    mixed
} menu_entry_type;

int crafting_inventory_t::select_items_to_use(const std::vector<component> &components, consume_flags flags, candvec &selected_items)
{
    if(components.empty()) {
        return -1;
    }
    typedef std::vector<candvec> candvecvec;
    
    // for each component of the set, this array
    // contains a candvec with all the candidate items that can be used
    // as component (basicly those items for witch
    // matches_type(component::type) return true).
    candvecvec available_items(components.size());
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
        candvec &candids = available_items[i];
        // Requirement for components[i]
        const requirement req(components[i], flags != assume_components);
        // Collect candidates from map and from player
        // note: collect_candidates appends to the vector
        const int pc = collect_candidates(req, S_PLAYER, candids);
        const int mc = collect_candidates(req, S_MAP, candids);
        if(mc + pc < req.count) {
            // Not enough items of this type, the requirement can not
            // be fullfilled, so skip this component
            continue;
        }
        assert(candids.size() > 0); // must have at least one, otherwise mc+pc==0
        if(candids.size() == 1) {
            // Only one possible choice
            options.push_back(candids.front().to_string(false));
            optionsIndizes.push_back(std::make_pair(single_choise, i));
        } else if(has_different_types(candids)) {
            // several items to choose, and they are of different types, 
            // make another menu later if the user chooses this
            options.push_back(item_controller->find_template(req.type)->name + " (different items)");
            optionsIndizes.push_back(std::make_pair(ask_again, i));
        } else if(!all_equal(candids)) {
            // several items, but they are all of the same type,
            // but with differing properties - list by location,
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
    candvec &vec_to_consume_from = available_items[index_of_component];
    assert(!vec_to_consume_from.empty());
    // They are based on components[index_of_component], make a requirement
    // from it.
    requirement reqToRemove(components[index_of_component], flags != assume_components);
    switch(optionsIndizes[selection].first) {
        case single_choise:
            // only one item in list, use this anyway, their is no choice
            reduce(reqToRemove, vec_to_consume_from);
            assert(!vec_to_consume_from.empty());
            break;
        case nearby:
            // use only items from map
            filter(vec_to_consume_from, S_MAP);
            assert(!vec_to_consume_from.empty());
            reduce(reqToRemove, vec_to_consume_from);
            assert(!vec_to_consume_from.empty());
            break;
        case person:
            // use only items from player
            filter(vec_to_consume_from, S_PLAYER);
            assert(!vec_to_consume_from.empty());
            reduce(reqToRemove, vec_to_consume_from);
            assert(!vec_to_consume_from.empty());
            break;
        case mixed:
            // use items from both sources, no filtering or changes needed
            // This essential means use all items available
            reduce(reqToRemove, vec_to_consume_from);
            assert(!vec_to_consume_from.empty());
            break;
        case ask_again:
            ask_for_items_to_use(reqToRemove, flags, vec_to_consume_from);
            assert(!vec_to_consume_from.empty());
            break;
        default:
            assert(false);
    }
    selected_items.swap(vec_to_consume_from);
    return index_of_component;
}

void crafting_inventory_t::reduce(requirement req, candvec &candidates) {
    assert(!candidates.empty());
    assert(req.count > 0);
    for(size_t i = 0; i < candidates.size(); i++) {
        if(req.count <= 0) {
            candidates.erase(candidates.begin() + i, candidates.end());
            break;
        }
        req.count -= req(candidates[i].get_item());
    }
}

int crafting_inventory_t::consume(const std::vector<component> &x, consume_flags flags, std::list<item> &used_items) {
    candvec vec_to_consume_from;
    const int index_of_component = select_items_to_use(x, flags, vec_to_consume_from);
    if(index_of_component >= 0) {
        const requirement reqToRemove(x[index_of_component], flags != assume_components);
        consume(reqToRemove, flags, vec_to_consume_from, used_items);
    }
    return index_of_component;
}

void crafting_inventory_t::consume(requirement req, consume_flags flags, const candvec &selected_items, std::list<item> &used_items) {
    if(req.ctype == C_AMOUNT && flags != assume_components) {
        // Basicly the requirement says: "need tool X, no charges required"
        // and the tool is not used up, so we can skip this here.
        return;
    }
    for(candvec::const_iterator a = selected_items.begin(); req.count > 0 && a != selected_items.end(); ++a) {
        // use up components and change the requirement according
        a->consume(g, p, req, used_items);
    }
    if(req.count > 0) {
        debugmsg("Their are still %d charges/amounts missing (not used) for %s", req.count, req.type.c_str());
    }
}

crafting_inventory_t::candidate_t crafting_inventory_t::ask_for_single_item(const requirement &req, consume_flags flags, const candvec &candidates)
{
    assert(candidates.size() != 0);
    if(all_equal(candidates)) {
        return candidates[0];
    }
    std::vector<std::string> options;
    std::vector<int> indizes;
    std::set<std::string> wehadethis;
    bool allHaveEmptyPrefix = true;
    for(candvec::const_iterator a = candidates.begin(); a != candidates.end(); ++a) {
        const std::string displayString = a->to_string(flags != assume_components);
        if(wehadethis.count(displayString) > 0) {
            continue;
        }
        wehadethis.insert(displayString);
        if(allHaveEmptyPrefix && displayString.compare(0, 3 + 3, "      ") != 0) {
            // "      " == 3 spaces == "XXX % "
            allHaveEmptyPrefix = false;
        }
        options.push_back(displayString);
        indizes.push_back(a - candidates.begin());
    }
    int selection = 0;
    if(options.size() != 1) {
        if(allHaveEmptyPrefix) {
            for(size_t i = 0; i < options.size(); i++) {
                options[i].erase(0, 3 + 3);
            }
        }
        selection = menu_vec(false, _("Use which gizmo?"), options) - 1;
    }
    assert(selection >= 0 && selection < indizes.size());
    return candidates[indizes[selection]];
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, consume_flags flags, candvec &candidates) {
    candvec tmpVec;
    ask_for_items_to_use(req, flags, candidates, tmpVec);
    tmpVec.swap(candidates);
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, consume_flags flags, const candvec &candidates, candvec &selected_candidates) {
    assert(candidates.size() > 0);
    int minCount = req(candidates[0].get_item());
    for(candvec::const_iterator a = candidates.begin(); a != candidates.end(); ++a) {
        minCount = std::min(minCount, req(a->get_item()));
    }
    // minCount means each item of candidates has at least
    // that much to give. Now if minCount >= req.count
    // than exactly one item of the vector is always enough
    // this changes the algorithm to not go to select/unselect
    // single items, but to directly select the item to use.
    if(minCount >= req.count) {
        const candidate_t ci = ask_for_single_item(req, flags, candidates);
        selected_candidates.push_back(ci);
        return;
    }
    
    // sort to make it nicer looking
    // sorts by location first
    std::sort(const_cast<candvec&>(candidates).begin(), const_cast<candvec&>(candidates).end(), std::less<candidate_t>());
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
            options.push_back(prefix + candidates[i].to_string(flags != assume_components));
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
                cntFromSelectedOnes += req(candidates[selection].get_item());
            } else {
                cntFromSelectedOnes -= req(candidates[selection].get_item());
            }
        }
    }
}




#define CMP_IF(_w) \
    do { if(_w != other._w) { return _w < other._w; } } while(false)
bool crafting_inventory_t::candidate_t::operator<(const candidate_t &other) const {
    if(timeModi != 0.0 && timeModi != 1.0 && other.timeModi != 1.0 && other.timeModi != 0.0) {
        CMP_IF(timeModi);
    }
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
    if(get_item().type != other.get_item().type) {
        return get_item().type->name < other.get_item().type->name;
    }
    return ::memcmp(this, &other, sizeof(*this));
}
#undef CMP_IF

std::string crafting_inventory_t::candidate_t::to_string(bool withTime) const {
    std::ostringstream buffer;
    if(withTime) {
        if(timeModi != 1.0 && timeModi != 0.0) {
            const int pc = (int) (timeModi * 100);
            if(pc < 100) {
                buffer << " ";
            }
            buffer << pc << " %% ";
        } else {
            buffer << "   " << "   "; // "XXX % "
        }
    }
    buffer << const_cast<item&>(get_item()).tname();
    switch(location) {
        case LT_VEHICLE_CARGO:
        case LT_VPART:
            buffer << " (in car)";
            break;
        case LT_MAP:
        case LT_SURROUNDING:
            buffer << " (nearby)";
            break;
        case LT_WEAPON:
        case LT_INVENTORY:
            break;
        case LT_BIONIC:
            buffer << " (bionic)";
            break;
        default:
            assert(false);
    }
    return buffer.str();
}

int crafting_inventory_t::candidate_t::drainVehicle(const std::string &ftype, int amount, std::list<item> &ret) const {
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

void crafting_inventory_t::candidate_t::consume(game *g, player *p, requirement &req, std::list<item> &used_items) const {
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
            // Below are pseudo item. They should not be used in recipes
            // as they can not be removed (used up).
            case LT_VPART:
                debugmsg("attempted to consume a pseudo vehicle part item %s", get_item().name.c_str());
                break; // can not remove surroundings pseudo-item
            case LT_SURROUNDING:
                debugmsg("attempted to consume a pseudo surrounding item %s", get_item().name.c_str());
                break; // can not remove surroundings pseudo-item
            case LT_BIONIC:
                debugmsg("attempted to consume a pseudo bionic item %s", get_item().name.c_str());
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
            if(get_item().type->id == "installed_kitchen_unit") {
                if(req.type == "func:hotplate") {
                    const int modi = get_item().type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, used_items);
                    req.count -= remains / modi;
                } else if(req.type == "water_clean") {
                    drainVehicle("water", req.count, used_items);
                }
            }
            /*
            if(vpartitem->veh->part_flag(vpartitem->part_num, "WELDRIG")) {
                if(req.type == "func:hotplate" || req.type == "func:soldering_iron") {
                    const int modi = get_item().type->getChargesModi(req.type);
                    const int remains = drainVehicle("battery", req.count * modi, used_items);
                    req.count -= remains / modi;
                }
            }
            */
            break;
        case LT_SURROUNDING:
            // Basicly this is an inifinte amount of things
            // like fire, or a water source, in this case we can ignore it.
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

bool crafting_inventory_t::all_equal(const candvec &candidates) {
    for(int i = 0; i + 1 < candidates.size(); i++) {
        const candidate_t &prev = candidates[i];
        const candidate_t &cur = candidates[i + 1];
        if(prev.get_item().type != cur.get_item().type) {
            return false;
        }
        if(prev.get_item().damage != cur.get_item().damage) {
            return false;
        }
        if(prev.get_item().burnt != cur.get_item().burnt) {
            return false;
        }
        if(prev.get_item().bigness != cur.get_item().bigness) {
            return false;
        }
        // FIXME: are there properties that need to be compared?
    }
    return true;
}


// basicly: ignore the item if we count by charges (not amount) and the item available_items no charges left
#define XMATCH(item_) \
    ((item_).matches_type(req.type) && (req.ctype == C_AMOUNT || (item_).charges > 0))

int crafting_inventory_t::collect_candidates(const requirement &req, int sources, candvec &candidates) {
    int count = 0;
    if(sources & LT_MAP) {
        for(std::list<items_on_map>::iterator a = on_map.begin(); a != on_map.end(); ++a) {
            std::vector<item> &items = *(a->items);
            for(std::vector<item>::iterator b = items.begin(); b != items.end(); ++b) {
                if(XMATCH(*b)) {
                    count += req(*b);
                    candidates.push_back(candidate_t(*a, b - items.begin(), *b, req.type));
                }
            }
        }
    }
    if(sources & LT_VEHICLE_CARGO) {
        for(std::list<items_in_vehicle_cargo>::iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            std::vector<item> &items = *a->items;
            for(size_t b = 0; b < items.size(); b++) {
                if(XMATCH(items[b])) {
                    count += req(items[b]);
                    candidates.push_back(candidate_t(*a, b, items[b], req.type));
                }
            }
        }
    }
    if(sources & LT_VPART) {
        for(std::list<item_from_vpart>::iterator a = vpart.begin(); a != vpart.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
                candidates.push_back(candidate_t(*a, req.type));
            }
        }
    }
    if(sources & LT_INVENTORY) {
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end();
                ++stack_iter) {
                if(XMATCH(*stack_iter)) {
                    count += req(*stack_iter);
                    candidates.push_back(candidate_t(p, stack_iter->invlet, *stack_iter, req.type));
                }
            }
        }
    }
    if(sources & LT_WEAPON) {
        if(XMATCH(p->weapon)) {
            count += req(p->weapon);
            candidates.push_back(candidate_t(p, req.type));
        }
    }
    if(sources & LT_BIONIC) {
        for(std::list<item_from_bionic>::iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
                candidates.push_back(candidate_t(*a, req.type));
            }
        }
    }
    if(sources & LT_SURROUNDING) {
        for(std::list<item_from_surrounding>::iterator a = surround.begin(); a != surround.end(); ++a) {
            if(XMATCH(a->the_item)) {
                count += req(a->the_item);
                candidates.push_back(candidate_t(*a, req.type));
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
#if 0
void crafting_inventory_t::form_from_map(game *g, point origin, int range)
{
    int junk = 0;
    for (int x = origin.x - range; x <= origin.x + range; x++) {
        for (int y = origin.y - range; y <= origin.y + range; y++) {
            if (g->m.has_flag(sealed, x, y) ||
                ((origin.x != x || origin.y != y) &&
                !g->m.clear_path( origin.x, origin.y, x, y, range, 1, 100, junk ) ) ) {
                continue;
            }
            const point p(x, y);
            items_on_map items(p, &(g->m.i_at(x, y)));
            if(!items.items->empty()) {
                on_map.push_back(items);
            }

            // Kludges for now!
            ter_id terrain_id = g->m.ter(x, y);
            if ((g->m.field_at(x, y).findField(fd_fire)) || (terrain_id == t_lava)) {
                item fire(g->itypes["fire"], 0);
                fire.charges = 1;
                surround.push_back(item_from_surrounding(p, fire));
            }
            if (terrain_id == t_water_sh || terrain_id == t_water_dp) {
                item water(g->itypes["water"], 0);
                water.charges = 50;
                surround.push_back(item_from_surrounding(p, water));
            }

            int vpart = -1;
            vehicle *veh = g->m.veh_at(x, y, vpart);
            // include only non-moving cars. This ensures that the car
            // is still there _after_ the crafting.
            if (veh && veh->cruise_velocity == 0 && veh->velocity == 0) {
                const int kpart = veh->part_with_function(vpart, vpc_kitchen);
                if (kpart >= 0) {
                    item kitchen(g->itypes["installed_kitchen_unit"], 0);
                    kitchen.charges = veh->fuel_left("battery", true);
                    this->vpart.push_back(item_from_vpart(veh, kpart, kitchen));

                    item water(g->itypes["water_clean"], 0);
                    water.charges = veh->fuel_left("water");
                    this->vpart.push_back(item_from_vpart(veh, kpart, water));
                }

                const int cpart = veh->part_with_function(vpart, vpc_cargo);
                if (cpart >= 0) {
                    items_in_vehicle_cargo inveh(veh, cpart, &(veh->parts[cpart].items));
                    if(!inveh.items->empty()) {
                        in_veh.push_back(inveh);
                    }
                }
            }
        }
    }
}
#endif













bool crafting_inventory_t::has_all_components(std::vector<component> &comps) const {
    bool isSomethingMissing = false;
    for(size_t i = 0; i < comps.size(); i++) {
        if(!has_components(comps[i])) {
            isSomethingMissing = true;
        }
    }
    return comps.empty() || !isSomethingMissing;
}

bool crafting_inventory_t::has_any_components(std::vector<component> &comps) const {
    bool somethingFound = false;
    for(size_t i = 0; i < comps.size(); i++) {
        if(has_components(comps[i])) {
            somethingFound = true;
        }
    }
    return comps.empty() || somethingFound;
}

bool crafting_inventory_t::has_components(const itype_id &type, int count) const {
    component comp(type, count);
    return has_components(comp);
}

bool crafting_inventory_t::has_components(component &comps) const {
    return has(comps, false);
}



bool crafting_inventory_t::has_all_tools(std::vector<component> &tools) const {
    bool isSomethingMissing = false;
    for(size_t i = 0; i < tools.size(); i++) {
        if(!has_tools(tools[i])) {
            isSomethingMissing = true;
        }
    }
    return tools.empty() || !isSomethingMissing;
}

bool crafting_inventory_t::has_any_tools(std::vector<component> &tools) const {
    bool somethingFound = false;
    for(size_t i = 0; i < tools.size(); i++) {
        if(has_tools(tools[i])) {
            somethingFound = true;
        }
    }
    return tools.empty() || somethingFound;
}

bool crafting_inventory_t::has_tools(const itype_id &type, int count) const {
    component comp(type, count);
    return has_tools(comp);
}

bool crafting_inventory_t::has_tools(const itype_id &type) const {
    component comp(type, -1);
    return has_tools(comp);
}

bool crafting_inventory_t::has_tools(component &tools) const {
    return has(tools, true);
}



std::list<item> crafting_inventory_t::consume_any_tools(const std::vector<component> &tools, bool force_available)
{
    std::list<item> result;
    consume(tools, force_available ? assume_tools_force_available : assume_tools, result);
    return result;
}

std::list<item> crafting_inventory_t::consume_all_tools(const std::vector<component> &tools, bool force_available)
{
    std::list<item> result;
    for(size_t i = 0; i < tools.size(); i++) {
        std::vector<component> tmpcomps(1, tools[i]);
        consume(tmpcomps, force_available ? assume_tools_force_available : assume_tools, result);
    }
    return result;
}

std::list<item> crafting_inventory_t::consume_tools(const component &tools, bool force_available)
{
    std::vector<component> tmpcomps(1, tools);
    std::list<item> result;
    consume(tmpcomps, force_available ? assume_tools_force_available : assume_tools, result);
    return result;
}

std::list<item> crafting_inventory_t::consume_tools(const itype_id &type, int count, bool force_available) {
    component comp(type, count);
    return consume_tools(comp, force_available);
}

std::list<item> crafting_inventory_t::consume_tools(const itype_id &type) {
    component comp(type, -1);
    return consume_tools(comp, true);
}



std::list<item> crafting_inventory_t::consume_any_components(const std::vector<component> &comps)
{
    std::list<item> result;
    consume(comps, assume_components, result);
    return result;
}

std::list<item> crafting_inventory_t::consume_all_components(const std::vector<component> &comps)
{
    std::list<item> result;
    for(size_t i = 0; i < comps.size(); i++) {
        std::vector<component> tmpcomps(1, comps[i]);
        consume(tmpcomps, assume_components, result);
    }
    return result;
}

std::list<item> crafting_inventory_t::consume_components(const component &comps)
{
    std::vector<component> tmpcomps(1, comps);
    std::list<item> result;
    consume(tmpcomps, assume_components, result);
    return result;
}

std::list<item> crafting_inventory_t::consume_components(const itype_id &type, int count) {
    component comp(type, count);
    return consume_components(comp);
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
