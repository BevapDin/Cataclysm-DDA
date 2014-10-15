#include "iuse_actor.h"
#include "item.h"
#include "game.h"
#include "monster.h"
#include <sstream>
#include <algorithm>

iuse_transform::~iuse_transform()
{
}

iuse_actor *iuse_transform::clone() const
{
    return new iuse_transform(*this);
}

long iuse_transform::use(player *p, item *it, bool /*t*/) const
{
    if (need_fire > 0 && p != NULL && p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater"));
        return 0;
    }
    if (need_charges > 0 && it->charges < need_charges) {
        if (!need_charges_msg.empty()) {
            p->add_msg_if_player(m_info, need_charges_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    if (p != NULL && need_fire > 0 && !p->use_charges_if_avail("fire", need_fire)) {
        if (!need_fire_msg.empty()) {
            p->add_msg_if_player(m_info, need_fire_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    // load this from the original item, not the transformed one.
    const int charges_to_use = it->type->charges_to_use();
    p->add_msg_if_player(m_neutral, msg_transform.c_str(), it->tname().c_str());
    item *target;
    if (container_id.empty()) {
        // No container, assume simple type transformation like foo_off -> foo_on
        it->make(target_id);
        target = it;
    } else {
        // Transform into something in a container, assume the content is
        // "created" right now and give the content the current time as birthday
        it->make(container_id);
        it->contents.push_back(item(target_id, calendar::turn));
        target = &it->contents.back();
    }
    target->active = active;
    if (target_charges > -2) {
        // -1 is for items that can not have any charges at all.
        target->charges = target_charges;
    }
    p->moves -= moves;
    return charges_to_use;
}



auto_iuse_transform::~auto_iuse_transform()
{
}

iuse_actor *auto_iuse_transform::clone() const
{
    return new auto_iuse_transform(*this);
}

long auto_iuse_transform::use(player *p, item *it, bool t) const
{
    if (t) {
        if (!when_underwater.empty() && p != NULL && p->is_underwater()) {
            // Dirty hack to display the "when unterwater" message instead of the normal message
            std::swap(const_cast<auto_iuse_transform *>(this)->when_underwater,
                      const_cast<auto_iuse_transform *>(this)->msg_transform);
            const long tmp = iuse_transform::use(p, it, t);
            std::swap(const_cast<auto_iuse_transform *>(this)->when_underwater,
                      const_cast<auto_iuse_transform *>(this)->msg_transform);
            return tmp;
        }
        // Normal use, don't need to do anything here.
        return 0;
    }
    if (it->charges > 0 && !non_interactive_msg.empty()) {
        p->add_msg_if_player(m_info, non_interactive_msg.c_str(), it->tname().c_str());
        // Activated by the player, but not allowed to do so
        return 0;
    }
    return iuse_transform::use(p, it, t);
}



explosion_iuse::~explosion_iuse()
{
}

iuse_actor *explosion_iuse::clone() const
{
    return new explosion_iuse(*this);
}

// defined in iuse.cpp
extern std::vector<point> points_for_gas_cloud(const point &center, int radius);

long explosion_iuse::use(player *p, item *it, bool t) const
{
    // This ise used for active items, their charges are autmatically
    // decremented, therfor this function always returns 0.
    const point pos = g->find_item(it);
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) {
        if (sound_volume >= 0) {
            g->sound(pos.x, pos.y, sound_volume, sound_msg);
        }
        return 0;
    }
    if (it->charges > 0) {
        if (no_deactivate_msg.empty()) {
            p->add_msg_if_player(m_warning,
                                 _("You've already set the %s's timer you might want to get away from it."), it->tname().c_str());
        } else {
            p->add_msg_if_player(m_info, no_deactivate_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    if (explosion_power >= 0) {
        g->explosion(pos.x, pos.y, explosion_power, explosion_shrapnel, explosion_fire, explosion_blast);
    }
    if (draw_explosion_radius >= 0) {
        g->draw_explosion(pos.x, pos.y, draw_explosion_radius, draw_explosion_color);
    }
    if (do_flashbang) {
        g->flashbang(pos.x, pos.y, flashbang_player_immune);
    }
    if (fields_radius >= 0 && fields_type != fd_null) {
        std::vector<point> gas_sources = points_for_gas_cloud(pos, fields_radius);
        for(size_t i = 0; i < gas_sources.size(); i++) {
            const point &p = gas_sources[i];
            const int dens = rng(fields_min_density, fields_max_density);
            g->m.add_field(p.x, p.y, fields_type, dens);
        }
    }
    if (scrambler_blast_radius >= 0) {
        for (int x = pos.x - scrambler_blast_radius; x <= pos.x + scrambler_blast_radius; x++) {
            for (int y = pos.y - scrambler_blast_radius; y <= pos.y + scrambler_blast_radius; y++) {
                g->scrambler_blast(x, y);
            }
        }
    }
    if (emp_blast_radius >= 0) {
        for (int x = pos.x - emp_blast_radius; x <= pos.x + emp_blast_radius; x++) {
            for (int y = pos.y - emp_blast_radius; y <= pos.y + emp_blast_radius; y++) {
                g->emp_blast(x, y);
            }
        }
    }
    return 0;
}



unfold_vehicle_iuse::~unfold_vehicle_iuse()
{
}

iuse_actor *unfold_vehicle_iuse::clone() const
{
    return new unfold_vehicle_iuse(*this);
}

long unfold_vehicle_iuse::use(player *p, item *it, bool /*t*/) const
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }

    for (auto tool = tools_needed.cbegin(); tool != tools_needed.cend(); ++tool) {
        // Amount == -1 means need one, but don't consume it.
        if (!p->has_amount(tool->first, 1)) {
            p->add_msg_if_player(_("You need %s to do it!"),
                                 item(tool->first, 0).type->nname(1).c_str());
            return 0;
        }
    }

    vehicle *veh = g->m.add_vehicle(vehicle_name, p->posx, p->posy, 0, 0, 0, false);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no room to unfold the %s."), it->tname().c_str());
        return 0;
    }

    // Mark the vehicle as foldable.
    veh->tags.insert("convertible");
    // Store the id of the item the vehicle is made of.
    veh->tags.insert(std::string("convertible:") + it->type->id);
    p->add_msg_if_player(unfold_msg.c_str(), it->tname().c_str());
    p->moves -= moves;
    // Restore HP of parts if we stashed them previously.
    if (it->item_vars.count("folding_bicycle_parts") == 0) {
        // Brand new, no HP stored
        return 1;
    }
    std::istringstream veh_data;
    const std::string &data = it->item_vars["folding_bicycle_parts"];
    veh_data.str(data);
    if (!data.empty() && data[0] >= '0' && data[0] <= '9') {
        // starts with a digit -> old format
        for (size_t p = 0; p < veh->parts.size(); p++) {
            veh_data >> veh->parts[p].hp;
        }
    } else {
        try {
            JsonIn json(veh_data);
            // Load parts into a temporary vector to not override
            // cached values (like precalc_dx, passenger_id, ...)
            std::vector<vehicle_part> parts;
            json.read(parts);
            for(size_t i = 0; i < parts.size() && i < veh->parts.size(); i++) {
                const vehicle_part &src = parts[i];
                vehicle_part &dst = veh->parts[i];
                // and now only copy values, that are
                // expected to be consistent.
                dst.hp = src.hp;
                dst.blood = src.blood;
                dst.bigness = src.bigness;
                // door state/amount of fuel/direction of headlight
                dst.amount = src.amount;
                dst.flags = src.flags;
            }
        } catch(std::string e) {
            debugmsg("Error restoring vehicle: %s", e.c_str());
        }
    }
    return 1;
}

consume_drug_iuse::~consume_drug_iuse() {};

iuse_actor *consume_drug_iuse::clone() const
{
    return new consume_drug_iuse(*this);
}

long consume_drug_iuse::use(player *p, item *it, bool) const
{
    // Check prerequisites first.
    for( auto tool = tools_needed.cbegin(); tool != tools_needed.cend(); ++tool ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_amount( tool->first, 1 ) ) {
            p->add_msg_if_player( _("You need %s to consume %s!"),
                                  item(tool->first, 0).type->nname(1).c_str(),
                                  it->type->nname(1).c_str() );
            return -1;
        }
    }
    for( auto consumable = charges_needed.cbegin(); consumable != charges_needed.cend();
         ++consumable ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_charges( consumable->first, (consumable->second == -1) ?
                             1 : consumable->second ) ) {
            p->add_msg_if_player( _("You need %s to consume %s!"),
                                  item(consumable->first, 0).type->nname(1).c_str(),
                                  it->type->nname(1).c_str() );
            return -1;
        }
    }
    // Apply the various effects.
    for( auto disease = diseases.cbegin(); disease != diseases.cend(); ++disease ) {
        int duration = disease->second;
        if (p->has_trait("TOLERANCE")) {
            duration -= 10; // Symmetry would cause negative duration.
        } else if (p->has_trait("LIGHTWEIGHT")) {
            duration += 20;
        }
        p->add_disease( disease->first, duration );
    }
    for( auto stat = stat_adjustments.cbegin(); stat != stat_adjustments.cend(); ++stat ) {
        p->mod_stat( stat->first, stat->second );
    }
    for( auto field = fields_produced.cbegin(); field != fields_produced.cend(); ++field ) {
        const field_id fid = field_from_ident( field->first );
        for(int i = 0; i < 3; i++) {
            g->m.add_field(p->posx + int(rng(-2, 2)), p->posy + int(rng(-2, 2)), fid, field->second);
        }
    }
    // Output message.
    p->add_msg_if_player( _(activation_message.c_str()) );
    // Consume charges.
    for( auto consumable = charges_needed.cbegin(); consumable != charges_needed.cend();
         ++consumable ) {
        if( consumable->second != -1 ) {
            p->use_charges( consumable->first, consumable->second );
        }
    }
    return it->type->charges_to_use();
}

place_monster_iuse::~place_monster_iuse() {};

iuse_actor *place_monster_iuse::clone() const
{
    return new place_monster_iuse(*this);
}

long place_monster_iuse::use( player *p, item *it, bool ) const
{
    monster newmon( GetMType( mtype_id ) );
    point target;
    if( place_randomly ) {
        std::vector<point> valid;
        for( int x = p->posx - 1; x <= p->posx + 1; x++ ) {
            for( int y = p->posy - 1; y <= p->posy + 1; y++ ) {
                if( g->is_empty( x, y ) ) {
                    valid.push_back( point( x, y ) );
                }
            }
        }
        if( valid.empty() ) { // No valid points!
            p->add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                  newmon.name().c_str() );
            return 0;
        }
        target = valid[rng( 0, valid.size() - 1 )];
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name().c_str() );
        if( !choose_adjacent( query, target.x, target.y ) ) {
            return 0;
        }
        if( !g->is_empty( target.x, target.y ) ) {
            p->add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name().c_str() );
            return 0;
        }
    }
    p->moves -= moves;
    newmon.spawn( target.x, target.y );
    for( auto & amdef : newmon.ammo ) {
        item ammo_item( amdef.first, 0 );
        const int available = p->charges_of( amdef.first );
        if( available == 0 ) {
            amdef.second = 0;
            p->add_msg_if_player( m_info,
                                  _( "If you had standard factory-built %s bullets, you could load the %s." ),
                                  ammo_item.type->nname( 2 ).c_str(), newmon.name().c_str() );
            continue;
        }
        // Don't load more than the default from the the monster definition.
        ammo_item.charges = std::min( available, amdef.second );
        p->use_charges( amdef.first, ammo_item.charges );
        //~ First %s is the ammo item (with plural form and count included), second is the monster name
        p->add_msg_if_player( ngettext( "You load %d x %s round into the %s.",
                                        "You load %d x %s rounds into the %s.", ammo_item.charges ), ammo_item.charges,
                              ammo_item.type->nname( ammo_item.charges ).c_str(), newmon.name().c_str() );
        amdef.second = ammo_item.charges;
    }
    const int damfac = 5 - std::max<int>( 0, it->damage ); // 5 (no damage) ... 1 (max damage)
    // One hp at least, everything else would be unfair (happens only to monster with *very* low hp),
    newmon.hp = std::max( 1, newmon.hp * damfac / 5 );
    if( rng( 0, p->int_cur / 2 ) + p->skillLevel( "electronics" ) / 2 + p->skillLevel( "computer" ) <
        rng( 0, difficulty ) ) {
        if( hostile_msg.empty() ) {
            p->add_msg_if_player( m_bad, _( "The %s scans you and makes angry beeping noises!" ),
                                  newmon.name().c_str() );
        } else {
            p->add_msg_if_player( m_bad, "%s", _( hostile_msg.c_str() ) );
        }
    } else {
        if( friendly_msg.empty() ) {
            p->add_msg_if_player( m_warning, _( "The %s emits an IFF beep as it scans you." ),
                                  newmon.name().c_str() );
        } else {
            p->add_msg_if_player( m_warning, "%s", _( friendly_msg.c_str() ) );
        }
        newmon.friendly = -1;
    }
    // TODO: add a flag instead of monster id or something?
    if( newmon.type->id == "mon_laserturret" && !g->is_in_sunlight( newmon.posx(), newmon.posy() ) ) {
        p->add_msg_if_player( _( "A flashing LED on the laser turret appears to indicate low light." ) );
    }
    g->add_zombie( newmon );
    return 1;
}
