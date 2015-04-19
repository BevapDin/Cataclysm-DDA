#include "crafting_inventory_t.h"
#include "messages.h"
#include "game.h"
#include "bionics.h"
#include "debug.h"
#include "map.h"
#include "player.h"
#include "item_factory.h"
#include "inventory.h"
#include "crafting.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <sstream>

void resort_item_vectors();
   
void multiply(requirement_data &r, int n) {
    for(auto &a : r.tools) {
        for(auto &b : a) {
			if(b.count > 0) {
				b.count *= n;
			}
        }
    }
    for(auto &a : r.components) {
        for(auto &b : a) {
			if(b.count > 0) {
				b.count *= n;
			}
        }
    }
}

std::ostream &operator<<(std::ostream &buffer, const crafting_inventory_t::requirement &req);

crafting_inventory_t::crafting_inventory_t(player *p)
: p(p)
{
    init(PICKUP_RANGE);
}

crafting_inventory_t::crafting_inventory_t(player *p, int range)
: p(p)
{
    init(range);
}

void crafting_inventory_t::init(int range)
{
    if(range != -1) {
        assert(range >= 0);
        form_from_map(p->pos3(), range);
    }
    // iterator of all bionics of the player and grab the toolsets automaticly
    // This allows easy addition of more toolsets
    for(std::vector<bionic>::const_iterator a = p->getMyBionics().begin(); a != p->getMyBionics().end(); ++a) {
        add_bio_toolset(*a, calendar::turn);
    }
}

void crafting_inventory_t::add_bio_toolset(const bionic &bio, const calendar &turn) {
    static const std::string prefixIn("bio_tools_");
    static const std::string prefixOut("toolset_");
    if(bio.id.compare(0, prefixIn.length(), prefixIn) != 0) {
        return;
    }
    // FIXME: bionics _share_ the power!
    const std::string tool_name = prefixOut + bio.id.substr(prefixIn.length());
    item tools(tool_name, turn);
    tools.charges = p->power_level;
    by_bionic.push_back(item_from_bionic(bio.id, tools));
}

typedef std::pair<itype_id, float> funcWithModi;
typedef std::vector<funcWithModi> tidvec;
typedef std::map<itype_id, tidvec> funcmap;
const tidvec &get_tidvec(const itype_id &type) {
    static funcmap functionTypes;
    tidvec &types = functionTypes[type];
    if(types.empty()) {
        for( auto p : item_controller->get_all_itypes() ) {
            auto t = p.second;
            if(!t->hasFunc(type)) {
                if(type.compare(5, std::string::npos, t->id) == 0) {
                    types.push_back(funcWithModi(p.first, 1.0f));
                }
                continue;
            }
            types.push_back(funcWithModi(p.first, t->getChargesModi(type)));
        }
    }
    return types;
}

bool crafting_inventory_t::has_all_requirements(const requirement_data &making, int batch_size) {
    requirement_data tmp = making;
    multiply(tmp, batch_size);
    return has_all_requirements(tmp);
}

bool crafting_inventory_t::has_all_requirements(const requirement_data &making) {
    solution s;
    return has_all_requirements(making, s);
}

std::string crafting_inventory_t::complex_req::serialize() const {
    assert(!simple_reqs.empty());
    assert(!selected_items.empty());
    assert((size_t) selected_simple_req_index < simple_reqs.size());
    return crafting_inventory_t::serialize(selected_simple_req_index, selected_items);
}

void crafting_inventory_t::complex_req::deserialize(crafting_inventory_t &cinv, JsonArray &arr) {
    selected_simple_req_index = cinv.deserialize(arr, selected_items);
    if((size_t) selected_simple_req_index >= simple_reqs.size()) {
        if(as_tool) {
            popup(_("a tool you used has vanished while crafting"));
        } else {
            popup(_("a component you used has vanished while crafting"));
        }
        selected_simple_req_index = -1;
        selected_items.clear();
    } else {
        simple_reqs[selected_simple_req_index].available = a_true;
        simple_reqs[selected_simple_req_index].comp->available = a_true;
    }
}

void crafting_inventory_t::complex_req::consume(crafting_inventory_t &cinv, std::list<item> &used_items) {
    if(selected_simple_req_index == -1) {
        return;
    }
    assert((size_t) selected_simple_req_index < simple_reqs.size());
    cinv.consume(simple_reqs[selected_simple_req_index].req, as_tool ? assume_tools : assume_components , selected_items, used_items);
}

void crafting_inventory_t::solution::serialize(player_activity &activity) const {
    std::ostringstream buffer;
    buffer << "[";
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        if(i != 0) { buffer << ","; }
        buffer << complex_reqs[i].serialize();
    }
    buffer << "]";
    activity.str_values.push_back(buffer.str());
}

void crafting_inventory_t::solution::deserialize(crafting_inventory_t &cinv, player_activity &activity) {
    if(activity.str_values.empty()) {
        debugmsg("player_activity::str_values is empty");
        return;
    }
    const std::string data = activity.str_values.front();
    std::istringstream buffer(data);
    activity.str_values.erase(activity.str_values.begin());
    try {
        JsonIn json(buffer);
        JsonArray arr(json);
        if((size_t) arr.size() != complex_reqs.size()) {
            debugmsg("failed to deserialize: input %s is too small", data.c_str());
            return;
        }
        for(size_t i = 0; i < complex_reqs.size(); i++) {
            complex_req &cr = complex_reqs[i];
            JsonArray a(arr.get_array(i));
            cr.deserialize(cinv, a);
        }
    } catch(const std::string &err) {
        debugmsg("failed to deserialize: %s", err.c_str());
    }
}

void crafting_inventory_t::solution::consume(crafting_inventory_t &cinv, std::list<item> &used_items, std::list<item> &used_tools) {
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_req &cr = complex_reqs[i];
        if(cr.selected_simple_req_index != -1) {
            cr.consume(cinv, cr.as_tool ? used_tools : used_items);
            complex_reqs.erase(complex_reqs.begin() + i);
            i--;
        } else {
            debugmsg("Must gather requirement %d again", i);
        }
    }
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_req &cr = complex_reqs[i];
        cr.gather(cinv, true);
    }
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_req &cr = complex_reqs[i];
        cr.select_items_to_use();
        cr.consume(cinv, cr.as_tool ? used_tools : used_items);
    }
    resort_item_vectors();
}

void crafting_inventory_t::solution::consume(crafting_inventory_t &cinv, std::list<item> &used) {
    consume(cinv, used, used);
}

void crafting_inventory_t::solution::select_items_to_use() {
    toolfactor = 0.0;
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_reqs[i].select_items_to_use();
        merge_time_modi(complex_reqs[i].toolfactor, toolfactor);
    }
}

void crafting_inventory_t::gather_and_consume(const requirement_data &making, std::list<item> &used_items, std::list<item> &used_tools) {
    solution s;
    gather_and_consume(making, s, used_items, used_tools);
}

void crafting_inventory_t::gather_and_consume(
    const requirement_data &making,
    solution &s,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    if(making.tools.empty() && making.components.empty()) {
        return;
    }
    s.init(making);
    s.gather(*this, true);
    s.select_items_to_use();
    s.consume(*this, used_items, used_tools);
}

void crafting_inventory_t::gather_input(const requirement_data &making, player_activity &activity) {
    solution s;
    gather_input(making, s, activity);
}

void crafting_inventory_t::gather_input(const requirement_data &making, player_activity &activity, int batch_size) {
    requirement_data tmp = making;
    multiply(tmp, batch_size);
    gather_input(tmp, activity);
}

void crafting_inventory_t::consume_gathered(
    const requirement_data &making,
    player_activity &activity,
    int batch_size,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    requirement_data tmp = making;
    multiply(tmp, batch_size);
    consume_gathered(tmp, activity, used_items, used_tools);
}

void crafting_inventory_t::consume_gathered(
    const requirement_data &making,
    player_activity &activity,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    solution s;
    consume_gathered(making, s, activity, used_items, used_tools);
}

void crafting_inventory_t::consume_gathered(
    const requirement_data &making,
    solution &s,
    player_activity &activity,
    std::list<item> &used_items,
    std::list<item> &used_tools
) {
    if(making.tools.empty() && making.components.empty()) {
        return;
    }
    s.init(making);
    s.deserialize(*this, activity);
    s.consume(*this, used_items, used_tools);
}

void crafting_inventory_t::gather_input(const requirement_data &making, solution &s, player_activity &activity) {
    if(making.tools.empty() && making.components.empty()) {
        return;
    }
    s.init(making);
    s.gather(*this, true);
    s.select_items_to_use();
    s.serialize(activity);
    const double toolfactor = s.toolfactor;
    if(toolfactor > 0.0 && toolfactor != 1.0) {
        int move_points = activity.moves_left;
        move_points = static_cast<int>(move_points * toolfactor);
        add_msg("craft-factors: tool: %f", toolfactor);
        activity.moves_left = move_points;
    }
}

template<typename T>
crafting_inventory_t::solution::solution(const std::vector<T> &comps, crafting_inventory_t &cinv) {
    init_need_any(comps);
    gather(cinv, false);
}

template<typename T>
crafting_inventory_t::solution::solution(const T &comp, crafting_inventory_t &cinv) {
    init_single_req(comp);
    gather(cinv, false);
}

template<typename T>
void crafting_inventory_t::solution::init_need_any(const std::vector<T> &comps) {
    complex_reqs.resize(1);
    complex_reqs[0].init(comps, *this);
    complex_reqs[0].init_pointers();
}

template<typename T>
void crafting_inventory_t::solution::init_single_req(const T &comp) {
    complex_reqs.resize(1);
    complex_reqs[0].init(comp, *this);
    complex_reqs[0].init_pointers();
}

