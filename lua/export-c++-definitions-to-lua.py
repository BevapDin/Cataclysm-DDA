#!/usr/bin/python2

from parser import Parser

import re

# All the headers in the src folder that will be searched for the definitions
# of the exported types. The script will complain if a type is requested, but
# not found.
headers = [
    'player.h', 'character.h', 'bodypart.h', 'creature.h', 'game.h',
    'map.h', 'enums.h', 'calendar.h', 'mtype.h', 'itype.h', 'item.h',
    'ui.h', 'martialarts.h', 'trap.h', 'field.h', 'overmap.h',
    'mutation.h', 'effect.h', 'material.h', 'start_location.h', 'ammo.h',
    'monstergenerator.h', 'item_stack.h', 'mongroup.h', 'weather_gen.h',
]

parser = Parser()

# All the classes that should be available in Lua. See doc/LUA_SUPPORT.md for
# the meaning of by_reference/by_value/by_value_and_reference.
parser.add_export_by_reference('effect_type')
parser.add_export_by_value_and_reference('calendar')
parser.add_export_by_reference('mutation_branch')
parser.add_export_by_reference('Character')
parser.add_export_by_value('map_stack')
parser.add_export_by_reference('game')
parser.add_export_by_value('encumbrance_data')
parser.add_export_by_reference('stats')
parser.add_export_by_reference('player')
parser.add_export_by_value_and_reference('item')
parser.add_export_by_value('item_location')
parser.add_export_by_value('point')
parser.add_export_by_value('tripoint')
parser.add_export_by_reference('uimenu')
parser.add_export_by_reference('field_entry')
parser.add_export_by_reference('field')
parser.add_export_by_reference('map')
parser.add_export_by_reference('ter_t')
parser.add_export_by_reference('furn_t')
parser.add_export_by_reference('Creature')
parser.add_export_by_reference('monster')
parser.add_export_by_reference('martialart')
parser.add_export_by_reference('material_type')
parser.add_export_by_reference('start_location')
parser.add_export_by_reference('ma_buff')
parser.add_export_by_reference('ma_technique')
parser.add_export_by_reference('Skill')
parser.add_export_by_reference('quality')
parser.add_export_for_string_id('npc_template_id', 'npc_template')
parser.add_export_for_string_id('species_id', 'species_type')
parser.add_export_by_reference('ammunition_type')
parser.add_export_by_reference('MonsterGroup')
parser.add_export_by_reference('mtype')
parser.add_export_by_reference('mongroup')
parser.add_export_by_reference('overmap')
parser.add_export_by_value('units::volume')
parser.add_export_by_value('units::mass')
parser.add_export_by_value('nc_color')
parser.add_export_by_value('time_duration')
parser.add_export_by_value('time_point')
parser.add_export_by_reference('itype')
parser.add_export_by_reference('trap')
parser.add_export_by_reference('w_point')

# Enums that should be available in Lua.
parser.add_export_enumeration('body_part')
parser.add_export_enumeration('hp_part')
parser.add_export_enumeration('side')
parser.add_export_enumeration('phase_id')
parser.add_export_enumeration('m_size')
parser.add_export_enumeration('game_message_type')
parser.add_export_enumeration('season_type')
parser.add_export_enumeration('add_type')
parser.add_export_enumeration('field_id')

# Data members are checked against `readonly_identifiers` to see whether
# they should be writable from Lua. Const members are always non-writable.

# Not writable to avoid messing things up (can be changed via JSON):
parser.readonly_identifiers.add(re.compile('.*::id$'))
parser.readonly_identifiers.add(re.compile('.*::loadid$'))


# One can block specific members from being exported:
# The given string or regex is matched against the C++ name of the member.
# This includes the parameter list (only type names) for function.

# Exported to Lua via a global variables:
parser.blocked_identifiers.add('game::m')
parser.blocked_identifiers.add('game::u')
# Never exported, internal usage only:
parser.blocked_identifiers.add(re.compile('.*::was_loaded$'))
parser.blocked_identifiers.add(re.compile('.*::active_items$'))
# Stack objects are created by the containing class and not by anyone else.
parser.blocked_identifiers.add(re.compile('map_stack::map_stack\(.*\)'))
# Those are used during loading from JSON and should not be used any other time.
parser.blocked_identifiers.add(re.compile('static .*::load\(.*JsonObject.*\)$'))
parser.blocked_identifiers.add(re.compile('static .*::reset\(\)$'))
parser.blocked_identifiers.add(re.compile('static .*::check_consistency\(\)$'))
parser.blocked_identifiers.add(re.compile('static .*::finalize\(\)$'))


for header in headers:
    parser.parse('src/' + header)

parser.export('lua/generated_class_definitions.lua')