void crafting_inventory_t::solution::init(const requirement_data &making) {
    complex_reqs.resize(making.tools.size() + making.components.size());
    size_t nr = 0;
    for(size_t i = 0; i < making.tools.size(); i++, nr++) {
        complex_reqs[nr].init(making.tools[i], *this);
    }
    for(size_t i = 0; i < making.components.size(); i++, nr++) {
        complex_reqs[nr].init(making.components[i], *this);
    }
    erase_empty_reqs();
    init_pointers();
    find_overlays();
}

void crafting_inventory_t::solution::init_pointers() {
    // Set up the pointers to the parent
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_reqs[i].init_pointers();
    }
}

void crafting_inventory_t::solution::erase_empty_reqs() {
    assert(!complex_reqs.empty());
    // Some complex_req might end up empty, because their content
    // been merged into other other complex_req.
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        if(complex_reqs[i].simple_reqs.empty()) {
            complex_reqs.erase(complex_reqs.begin() + i);
            i--;
        }
    }
    assert(!complex_reqs.empty());
}

void crafting_inventory_t::solution::find_overlays() {
    // set up overlays
    assert(!complex_reqs.empty());
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_req &rc1 = complex_reqs[i];
        for(size_t j = 0; j < rc1.simple_reqs.size(); j++) {
            simple_req &rc2 = rc1.simple_reqs[j];
            for(size_t j1 = j + 1; j1 < rc1.simple_reqs.size(); j1++) {
                simple_req &rc3 = rc1.simple_reqs[j1];
                rc3.find_overlays(rc2);
            }
        }
        for(size_t j = 0; j < rc1.simple_reqs.size(); j++) {
            simple_req &rc2 = rc1.simple_reqs[j];
            for(size_t i1 = i + 1; i1 < complex_reqs.size(); i1++) {
                complex_req &rc3 = complex_reqs[i1];
                for(size_t j1 = 0; j1 < rc3.simple_reqs.size(); j1++) {
                    rc3.simple_reqs[j1].find_overlays(rc2);
                }
            }
        }
    }
}

bool operator==(const crafting_inventory_t::requirement &a, const crafting_inventory_t::requirement &b) {
    return a.ctype == b.ctype && a.type == b.type && a.count == b.count;
}

void crafting_inventory_t::complex_req::init_pointers() {
    assert(!simple_reqs.empty());
    for(size_t k = 0; k < simple_reqs.size(); k++) {
        simple_reqs[k].parent = this;
    }
}

bool crafting_inventory_t::complex_req::contains_req_type(const itype_id &type) const {
    for(size_t k = 0; k < simple_reqs.size(); k++) {
        if(simple_reqs[k].req.type == type) {
            return true;
        }
    }
    return false;
}

crafting_inventory_t::simple_req* crafting_inventory_t::complex_req::get_req_type(const itype_id &type) {
    for(size_t k = 0; k < simple_reqs.size(); k++) {
        if(simple_reqs[k].req.type == type) {
            return &(simple_reqs[k]);
        }
    }
    return NULL;
}

bool crafting_inventory_t::simple_req::merge(const requirement &otherReq) {
    assert(req.type == otherReq.type);
    if(req.ctype != otherReq.ctype) {
        return false;
    }
    req.count = std::max(req.count, otherReq.count);
    return true;
}

void crafting_inventory_t::complex_req::add_or_merge(const simple_req &rs2) {
    assert(rs2.req.type.compare(0, 5, "func:") != 0);
    simple_req *ot = get_req_type(rs2.req.type);
    if(ot != NULL && ot->merge(rs2.req)) {
        if(
            ot->comp->type.compare(0, 5, "func:") == 0
            && rs2.comp->type.compare(0, 5, "func:") != 0) {
            ot->comp = rs2.comp;
        }
        // a item that has this function is explictly listed,
        // or a item has several functions.
        return;
    }
    simple_reqs.push_back(rs2);
}

void crafting_inventory_t::complex_req::add(const component &c) {
    c.available = a_false;
    requirement req(c, as_tool);
    simple_req rs2(req, &c);
    if(rs2.req.type.compare(0, 5, "func:") != 0) {
        add_or_merge(rs2);
        return;
    }
    const tidvec &types = get_tidvec(rs2.req.type);
    assert(!types.empty());
    const int org_count = rs2.req.count;
    for(size_t i = 0; i < types.size(); i++) {
        simple_req rs3(rs2);
        rs3.req.type = types[i].first;
        if(req.ctype == C_CHARGES) {
            const float modi = types[i].second;
            if(modi == -1.0f) {
                // No charges required for this tool
                rs3.req.ctype = C_AMOUNT;
                rs3.req.count = 1;
            } else if(modi == 0.0f) {
                // Does not apply for use as charged object
                continue;
            } else {
                rs3.req.count = static_cast<int>(std::ceil(org_count * modi));
                if(rs3.req.count <= 0) {
                    // Need at least a charge, nothing is free
                    rs3.req.count = 1;
                }
            }
        }
        add_or_merge(rs3);
    }
}

bool cmp_simple_req(const crafting_inventory_t::simple_req &a, const crafting_inventory_t::simple_req &b) {
    if(a.req.ctype != b.req.ctype) {
        return a.req.ctype == crafting_inventory_t::C_AMOUNT;
    }
    if(a.req.count != b.req.count) {
        return a.req.count < b.req.count;
    }
    return a.req.type < b.req.type;
}

template<typename T>
void crafting_inventory_t::complex_req::init(const std::vector<T> &components, solution &s) {
    (void) s;
    assert(!components.empty());
    as_tool = std::is_same<const tool_comp,const T>::value;
    for(size_t j = 0; j < components.size(); j++) {
        add(components[j]);
    }
    assert(!simple_reqs.empty());
    std::sort(simple_reqs.begin(), simple_reqs.end(), cmp_simple_req);
}

template<typename T>
void crafting_inventory_t::complex_req::init(const T &components, solution &s) {
    (void) s;
    as_tool = std::is_same<const tool_comp,const T>::value;
    add(components);
    assert(!simple_reqs.empty());
}

void crafting_inventory_t::simple_req::find_overlays(simple_req &rc) {
    if(rc.req.type != req.type) {
        return;
    }
    if(parent->as_tool && rc.parent->as_tool) {
        // Both are used as tool, if they require an amount,
        // there is no real overlay, a tool can be used
        // knife and as screwdriver and as hammer.
        if(rc.req.ctype == C_AMOUNT || req.ctype == C_AMOUNT) {
            return;
        }
    }
    overlays.push_back(&rc);
}

bool crafting_inventory_t::solution::is_possible() const {
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        if(!complex_reqs[i].is_possible()) {
            return false;
        }
    }
    return true;
}

bool crafting_inventory_t::complex_req::is_possible() const {
    assert(!simple_reqs.empty());
    for(size_t i = 0; i < simple_reqs.size(); i++) {
        if(simple_reqs[i].is_possible()) {
            return true;
        }
    }
    return false;
}

bool crafting_inventory_t::simple_req::is_possible() const {
    return available == a_true;
}

void crafting_inventory_t::solution::gather(crafting_inventory_t &cinv, bool store) {
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        complex_reqs[i].gather(cinv, store);
    }
}

void crafting_inventory_t::complex_req::gather(crafting_inventory_t &cinv, bool store) {
    assert(!simple_reqs.empty());
    for(size_t i = 0; i < simple_reqs.size(); i++) {
        simple_reqs[i].gather(cinv, store);
    }
    for(size_t i = 0; i < simple_reqs.size(); i++) {
        simple_req &sr = simple_reqs[i];
        sr.comp->available = (available_status) std::max<int>(sr.available, sr.comp->available);
    }
}

crafting_inventory_t::candidate_t crafting_inventory_t::candidate_t::split(const requirement &req, int count_to_remove) {
    assert(count_to_remove > 0);
    assert(valid());
    if(location != LT_INVENTORY || invcount == 1) {
        // Non-locations can not be splited, nor can single item stacks (invcount==1)
        return candidate_t();
    }
    if(req.ctype == C_AMOUNT) {
        if(invcount == count_to_remove) {
            // Splitting would result in this.invcount == 0
            return candidate_t();
        }
        invcount -= count_to_remove;
        return candidate_t(the_player, invpos, count_to_remove, usageType);
    }
    const int charges_per_item = req(get_item());
    const int total_charges = charges_per_item * invcount;
    const int remaining_charges = total_charges - count_to_remove;
    const int remaining_items = remaining_charges / charges_per_item;
    // ^^ note: rounds downwards -> remaining_charges = 99 and
    // charges_per_item = 100 -> remaining_items == 0
    if(remaining_items <= 0) {
        // There would remain less than a full items worth of charges, split
        // split would use up all the items
        return candidate_t();
    }
    assert(remaining_items < invcount);
    const int items_to_remove = invcount - remaining_items;
    invcount -= items_to_remove;
    return candidate_t(the_player, invpos, items_to_remove, usageType);
}

void crafting_inventory_t::simple_req::recount_candidate_sources() {
    cnt_on_map = 0;
    cnt_on_player = 0;
    for(candvec::const_iterator a = candidate_items.begin(); a != candidate_items.end(); ++a) {
        const candidate_t &can = *a;
        if(can.is_source(S_MAP)) {
            cnt_on_map += req(can);
        } else {
            assert(can.is_source(S_PLAYER));
            cnt_on_player = req(can);
        }
    }
}

bool sort_by_charges(const crafting_inventory_t::candidate_t &a, const crafting_inventory_t::candidate_t &b) {
    const item &ia = a.get_item();
    const item &ib = b.get_item();
    return ia.charges < ib.charges;
}

void crafting_inventory_t::simple_req::move_as_required_to(simple_req &other) {
    int cnt_reqired = other.req.count;
    // Include those items already in the candidate list
    other.recount_candidate_sources();
    cnt_reqired -= other.cnt_on_map + other.cnt_on_player;
    for(candvec::iterator a = candidate_items.begin(); cnt_reqired > 0 && a != candidate_items.end(); /*empty*/) {
        candidate_t &can = *a;
        const candidate_t splitted = can.split(other.req, cnt_reqired);
        if(!splitted.valid()) {
            // Move the candidate completely
            other.candidate_items.push_back(can);
            cnt_reqired -= other.req(can);
            a = candidate_items.erase(a);
        } else {
            // Move only part of it
            other.candidate_items.push_back(splitted);
            break;
        }
    }
    recount_candidate_sources();
    other.recount_candidate_sources();
    for(candvec::iterator a = other.candidate_items.begin(); a != other.candidate_items.end(); a++) {
        a->usageType = other.comp->type;
    }
}

void crafting_inventory_t::simple_req::separate(simple_req &other) {
    if(req.ctype == C_AMOUNT && other.req.ctype == C_AMOUNT) {
        // We need req.count item and other.req.count items
        // Note: collect_candidates has created the same list of
        // candidates for both simple_req objects!
        /**
         * How it works:
         * Go through the list of candidates of this and
         * _move_ some of them into the list of candidates of the
         * other simple_req.
         * cnt_reqired stores the required count that must still be
         * added to the other simple_req.
         */
        other.candidate_items.clear();
        move_as_required_to(other);
    } else if(req.ctype == C_CHARGES && other.req.ctype == C_AMOUNT) {
        other.separate(*this); // same case other direction
    } else if(req.ctype == C_AMOUNT && other.req.ctype == C_CHARGES) {
        /**
         * How it works:
         * Go through the list the candidates of other and
         * _move_ some of them into the list of candidates of this.
         * (Only those that have no charges).
         * cnt_reqired stores the required count that must still be
         * added to this simple_req.
         * Also the candidate list is sorted by charges first
         * (items with smaller charges first), therfor  if there are items
         * without charges those get moved to this simple_req and not
         * those with huge charges.
         * Note: this.candidate_items might contain some entries that
         * are not in other.candidate_items because items with no charges
         * at all would not be included when counting by charges,
         * but they would be include when counting by amount.
         * Therfor we remove those items first.
         */
        for(candvec::iterator a = candidate_items.begin(); a != candidate_items.end(); /*empty*/) {
            const candidate_t &can = *a;
            if(other.req(can) > 0) {
                // Assume that this candidate is included in the
                // candidate_items of the other anyway.
                a = candidate_items.erase(a);
            } else {
                ++a;
            }
        }
        std::sort(other.candidate_items.begin(), other.candidate_items.end(), sort_by_charges);
        other.move_as_required_to(*this);
    } else if(req.ctype == C_CHARGES && other.req.ctype == C_CHARGES) {
        /**
         * No need to separate here, while consuming we consume until
         * the required charges have been used up and don't care where they
         * come from.
         */
    } else {
        assert(false);
    }
}

void crafting_inventory_t::simple_req::set_unavailable(available_status av) {
    candidate_items.clear();
    cnt_on_player = 0;
    cnt_on_map = 0;
    available = av;
}

void crafting_inventory_t::simple_req::gather(crafting_inventory_t &cinv, bool store) {
    if(req.count == 0) {
        comp->available = a_true;
        return;
    }
    assert(req.type.compare(0, 5, "func:") != 0);
    comp->available = a_false;
    if(store) {
        candidate_items.clear();
        cnt_on_player = cinv.collect_candidates(req, S_PLAYER, candidate_items);
        cnt_on_map = cinv.collect_candidates(req, S_MAP, candidate_items);
        for(candvec::iterator a = candidate_items.begin(); a != candidate_items.end(); ++a) {
            a->usageType = comp->type;
        }
        available = ((cnt_on_map + cnt_on_player) >= req.count) ? a_true : a_false;
    } else {
        available = cinv.has(req) ? a_true : a_false;
    }
    for(size_t j = 0; available == a_true && j < overlays.size(); j++) {
        check_overlay(cinv, store, *overlays[j]);
    }
}

bool crafting_inventory_t::complex_req::has_alternativ(const simple_req &sr) const {
    assert(!simple_reqs.empty());
    for(size_t i = 0; i < simple_reqs.size(); i++) {
        if(&(simple_reqs[i]) == &sr) {
            continue;
        }
        if(simple_reqs[i].is_possible()) {
            return true;
        }
    }
    return false;
}

void crafting_inventory_t::simple_req::check_overlay(crafting_inventory_t &cinv, bool store, simple_req &other) {
    assert(req.type == other.req.type);
    if(other.available != a_true) {
        // Might be possible if other needs more items than this
        return;
    }
    if(req.ctype == other.req.ctype) {
        requirement newReq = req;
        newReq.count += other.req.count;
        if(cinv.has(newReq)) {
            if(store) {
                separate(other);
            }
            return;
        }
        // Not enough for both
    } else {
        // count by different things.
        // Make temporary copies and gather components.
        simple_req _this(req, comp, parent);
        simple_req _other(other.req, other.comp, other.parent);
        if(store) {
            _this.candidate_items = this->candidate_items;
            _other.candidate_items = other.candidate_items;
        } else {
            _this.gather(cinv, true);
            _other.gather(cinv, true);
        }
        // Now separte them
        _this.separate(_other);
        // Now look if both are still possible.
        if(_this.req_is_fullfilled() && _other.req_is_fullfilled()) {
            // Both are possible, store changed candidates
            if(store) {
                _this.candidate_items.swap(this->candidate_items);
                _other.candidate_items.swap(other.candidate_items);
            }
            return;
        }
        if(!_this.req_is_fullfilled() && !_other.req_is_fullfilled()) {
            // Neither is possible - is this case even possible?
            set_unavailable(a_insufficent);
            other.set_unavailable(a_insufficent);
            return;
        }
    }
    // Only one of both is possible, which one to choose?
    if(other.parent->has_alternativ(other)) {
        // we have an alternativ to the other, disable it.
        other.set_unavailable(a_insufficent);
        return;
    }
    // Nope the other component is needed, deselect this one.
    set_unavailable(a_insufficent);
}

std::ostream &operator<<(std::ostream &buffer, const crafting_inventory_t::requirement &req) {
    if(req.ctype == crafting_inventory_t::C_CHARGES) {
        buffer << item::nname(req.type) << " (" << req.count << ")";
    } else if(req.count == 1) {
        const std::string n = item::nname(req.type);
        assert(!n.empty());
        if(n[n.length() - 1] == 's') {
            buffer << n;
        } else {
            static const std::string vowels("aeiou");
            if(vowels.find(n[0]) != std::string::npos) {
                buffer << "an " << n;
            } else {
                buffer << "a " << n;
            }
        }
    } else {
        buffer << req.count << " " << item::nname(req.type, req.count);
    }
    return buffer;
}

void crafting_inventory_t::solution::save(requirement_data &making) const {
    making.components.clear();
    making.tools.clear();
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        const complex_req &rc = complex_reqs[i];
        if(rc.as_tool) {
            making.tools.resize(making.tools.size() + 1);
            for(size_t j = 0; j < rc.simple_reqs.size(); j++) {
                int count = rc.simple_reqs[j].req.count;
                if(rc.as_tool && rc.simple_reqs[j].req.ctype == C_AMOUNT) {
                    count = -1;
                }
                making.tools.back().push_back(tool_comp(rc.simple_reqs[j].req.type, count));
            }
        } else {
            making.components.resize(making.components.size() + 1);
            for(size_t j = 0; j < rc.simple_reqs.size(); j++) {
                int count = rc.simple_reqs[j].req.count;
                if(rc.as_tool && rc.simple_reqs[j].req.ctype == C_AMOUNT) {
                    count = -1;
                }
                making.components.back().push_back(item_comp(rc.simple_reqs[j].req.type, count));
            }
        }
    }
}

bool crafting_inventory_t::has_all_requirements(const requirement_data &making, solution &s) {
    if(making.tools.empty() && making.components.empty()) {
        return true;
    }
    s.init(making);
    s.gather(*this, false);
    return s.is_possible();
}

std::string avail_to_string(available_status a) {
    switch(a) {
        case a_false: return "-";
        case a_true: return "+";
        case a_insufficent: return "#";
    }
    return "?";
}

std::string crafting_inventory_t::solution::to_string(int flags) const {
    std::ostringstream buffer;
    for(size_t i = 0; i < complex_reqs.size(); i++) {
        const complex_req &rs = complex_reqs[i];
        if(i < 9) { buffer << " " << (i+1); } else { buffer << (i+1); }
        buffer << rs.to_string(flags) << "\n";
    }
    return buffer.str();
}

std::string crafting_inventory_t::complex_req::to_string(int flags) const {
    std::ostringstream buffer;
    const component* last_comp = NULL;
    for(size_t j = 0; j < simple_reqs.size(); j++) {
        const simple_req &rc = simple_reqs[j];
        
        if((flags & simple_req::ts_selected) != 0 && j != (size_t) selected_simple_req_index) {
            continue;
        }
        
        if((flags & simple_req::ts_compress) != 0) {
            if(rc.comp == last_comp) {
            } else {
                if(j != 0) { buffer << " OR\n  "; }
                last_comp = rc.comp;
                available_status avail = rc.available;
                for(size_t k = j + 1; avail != 1 && k < simple_reqs.size(); k++) {
                    if(simple_reqs[k].comp == last_comp) {
                        avail = (available_status)std::max<int>(avail, simple_reqs[k].available);
                    }
                }
                buffer << " " << avail_to_string(avail) << " ";
                buffer << requirement(*rc.comp, as_tool);
            }
        } else {
            if(j != 0) { buffer << " OR\n  "; }
            buffer << " " << avail_to_string(rc.available) << " ";
            buffer << rc.req;
            if(rc.req.type != rc.comp->type) {
                if(rc.comp->type.compare(5, rc.req.type.length(), rc.req.type) != 0) {
                    buffer << " (used as " << item::nname(rc.comp->type) << ")";
                }
            }
            
            if((flags & simple_req::ts_overlays) != 0 && !rc.overlays.empty()) {
                buffer << " overlays with: ";
                for(std::vector<simple_req*>::const_iterator a = rc.overlays.begin(); a != rc.overlays.end(); a++) {
                    assert(*a != NULL);
                    if(a != rc.overlays.begin()) { buffer << ", "; }
                    buffer << item::nname((*a)->req.type);
                }
            }
        }
        
        if(!rc.candidate_items.empty() && (flags & simple_req::ts_found_items) != 0) {
            buffer << "\n     found: ";
            bool needs_comma = false;
            for(candvec::const_iterator a = rc.candidate_items.begin(); a != rc.candidate_items.end(); ++a) {
                if(needs_comma) { buffer << ", "; } else { needs_comma = true; }
                buffer << a->to_string(false);
            }
        }
    }
    return buffer.str();
}

bool crafting_inventory_t::has(const requirement &req, source_flags sources) const {
    return count(req, req.count, sources) >= req.count;
}

int crafting_inventory_t::count(const requirement &req, source_flags sources) const {
    return count(req, -1, sources);
}

bool crafting_inventory_t::has(component &x, bool as_tool) const {
    if(has(requirement(x, as_tool))) {
        x.available = a_true;
    } else {
        x.available = a_false;
    }
    return x.available == a_true;
}

map_stack crafting_inventory_t::items_on_map::items() const {
    return m->i_at(position.x, position.y);
}

vehicle_stack crafting_inventory_t::items_in_vehicle_cargo::items() const {
    std::vector<int> parts = veh->parts_at_relative(mount_dx, mount_dy);
    for(size_t i = 0; i < parts.size(); i++) {
        if(veh->part_info(parts[i]).has_flag("CARGO")) {
            return veh->get_items( parts[i] );
        }
    }
    debugmsg("cargo part not found");
    static std::list<item> nulitems;
    return vehicle_stack( &nulitems, point(0, 0), nullptr, 0 );
}


#define COUNT_IT_(item_, factor) \
    do { \
        count += req(item_) * factor; \
        if(max > 0 && count >= max) { return count; } \
    } while(false)

int crafting_inventory_t::count(const requirement &req, int max, int sources) const
{
    CacheMap &cm = (req.ctype == C_CHARGES ? counted_by_charges : counted_by_amount);
    std::pair<CacheMap::iterator, bool> xf = cm.insert(CacheMap::value_type(req.type, 0));
    if(!xf.second) {
        return xf.first->second;
    }
    int &count = xf.first->second;
    max = -1; // force complete counting
    
    if(sources & LT_INVENTORY) {
        if(!p->weapon.is_null()) {
            COUNT_IT_(p->weapon, 1);
        }
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter) {
            assert(!iter->empty());
            COUNT_IT_(iter->front(), iter->size());
        }
        for(std::vector<item>::const_iterator iter = p->worn.begin(); iter != p->worn.end();
            ++iter) {
            COUNT_IT_(*iter, 1);
        }
    }
    if(sources & LT_MAP) {
        for(std::list<items_on_map>::const_iterator a = on_map.begin(); a != on_map.end(); ++a) {
            const auto items = a->items();
            for( const auto & b : items ) {
                if(b.made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(b, 1);
            }
        }
    }
    if(sources & LT_VEHICLE_CARGO) {
        for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            const auto items = a->items();
            for( const auto & b : items ) {
                if(b.made_of(LIQUID)) {
                    continue;
                }
                COUNT_IT_(b, 1);
            }
        }
    }
    if(sources & LT_VPART) {
        for(std::list<item_from_vpart>::const_iterator a = vpart.begin(); a != vpart.end(); ++a) {
            COUNT_IT_(a->the_item, 1);
        }
    }
    if(sources & LT_BIONIC) {
        for(std::list<item_from_bionic>::const_iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            COUNT_IT_(a->the_item, 1);
        }
    }
    if(sources & LT_SURROUNDING) {
        for(std::list<item_from_surrounding>::const_iterator a = surround.begin(); a != surround.end(); ++a) {
            COUNT_IT_(a->the_item, 1);
        }
    }
    return count;
}
#undef COUNT_IT_

crafting_inventory_t::candidate_t::candidate_t(items_on_map &ifm, int i, const itype_id &type):
location(LT_MAP),
mapitems(&ifm),
mindex(i),
usageType(type)
{ }

crafting_inventory_t::candidate_t::candidate_t(player *p, int pos, int count, const itype_id &type):
location(LT_INVENTORY),
the_player(p),
invpos(pos),
invcount(count),
usageType(type)
{ }

crafting_inventory_t::candidate_t::candidate_t(items_in_vehicle_cargo &ifv, int i, const itype_id &type):
location(LT_VEHICLE_CARGO),
vitems(&ifv),
iindex(i),
usageType(type)
{ }

crafting_inventory_t::candidate_t::candidate_t(item_from_vpart &ifv, const itype_id &type):
location(LT_VPART),
vpartitem(&ifv),
usageType(type)
{ }

crafting_inventory_t::candidate_t::candidate_t(item_from_bionic &ifb, const itype_id &type):
location(LT_BIONIC),
bionic(&ifb),
usageType(type)
{ }

crafting_inventory_t::candidate_t::candidate_t(item_from_surrounding &ifs, const itype_id &type):
location(LT_SURROUNDING),
surroundings(&ifs),
usageType(type)
{ }

bool crafting_inventory_t::candidate_t::valid() const {
    switch(location) {
        case LT_MAP:
            return mapitems != 0 && (size_t) mindex < mapitems->items().size();
        case LT_SURROUNDING:
            return surroundings != NULL;
        case LT_VEHICLE_CARGO:
            return vitems != NULL && (size_t) iindex < vitems->items().size();
        case LT_VPART:
            return vpartitem != NULL;
        case LT_INVENTORY:
            return the_player != NULL && invcount > 0 && !the_player->i_at(invpos).is_null();
        case LT_BIONIC:
            return bionic != NULL;
        default:
            return false;
    }
}

float crafting_inventory_t::candidate_t::get_time_modi() const {
    assert(valid());
    return get_item().type->getTimeModi(usageType);
}

void crafting_inventory_t::candidate_t::deserialize(crafting_inventory_t &cinv, JsonObject &obj) {
    assert(obj.has_int("location"));
    location = (LocationType) obj.get_int("location");
    assert(obj.has_string("utype"));
    usageType = obj.get_string("utype");
    tripoint tmppnt;
    tmppnt.z = g->get_levz();
    std::string tmpstr;
    int veh_ptr;
    switch(location) {
        case LT_MAP:
            tmppnt.x = obj.get_int("x");
            tmppnt.y = obj.get_int("y");
            tmppnt.z = obj.get_int("z");
            mapitems = NULL;
            for(std::list<items_on_map>::iterator a = cinv.on_map.begin(); a != cinv.on_map.end(); ++a) {
                if(a->position == tmppnt) {
                    mapitems = &(*a);
                    break;
                }
            }
            mindex = obj.get_int("index");
            break;
        case LT_SURROUNDING:
            tmppnt.x = obj.get_int("x");
            tmppnt.y = obj.get_int("y");
            tmppnt.z = obj.get_int("z");
            tmpstr = obj.get_string("type");
            surroundings = NULL;
            for(std::list<item_from_surrounding>::iterator a = cinv.surround.begin(); a != cinv.surround.end(); ++a) {
                if(a->position == tmppnt
                    && a->the_item.type->id == tmpstr
                ) {
                    surroundings = &(*a);
                    break;
                }
            }
            if(surroundings == NULL) {
//                debugmsg("surrounding %s is gone - will recreate it", tmpstr.c_str());
                item it(tmpstr, calendar::turn);
                it.charges = 50;
                cinv.surround.push_back(item_from_surrounding(tmppnt, it));
                surroundings = &(cinv.surround.back());
            }
            break;
        case LT_VEHICLE_CARGO:
            veh_ptr = obj.get_int("vehptr");
            tmppnt.x = obj.get_int("mount_dx");
            tmppnt.y = obj.get_int("mount_dy");
            vitems = NULL;
            for(std::list<items_in_vehicle_cargo>::iterator a = cinv.in_veh.begin(); a != cinv.in_veh.end(); ++a) {
                if(reinterpret_cast<int>(a->veh) == veh_ptr
                    && a->mount_dx == tmppnt.x
                    && a->mount_dy == tmppnt.y
                ) {
                    vitems = &(*a);
                    break;
                }
            }
            iindex = obj.get_int("index");
            break;
        case LT_VPART:
            veh_ptr = obj.get_int("vehptr");
            tmppnt.x = obj.get_int("mount_dx");
            tmppnt.y = obj.get_int("mount_dy");
            vpartitem = NULL;
            tmpstr = obj.get_string("type");
            for(std::list<item_from_vpart>::iterator a = cinv.vpart.begin(); a != cinv.vpart.end(); ++a) {
                if(reinterpret_cast<int>(a->veh) == veh_ptr
                    && a->mount_dx == tmppnt.x
                    && a->mount_dy == tmppnt.y
                    && a->the_item.type->id == tmpstr
                ) {
                    vpartitem = &(*a);
                    break;
                }
            }
            break;
        case LT_INVENTORY:
            the_player = cinv.p;
            invpos = obj.get_int("invpos");
            invcount = obj.get_int("invcount");
            break;
        case LT_BIONIC:
            bionic = NULL;
            tmpstr = obj.get_string("bio_id");
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
}

const item &crafting_inventory_t::candidate_t::get_item() const {
    static const item null_item;
    if(!valid()) {
        return null_item;
    }
    switch(location) {
        case LT_MAP:
            return mapitems->items()[mindex];
            break;
        case LT_SURROUNDING:
            return surroundings->the_item;
            break;
        case LT_VEHICLE_CARGO:
            return vitems->items()[iindex];
            break;
        case LT_VPART:
            return vpartitem->the_item;
            break;
        case LT_INVENTORY:
            return the_player->i_at(invpos);
        case LT_BIONIC:
            return bionic->the_item;
        default:
            debugmsg("Unknown %d location in candidate_t::get_item", (int) location);
            return null_item;
    }
}

void crafting_inventory_t::candidate_t::serialize(JsonOut &json) const {
    json.start_object();
    json.member("location", (int) location);
    json.member("utype", usageType);
    switch(location) {
        case LT_MAP:
            json.member("x", mapitems->position.x);
            json.member("y", mapitems->position.y);
            json.member("z", mapitems->position.z);
            json.member("index", mindex);
            break;
        case LT_SURROUNDING:
            json.member("x", surroundings->position.x);
            json.member("y", surroundings->position.y);
            json.member("z", surroundings->position.z);
            json.member("type", surroundings->the_item.type->id);
            break;
        case LT_VEHICLE_CARGO:
            // Note: this casts a pointer to int, this is not stable after
            // e.g. restarting.
            // BUT: this value is read in and searched for in the crafting_inventory_t,
            // if there is no matching vehicle found, than we reset the_item to NULL.
            json.member("vehptr", reinterpret_cast<int>(vitems->veh));
            json.member("mount_dx", vitems->mount_dx);
            json.member("mount_dy", vitems->mount_dy);
            json.member("index", iindex);
            break;
        case LT_VPART:
            // Note: see above case LT_VEHICLE_CARGO
            json.member("vehptr", reinterpret_cast<int>(vitems->veh));
            json.member("mount_dx", vitems->mount_dx);
            json.member("mount_dy", vitems->mount_dy);
            json.member("type", vpartitem->the_item.type->id);
            break;
        case LT_INVENTORY:
            json.member("invpos", invpos);
            json.member("invcount", invcount);
            break;
        case LT_BIONIC:
            json.member("bio_id", bionic->bio_id);
            break;
        default:
            debugmsg("Unknown %d location in candidate_t", (int) location);
            break;
    }
    json.end_object();
}

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

int crafting_inventory_t::deserialize(JsonArray &arr, candvec &vec) {
    if(arr.size() < 1) {
        debugmsg("failed to deserialize: input array is empty");
        return -1;
    }
    const int index = arr.get_int(0);
    vec.clear();
    for(size_t i = 1; i < arr.size(); i++) {
        JsonObject o(arr.get_object(i));
        candidate_t ci(*this, o);
        if(!ci.valid()) {
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
    JsonOut json(buffer);
    json.start_array();
    json.write(index);
    for(size_t i = 0; i < vec.size(); i++) {
        vec[i].serialize(json);
    }
    json.end_array();
    return buffer.str();
}

void crafting_inventory_t::gather_inputs(const std::vector< std::vector<component> > &components, consume_flags flags, std::vector<std::string> &strVec, double *timeModi)
{
    for (size_t i = 0; i < components.size(); i++) {
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
    assert((size_t) index_of_component < cv.size());
    strVec.push_back(serialize(index_of_component, selected_items));
    if(timeModi == NULL) {
        return;
    }
    merge_time_modi(calc_time_modi(selected_items), *timeModi);
}

void crafting_inventory_t::merge_time_modi(double modi, double &result) {
    if(modi == 1.0 || modi == 0.0) {
        return;
    }
    assert(modi > 0);
    // We must take the worst (largest) modi here, because
    // if several tools are required (think hammer+screwdriver)
    // it does not matter if one of those is super-mega-good,
    // if the other is bad and damaged
    if(result == 0.0 || result < modi) {
        result = modi;
    }
}

double crafting_inventory_t::calc_time_modi(const candvec &tools) {
    double worst_modi = 0.0;
    for(candvec::const_iterator a = tools.begin(); a != tools.end(); a++) {
        const double modi = a->get_time_modi();
        if(modi == 1.0 || modi == 0.0) {
            continue;
        }
        assert(modi > 0);
        if(worst_modi == 0.0 || worst_modi < modi) {
            add_msg("Tool %s has time modi %f", a->get_item().tname().c_str(), modi);
            worst_modi = modi;
        }
    }
    return worst_modi;
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
    
    for (size_t i = 0; i < components.size(); i++) {
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
            options.push_back(item::nname(req.type) + " (different items)");
            optionsIndizes.push_back(std::make_pair(ask_again, i));
        } else if(!all_equal(candids)) {
            // several items, but they are all of the same type,
            // but with differing properties - list by location,
            options.push_back(item::nname(req.type) + " (different items)");
            optionsIndizes.push_back(std::make_pair(ask_again, i));
        } else {
            // several items, but they are all of the same type, list by location,
            const std::string name = item::nname(req.type);
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
    assert((size_t) selection < options.size());
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

int sort_by_likeness(const item& a, const item& b) {
    if(!a.is_container() && b.is_container()) {
        return -1; // Prefer stuff outside of containers
    } else if(a.is_container() && !b.is_container()) {
        return +1; // same
    }
    if(a.is_container() && !a.contents.empty() && b.is_container() && !b.contents.empty()) {
        // if both are containers, sort by contents, if contents sort equal, sort by container
        // item type instead (to keep same container item together).
        const int r = sort_by_likeness(a.contents[0], b.contents[0]);
        if( r != 0 ) {
            return r;
        }
    }
    if( a.goes_bad() && b.goes_bad() ) {
        if( a.bday != b.bday ) {
            return a.bday < b.bday ? -1 : +1; // Prefer the older stuff
        }
    } else if( a.goes_bad() ) {
        return -1;
    } else if( b.goes_bad() ) {
        return +1;
    }
    if(a.charges != b.charges) {
        return a.charges < b.charges ? -1 : +1; // Prefer stuff with less charges
    }
    if( a.damage != b.damage ) {
        return a.damage > b.damage ? -1 : +1; // Prefer more damaged stuff
    }
    return 0;
}

bool sort_bylikeness(const crafting_inventory_t::candidate_t& a, const crafting_inventory_t::candidate_t& b) {
    if(a.getLocation() != b.getLocation()) {
        return a.getLocation() < b.getLocation();
    }
    return sort_by_likeness(a.get_item(), b.get_item()) < 0;
}

void crafting_inventory_t::complex_req::select_items_to_use() {
    selected_items.clear();
    selected_simple_req_index = -1;
    toolfactor = 0.0;
    assert(!simple_reqs.empty());
    // List for the menu_vec below, contains the menu entries
    // that should be displayed. This is used to display possible
    // choises to the user.
    std::vector<std::string> options;
    // the menu_entry_type defines what we do if the user selects that menu entry.
    // the second value is just the index into the available_items array that belongs
    // to this option.
    std::vector< std::pair<menu_entry_type, size_t> > optionsIndizes;
    
    bool needNonWater = false;
    std::pair<int, const candidate_t*> indexOfWater(-1, 0);
    std::pair<int, const candidate_t*> indexOfCleanWater(-1, 0);
    std::pair<std::pair<int, float>, const candidate_t*> bestTool(std::make_pair(-1, 0.0f), 0);
    for(size_t i = 0; i < simple_reqs.size(); i++) {
        const simple_req &sr = simple_reqs[i];
        if(sr.available != a_true) {
            continue;
        }
        std::sort(simple_reqs[i].candidate_items.begin(), simple_reqs[i].candidate_items.end(), &sort_bylikeness);
        const requirement &req = sr.req;
        const candvec &candidate_items = sr.candidate_items;
        const int count = req.count;
        if(sr.cnt_on_map + sr.cnt_on_player < count) {
            continue;
        }
        for(candvec::const_iterator a = candidate_items.begin(); a != candidate_items.end(); ++a) {
            const candidate_t &can = *a;
            if(req(can) < count) {
                continue;
            }
            if(as_tool && req.ctype == C_AMOUNT) {
                const float time_modi = can.get_time_modi();
                if(bestTool.first.first == -1 || time_modi < bestTool.first.second) {
                    // can is better
                    bestTool.first.first = i;
                    bestTool.first.second = time_modi;
                    bestTool.second = &can;
                } else if(time_modi == bestTool.first.second) {
                    // Equally good, and different item types, let the used decide
                    assert(bestTool.second != 0);
                    if(can.get_item().type != bestTool.second->get_item().type) {
                        bestTool.first.first = -2;
                    }
                }
            }
            if(can.usageType == "burnt_out_bionic") {
                selected_simple_req_index = i;
                selected_items.push_back(can);
                toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
                return;
            }
            if(can.usageType != "water_clean" && can.usageType != "water") {
                // We need something else than water,
                // disable auto-choosing water
                needNonWater = true;
            }
            if(can.getLocation() == LT_SURROUNDING) {
                // Prefer surrounding objects (like fire)
                selected_simple_req_index = i;
                selected_items.push_back(can);
                toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
                return;
            }
            if(!needNonWater && can.usageType == "water" && indexOfWater.first == -1) {
                // Found water - any source of water is equally good.
                indexOfWater.first = i;
                indexOfWater.second = &can;
            }
            if(!needNonWater && can.usageType == "water_clean" && (indexOfCleanWater.first == -1 || can.getLocation() == LT_VPART)) {
                // Found water - any source of water is equally good,
                // except water from vehicle is "better"?
                indexOfCleanWater.first = i;
                indexOfCleanWater.second = &can;
            }
            if(can.getLocation() == LT_VPART && can.usageType != "water_clean" ) {
                // Prefer this one as it is linked to the car's battery
                // clean water must be selected manually, is too valueable
                selected_simple_req_index = i;
                selected_items.push_back(can);
                toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
                return;
            }
        }
        
        std::ostringstream buffer;
        buffer << "As " << req << ": ";
        if(candidate_items.size() == 1) {
            // Only one possible choice
            options.push_back(buffer.str() + candidate_items.front().to_string(false));
            optionsIndizes.push_back(std::make_pair(single_choise, i));
        } else if(!all_equal(candidate_items)) {
            // several items with differing properties - list by location,
            options.push_back(buffer.str() + "different items possible");
            optionsIndizes.push_back(std::make_pair(ask_again, i));
        } else {
            // several items, they are all of the same type and same
            // properties, list by location,
            // Rules here are: show entry for "on person",
            // for "nearby" (if any of them apply).
            // Also show the mixed entry, but only if none of the other two
            // are shown
            buffer << item::nname(req.type);
            if(sr.cnt_on_map >= count) {
                options.push_back(buffer.str() + " (nearby)");
                optionsIndizes.push_back(std::make_pair(nearby, i));
            }
            if(sr.cnt_on_player >= count) {
                options.push_back(buffer.str());
                optionsIndizes.push_back(std::make_pair(person, i));
            }
            if(sr.cnt_on_map < count && sr.cnt_on_player < count) {
                assert(sr.cnt_on_map + sr.cnt_on_player >= count);
                options.push_back(buffer.str() + " (on person & nearby)");
                optionsIndizes.push_back(std::make_pair(mixed, i));
            }
        }
    }
    if(!needNonWater && indexOfWater.first >= 0) {
        // Water is plenty and normaly you have no alternative
        // except clean water which is to valueable
        selected_simple_req_index = indexOfWater.first;
        selected_items.push_back(*indexOfWater.second);
        toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
        return;
    }
    if(!needNonWater && indexOfCleanWater.first >= 0) {
        selected_simple_req_index = indexOfCleanWater.first;
        selected_items.push_back(*indexOfCleanWater.second);
        toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
        return;
    }
    if(bestTool.first.first >= 0) {
        selected_simple_req_index = bestTool.first.first;
        selected_items.push_back(*bestTool.second);
        toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
        return;
    }
    
    if(options.size() == 0) {
        debugmsg("Attempted to select_items_to_use with no available simple_reqs!");
        return;
    }
    size_t selection = 0;
    if(options.size() != 1) {
        // no user-interaction needed if only one choise
        std::string msg;
        if(as_tool) {
            msg = _("Use witch item(s) as tool");
        } else {
            msg = _("Use witch item(s) as component");
        }
        selection = (size_t) menu_vec(false, msg.c_str(), options) - 1;
    }
    assert(selection < options.size());
    assert(options.size() == optionsIndizes.size());
    const size_t index_of_component = optionsIndizes[selection].second;
    simple_req &sr = simple_reqs[index_of_component];
    // The user has choosen this selection of items:
    candvec &vec_to_consume_from = sr.candidate_items;
    assert(!vec_to_consume_from.empty());
    const requirement reqToRemove = sr.req;
    switch(optionsIndizes[selection].first) {
        case single_choise:
            // only one item in list, use this anyway, their is no choice
            crafting_inventory_t::reduce(reqToRemove, vec_to_consume_from);
            break;
        case nearby:
            // use only items from map
            crafting_inventory_t::filter(vec_to_consume_from, S_MAP);
            assert(!vec_to_consume_from.empty());
            crafting_inventory_t::reduce(reqToRemove, vec_to_consume_from);
            break;
        case person:
            // use only items from player
            crafting_inventory_t::filter(vec_to_consume_from, S_PLAYER);
            assert(!vec_to_consume_from.empty());
            crafting_inventory_t::reduce(reqToRemove, vec_to_consume_from);
            break;
        case mixed:
            // use items from both sources, no filtering or changes needed
            // This essential means use all items available
            crafting_inventory_t::reduce(reqToRemove, vec_to_consume_from);
            break;
        case ask_again:
            crafting_inventory_t::ask_for_items_to_use(reqToRemove, as_tool ? assume_tools : assume_components, vec_to_consume_from);
            break;
        default:
            assert(false);
    }
    selected_items.swap(vec_to_consume_from);
    selected_simple_req_index = index_of_component;
    toolfactor = crafting_inventory_t::calc_time_modi(selected_items);
}

void crafting_inventory_t::reduce(requirement req, candvec &candidates) {
    assert(!candidates.empty());
    assert(req.count > 0);
    for(size_t i = 0; i < candidates.size(); i++) {
        if(req.count <= 0) {
            candidates.erase(candidates.begin() + i, candidates.end());
            break;
        }
        req.count -= req(candidates[i]);
    }
    assert(!candidates.empty());
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
        for(candvec::const_iterator a = selected_items.begin(); a != selected_items.end(); ++a) {
            item tmpit(a->get_item());
            if(req.ctype == C_CHARGES) {
                tmpit.charges = std::min(tmpit.charges, req.count);
            } else {
                tmpit.charges = 0;
            }
            used_items.push_back(tmpit);
        }
        // Basicly the requirement says: "need tool X, no charges required"
        // and the tool is not used up, so we can skip this here.
        return;
    }
    for(candvec::const_iterator a = selected_items.begin(); req.count > 0 && a != selected_items.end(); ++a) {
        // use up components and change the requirement according
        a->consume(p, req, used_items);
    }
    if(used_items.size() >= 1) {
        p->lastconsumed = used_items.front().type->id;
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
        std::ostringstream buffer;
        buffer << "Select the item to use as " << req;
        selection = menu_vec(false, buffer.str().c_str(), options) - 1;
    }
    assert((size_t) selection < indizes.size());
    return candidates[indizes[selection]];
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, consume_flags flags, candvec &candidates) {
    candvec tmpVec;
    ask_for_items_to_use(req, flags, candidates, tmpVec);
    tmpVec.swap(candidates);
}

void crafting_inventory_t::ask_for_items_to_use(const requirement &req, consume_flags flags, const candvec &candidates, candvec &selected_candidates) {
    assert(candidates.size() > 0);
    int minCount = req.count; // Sensible, dont' care about mincounts greater than this anyway
    int sum_count = 0;
    for(candvec::const_iterator a = candidates.begin(); a != candidates.end(); ++a) {
        const int cnt = req(*a);
        minCount = std::min(minCount, cnt);
        sum_count += cnt;
    }
    if(sum_count == req.count) {
        selected_candidates = candidates;
        return;
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
    
    std::ostringstream buffer;
    buffer << "Select the items to use as " << req;
    uimenu menu;
    menu.text = buffer.str();
    // sort to make it nicer looking
    // sorts by location first
    std::sort(const_cast<candvec&>(candidates).begin(), const_cast<candvec&>(candidates).end(), sort_bylikeness);
    // stores which items the user has selected
    std::vector<bool> selected(candidates.size(), false);
    // the (summed up) charges (as told by requirement::get_charges_or_amount)
    // of all selected items. If this reaches the required count, we can break out
    // of this loop.
    int cntFromSelectedOnes = 0;
    for( size_t i = 0; i < candidates.size() && cntFromSelectedOnes < req.count; i++ ) {
        selected[i] = true;
        cntFromSelectedOnes += req(candidates[i]);
    }
    for( size_t i = 0; i < candidates.size(); i++ ) {
        const std::string prefix( selected[i] ? "    using " : "not using " );
        menu.addentry( prefix + candidates[i].to_string( flags != assume_components ) );
    }
    menu.addentry( candidates.size(), cntFromSelectedOnes >= req.count, -1, "OK" );
    while(true) {
        // FIXME condens identical lines (e.g. 10 x "2x4 (on person)" -> "10 x 2x4 (on person)")
        menu.query();
        const size_t selection = menu.ret;
        if( selection >= candidates.size() && cntFromSelectedOnes >= req.count ) {
            for(size_t i = 0; i < candidates.size(); i++) {
                if(selected[i]) {
                    selected_candidates.push_back(candidates[i]);
                }
            }
            return;
        } else if( selection < candidates.size() ) {
            auto &c = candidates[selection];
            selected[selection] = !selected[selection];
            if( selected[selection] ) {
                cntFromSelectedOnes += req( c );
            } else {
                cntFromSelectedOnes -= req( c );
            }
            const std::string prefix( selected[selection] ? "    using " : "not using " );
            menu.entries[selection].txt = prefix + c.to_string( flags != assume_components );
            menu.entries.back().enabled = cntFromSelectedOnes >= req.count;
        }
    }
}

#define CMP_IF(_w) \
    do { if(_w != other._w) { return _w < other._w; } } while(false)
bool crafting_inventory_t::candidate_t::operator<(const candidate_t &other) const {
    CMP_IF(location);
    switch(location) {
        case LT_INVENTORY:
            CMP_IF(invpos);
            break;
        case LT_MAP:
            CMP_IF(mapitems->position.x);
            CMP_IF(mapitems->position.y);
            CMP_IF(mindex);
            break;
        case LT_VEHICLE_CARGO:
            CMP_IF(vitems->veh);
            CMP_IF(vitems->mount_dx);
            CMP_IF(vitems->mount_dy);
            CMP_IF(iindex);
            break;
        case LT_VPART:
            CMP_IF(vpartitem->veh);
            CMP_IF(vitems->mount_dx);
            CMP_IF(vitems->mount_dy);
            break;
        case LT_BIONIC:
            CMP_IF(bionic->bio_id);
            break;
        case LT_SURROUNDING:
            CMP_IF(mapitems->position.x);
            CMP_IF(mapitems->position.y);
            CMP_IF(surroundings->the_item.type->nname(1));
            break;
    }
    if(get_item().type != other.get_item().type) {
        return get_item().type->nname(1) < other.get_item().type->nname(1);
    }
    return ::memcmp(this, &other, sizeof(*this));
}
#undef CMP_IF

std::string crafting_inventory_t::candidate_t::to_string(bool withTime) const {
    std::ostringstream buffer;
    if(withTime) {
        const float timeModi = get_time_modi();
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
    const item &it = get_item();
    buffer << const_cast<item&>(it).tname();
    if(it.charges > 0) {
        buffer << " (" << it.charges << ")";
    }
    
    switch(location) {
        case LT_VEHICLE_CARGO:
        case LT_VPART:
            buffer << " (in car)";
            break;
        case LT_MAP:
        case LT_SURROUNDING:
            buffer << " (nearby)";
            break;
        case LT_INVENTORY:
            if(invcount > 1) {
                buffer << " (" << invcount << " items)";
            }
            break;
        case LT_BIONIC:
            buffer << " (bionic)";
            break;
        default:
            assert(false);
    }
    return buffer.str();
}

static void drainVehicle(vehicle *veh, const item &templ, const std::string &ftype, long &amount, std::list<item> &used_items) {
    item tmp(templ);
    tmp.charges = veh->drain(ftype, amount);
    amount -= tmp.charges;
    used_items.push_back(tmp);
}

long crafting_inventory_t::requirement::get_charges_or_amount(const item &the_item) const {
    long result = 0;
    for(size_t k = 0; k < the_item.contents.size(); k++) {
        result += get_charges_or_amount(the_item.contents[k]);
    }
    if(!the_item.contents.empty()) {
        // Non-empty container, never gets used
        return result;
    }
    if(!the_item.matches_type(type)) {
        // Wrong item type
        return result;
    }
    if(ctype == C_AMOUNT) {
        return result + 1; // +1 for the it itself
    }
    if(type.compare(0, 5, "func:") != 0 || the_item.type == NULL) {
        return result + the_item.charges;
    }
    const float modi = the_item.type->getChargesModi(type);
    if(modi == -1.0f) {
        // No charges required for this tool
        return count;
    } else if(modi == 0.0f) {
        // Does not apply for use as charged object
        return 0;
    }
    assert(modi > 0.0f);
    return result + static_cast<long>(the_item.charges / modi);
}

long crafting_inventory_t::requirement::operator()(const candidate_t &candidate) const {
    long cnt = (*this)(candidate.get_item());
    if(candidate.getLocation() == LT_INVENTORY) {
        cnt *= candidate.invcount;
    }
    return cnt;
}

bool crafting_inventory_t::requirement::use(item &the_item, std::list<item> &used_items) {
    if(count == 0) {
        return false;
    }
    assert(!the_item.is_null());
    for(size_t k = 0; k < the_item.contents.size() && count > 0; k++) {
        if(use(the_item.contents[k], used_items)) {
            the_item.contents.erase(the_item.contents.begin() + k);
            k--;
        }
    }
    assert(count >= 0);
    if(count == 0) {
        return false;
    }
    if(!the_item.contents.empty()) {
        // Non-empty container, never gets used
        return false;
    }
    if(!the_item.matches_type(type)) {
        // Wrong item type
        return false;
    }
    if(ctype == C_AMOUNT) {
        // Use up the item itself
        used_items.push_back(the_item);
        count--;
        // Let the caller destroy the item
        return true;
    }
    item tmp = the_item;
    if(type.compare(0, 5, "func:") == 0) {
        const float modi = the_item.type->getChargesModi(type);
        if(modi == -1.0f) {
            // No charges required for this tool
            tmp.charges = 0;
            count = 0;
        } else if(modi == 0.0f) {
            // Does not apply for use as charged object
            return false;
        }
        const long charges_norm = static_cast<long>(the_item.charges / modi);
        const long used_charges = std::min(count, charges_norm);
        tmp.charges = static_cast<long>(used_charges * modi);
        count -= used_charges;
    } else {
        const long used_charges = std::min(count, the_item.charges);
        tmp.charges = used_charges;
        count -= used_charges;
    }
    the_item.charges -= tmp.charges;
    assert(tmp.charges >= 0);
    assert(the_item.charges >= 0);
    used_items.push_back(tmp);
    // Tell the caller if to destroy the item
    return (the_item.charges == 0 && the_item.destroyed_at_zero_charges());
}

using isptr = std::unique_ptr<item_stack>;
using ipair = std::pair<isptr, item>;
using ipvec = std::vector<ipair>;
static ipvec item_vector_resort_set;
typedef std::vector< std::pair<int, item> > invpos_vector_t;
typedef std::map<player*,invpos_vector_t> invpos_map_t;
static invpos_map_t item_inpos_remove_map;

template<typename T>
void remove_releated(crafting_inventory_t::requirement &req, const T &v_, int index, std::list<item> &used_items) {
    std::unique_ptr< T > ptr( new T( v_ ) );
    T &v = *ptr;
    assert((size_t) index < v.size());
    if(req.use(v[index], used_items)) {
        const item olditem = v[index];
        v[index] = item();
        item_vector_resort_set.push_back( ipair( std::move( ptr ), olditem ) );
    }
}

void remove_releated(crafting_inventory_t::requirement &req, player* p, int invpos, std::list<item> &used_items) {
    item tmpit = p->i_at(invpos); // note: this is a copy
    if(req.use(tmpit, used_items)) {
        item_inpos_remove_map[p].push_back(std::make_pair(invpos, item()));
    } else {
        item_inpos_remove_map[p].push_back(std::make_pair(invpos, tmpit));
    }
}

bool invpos_item_pair_comparator(const std::pair<int, item>& a, const std::pair<int, item>& b) {
    if(a.first >= 0 && b.first >= 0) {
        return a.first > b.first;
    } else if(a.first < 0 && b.first < 0) {
        return a.first < b.first;
    } else {
        return (a.first >= 0) ? true : false;
    }
}

template<typename T>
void erase_null( T &v, const item &olditem ) {
    for( auto it = v.begin(); it != v.end(); ++it ) {
        if( it->is_null()) {
            *it = olditem;
            v.erase( it );
            return;
        }
    }
}

void resort_item_vectors() {
    for(invpos_map_t::iterator a = item_inpos_remove_map.begin(); a != item_inpos_remove_map.end(); ++a) {
        player* p = a->first;
        invpos_vector_t& vec = a->second;
        std::sort(vec.begin(), vec.end(), invpos_item_pair_comparator);
        for(invpos_vector_t::iterator b = vec.begin(); b != vec.end(); ++b) {
            if(b->second.is_null()) {
                p->i_rem(b->first);
            } else {
                p->i_at(b->first) = b->second;
            }
        }
    }
    item_inpos_remove_map.clear();
    for( auto & p : item_vector_resort_set ) {
        const auto pp = p.first.get();
        map_stack *ms = dynamic_cast<map_stack*>( pp );
        if( ms != nullptr ) {
            erase_null( *ms, p.second );
        }
        vehicle_stack *vs = dynamic_cast<vehicle_stack*>( pp );
        if( vs != nullptr ) {
            erase_null( *vs, p.second );
        }
    }
    item_vector_resort_set.clear();
}

void crafting_inventory_t::candidate_t::consume(player *p, requirement &req, std::list<item> &used_items) const {
    if(req.count == 0) {
        return;
    }
    assert(valid());
    const item *ix;
    switch(location) {
        case LT_INVENTORY:
            for(int i = 0; req.count > 0 && i < invcount; i++) {
                remove_releated(
                    req,
                    p,
                    invpos,
                    used_items
                );
            }
            return;
        case LT_VEHICLE_CARGO:
            remove_releated(
                req,
                vitems->items(),
                iindex,
                used_items
            );
            return;
        case LT_MAP:
            remove_releated(
                req,
                g->m.i_at(mapitems->position.x, mapitems->position.y),
                mindex,
                used_items
            );
            return;
        // Below are pseudo item. They should not be used in requirement_datas
        // as they can not be removed (used up).
        case LT_VPART:
            ix = &(get_item());
            if(req.ctype == C_AMOUNT) {
                debugmsg("attempted to consume a pseudo vehicle part item %s for %s", ix->tname().c_str(), req.type.c_str());
                return;
            }
            if(ix->type->id == "vpart_KITCHEN") {
                drainVehicle(vpartitem->veh, *ix, "battery", req.count, used_items);
            } else if(ix->type->id == "water_clean") {
                drainVehicle(vpartitem->veh, *ix, "water", req.count, used_items);
            } else if(ix->type->id == "vpart_WELDRIG") {
                drainVehicle(vpartitem->veh, *ix, "battery", req.count, used_items);
            } else if(ix->type->id == "vpart_CHEMLAB") {
                drainVehicle(vpartitem->veh, *ix, "battery", req.count, used_items);
            } else if(ix->type->id == "vpart_CRAFTRIG") {
                drainVehicle(vpartitem->veh, *ix, "battery", req.count, used_items);
            } else if(ix->type->id == "vpart_FORGE") {
                drainVehicle(vpartitem->veh, *ix, "battery", req.count, used_items);
            } else {
                debugmsg("Unknown pseudo vehicle part item %s for %s", ix->tname().c_str(), req.type.c_str());
            }
            return;
        case LT_SURROUNDING:
            ix = &(get_item());
            if(req.ctype == C_AMOUNT) {
                debugmsg("attempted to consume a pseudo surrounding item %s for %s", ix->tname().c_str(), req.type.c_str());
                return;
            }
            if(ix->type->id == "water" && furnlist[g->m.furn(mapitems->position.x, mapitems->position.y)].examine == &iexamine::toilet) {
                // get water charges at location
                auto toiletitems = g->m.i_at(mapitems->position.x, mapitems->position.y);
                for(size_t i = 0; req.count > 0 && i < toiletitems.size(); ++i) {
                    if(toiletitems[i].typeId() == "water") {
                        remove_releated(
                            req,
                            toiletitems,
                            i,
                            used_items
                        );
                    }
                }
                if(req.count <= 0) {
                    break;
                }
            }
            used_items.push_back(*ix);
            used_items.back().charges = req.count;
            // Basicly this is an inifinte amount of things
            // like fire, or a water source, in this case we can ignore it.
            req.count = 0;
            return;
        case LT_BIONIC:
            ix = &(get_item());
            if(req.ctype == C_AMOUNT) {
                debugmsg("attempted to consume a pseudo bionc item %s for %s", ix->tname().c_str(), req.type.c_str());
                return;
            }
            used_items.push_back(*ix);
            used_items.back().charges = std::min(static_cast<long>(p->power_level), req.count);
            if(req.count >= p->power_level) {
                req.count -= p->power_level;
                p->power_level = 0;
            } else {
                p->power_level -= req.count;
                req.count = 0;
            }
            return;
        default:
            debugmsg("dont know what to consume!");
            break;
    }
}

int compare(const item &a, const item &b) {
    if(a.is_container() && !a.contents.empty()) {
        const int c = compare(a.contents[0], b);
        if(c != 0) {
            return c;
        }
    } else if(b.is_container() && !b.contents.empty()) {
        const int c = compare(a, b.contents[0]);
        if(c != 0) {
            return c;
        }
    }
        
    if(a.type != b.type) {
        return a.type < b.type ? -1 : +1;
    }
    if(a.damage != b.damage) {
        return a.damage < b.damage ? -1 : +1;
    }
    if(a.burnt != b.burnt) {
        return a.burnt < b.burnt ? -1 : +1;
    }
    if(a.bigness != b.bigness) {
        return a.bigness < b.bigness ? -1 : +1;
    }
    // FIXME: are there more properties that need to be compared?
    return 0;
}

bool crafting_inventory_t::all_equal(const candvec &candidates) {
    for(size_t i = 0; i + 1 < candidates.size(); i++) {
        const candidate_t &prev = candidates[i];
        const candidate_t &cur = candidates[i + 1];
        const item &prev_item = prev.get_item();
        const item &cur_item = cur.get_item();
        if(compare(prev_item, cur_item) != 0) {
            return false;
        }
    }
    return true;
}


inline bool match_and_count(const item &it, const crafting_inventory_t::requirement &req, int factor, int &countSum) {
    const int c = req(it);
    if(c > 0) {
        countSum += c * factor;
        return true;
    }
    return false;
}

#define XMATCH(item_, factor) ::match_and_count(item_, req, factor, count)
int crafting_inventory_t::collect_candidates(const requirement &req, int sources, candvec &candidates) {
    int count = 0;
    if(sources & LT_MAP) {
        for(std::list<items_on_map>::iterator a = on_map.begin(); a != on_map.end(); ++a) {
            const auto items = a->items();
            int index = 0;
            for( const auto & b : items ) {
                if(XMATCH(b, 1)) {
                    candidates.push_back(candidate_t(*a, index, req.type));
                }
                index++;
            }
        }
    }
    if(sources & LT_VEHICLE_CARGO) {
        for(std::list<items_in_vehicle_cargo>::iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
            const auto items = a->items();
            int index = 0;
            for( const auto & b : items ) {
                if(XMATCH(b, 1)) {
                    candidates.push_back(candidate_t(*a, index, req.type));
                }
                index++;
            }
        }
    }
    if(sources & LT_VPART) {
        for(std::list<item_from_vpart>::iterator a = vpart.begin(); a != vpart.end(); ++a) {
            if(XMATCH(a->the_item, 1)) {
                candidates.push_back(candidate_t(*a, req.type));
            }
        }
    }
    if(sources & LT_INVENTORY) {
        int i = 0;
        for(invstack::const_iterator iter = p->inv.getItems().begin(); iter != p->inv.getItems().end();
            ++iter, i++) {
            const item &it = iter->front();
            if(XMATCH(it, iter->size())) {
                candidates.push_back(candidate_t(p, i, iter->size(), req.type));
            }
        }
        int w = 0;
        for(std::vector<item>::const_iterator iter = p->worn.begin(); iter != p->worn.end();
            ++iter, w++) {
            if(XMATCH(*iter, 1)) {
                candidates.push_back(candidate_t(p, player::worn_position_to_index(w), 1, req.type));
            }
        }
        if(!p->weapon.is_null()) {
            if(XMATCH(p->weapon, 1)) {
                candidates.push_back(candidate_t(p, -1, 1, req.type));
            }
        }
    }
    if(sources & LT_BIONIC) {
        for(std::list<item_from_bionic>::iterator a = by_bionic.begin(); a != by_bionic.end(); ++a) {
            if(XMATCH(a->the_item, 1)) {
                candidates.push_back(candidate_t(*a, req.type));
            }
        }
    }
    if(sources & LT_SURROUNDING) {
        for(std::list<item_from_surrounding>::iterator a = surround.begin(); a != surround.end(); ++a) {
            if(XMATCH(a->the_item, 1)) {
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
            itype *it = item::find_type(type);
            if(type.compare(0, 5, "func:") == 0) {
                it = item::find_type(type.substr(5));
            }
            it_tool *itt = dynamic_cast<it_tool*>(it);
            if(itt != nullptr && itt->max_charges > 0) {
                this->ctype = C_CHARGES;
            } else {
                this->ctype = C_AMOUNT;
            }
        } else {
            this->count = std::max(1, abs(req));
            this->ctype = C_AMOUNT;
        }
    } else {
        if(req > 0 && item::count_by_charges(type)) {
            this->count = req;
            this->ctype = C_CHARGES;
        } else {
            this->count = req < 0 ? -req : req;
            this->ctype = C_AMOUNT;
        }
    }
}

bool crafting_inventory_t::has_any_components(const std::vector<item_comp> &comps) const {
    if(comps.empty()) { return true; }
    solution s(comps, const_cast<crafting_inventory_t&>(*this));
    return s.is_possible();
}

bool crafting_inventory_t::has_components(const itype_id &type, int count) const {
    item_comp comp(type, count);
    return has_components(comp);
}

bool crafting_inventory_t::has_components(const item_comp &comp) const {
    std::vector<item_comp> tmpcomps(1, comp);
    return has_any_components(tmpcomps);
}



bool crafting_inventory_t::has_any_tools(const std::vector<tool_comp> &tools) const {
    if(tools.empty()) { return true; }
    solution s(tools, const_cast<crafting_inventory_t&>(*this));
    return s.is_possible();
}

bool crafting_inventory_t::has_tools(const itype_id &type, int count) const {
    tool_comp comp(type, count);
    return has_tools(comp);
}

bool crafting_inventory_t::has_tools(const itype_id &type) const {
    tool_comp comp(type, -1);
    return has_tools(comp);
}

bool crafting_inventory_t::has_tools(const tool_comp &tools) const {
    std::vector<tool_comp> tmptools(1, tools);
    return has_any_tools(tmptools);
}

std::list<item> crafting_inventory_t::consume_any_tools(const std::vector<tool_comp> &tools)
{
    std::list<item> result;
    if(tools.empty()) { return result; }
    solution s;
    s.init_need_any(tools);
    s.gather(const_cast<crafting_inventory_t&>(*this), true);
    if(s.is_possible()) {
        s.select_items_to_use();
        s.consume(*this, result);
    } else {
        debugmsg("Attempted to consume_any_tools with no possible tools");
    }
    return result;
}

std::list<item> crafting_inventory_t::consume_tools(const tool_comp &tools)
{
    std::vector<tool_comp> tmptools(1, tools);
    return consume_any_tools(tmptools);
}

std::list<item> crafting_inventory_t::consume_tools(const itype_id &type, int count) {
    tool_comp comp(type, count);
    return consume_tools(comp);
}

std::list<item> crafting_inventory_t::consume_tools(const itype_id &type) {
    tool_comp comp(type, -1);
    return consume_tools(comp);
}



std::list<item> crafting_inventory_t::consume_any_components(const std::vector<item_comp> &comps)
{
    std::list<item> result;
    if(comps.empty()) { return result; }
    if(comps[0].count == 0) { return result; }
    solution s;
    s.init_need_any(comps);
    s.gather(const_cast<crafting_inventory_t&>(*this), true);
    if(s.is_possible()) {
        s.select_items_to_use();
        s.consume(*this, result);
    } else {
        debugmsg("Attempted to consume_any_components with no possible components");
    }
    return result;
}

std::list<item> crafting_inventory_t::consume_components(const item_comp &comps)
{
    std::vector<item_comp> tmpcomps(1, comps);
    return consume_any_components(tmpcomps);
}

std::list<item> crafting_inventory_t::consume_components(const itype_id &type, int count) {
    item_comp comp(type, count);
    return consume_components(comp);
}

void crafting_inventory_t::consume_items(const std::vector<item_comp> &comps) {
    requirement_data r;
    for(size_t i = 0; i < comps.size(); i++) {
        if(comps[i].count == 0) { continue; }
        r.components.resize(r.components.size() + 1);
        r.components.back().push_back(comps[i]);
    }
    if(r.components.empty()) { return; }
    solution s;
    s.init(r);
    s.gather(*this, true);
    s.select_items_to_use();
    std::list<item> result;
    s.consume(*this, result);
}


bool hasQuality(const item &the_item, const std::string &name, int level)
{
    const std::map<std::string, int> &qualities = the_item.type->qualities;
    const std::map<std::string, int>::const_iterator quality_iter = qualities.find(name);
    return (quality_iter != qualities.end() && level <= quality_iter->second);
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
        const auto items = a->items();
        for( const auto & b : items ) {
            if(b.made_of(LIQUID)) {
                continue;
            }
            if(::hasQuality(b, name, level)) {
                amount--;
                if(amount <= 0) {
                    return true;
                }
            }
        }
    }
    for(std::list<items_in_vehicle_cargo>::const_iterator a = in_veh.begin(); a != in_veh.end(); ++a) {
        const auto items = a->items();
        for( const auto & b : items ) {
            if(::hasQuality(b, name, level)) {
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

void crafting_inventory_t::add_vpart(vehicle *veh, int mpart, const std::string &vpart_flag_name, const ammotype &fuel) {
    static const std::string vpart_name_prefix("vpart_");
    const int part = veh->part_with_feature(mpart, vpart_flag_name);
    if (part < 0) {
        // No such part here!
        return;
    }
    const itype_id type = vpart_name_prefix + vpart_flag_name;
    if(!item::type_is_defined(type)) {
        debugmsg("Missing template for vpart pseudo item %s", type.c_str());
        return;
    }
    item vpart_item(type, calendar::turn);
    if(fuel.empty() || fuel == "null" || fuel == "NULL") {
        vpart_item.charges = -1;
    } else {
        vpart_item.charges = veh->fuel_left(fuel);
    }
    vpart.push_back(item_from_vpart(veh, veh->parts[part].mount.x, veh->parts[part].mount.y, vpart_item));
}

void crafting_inventory_t::add_surround(const tripoint &p, const item &it) {
    surround.push_back(item_from_surrounding(p, it));
}
