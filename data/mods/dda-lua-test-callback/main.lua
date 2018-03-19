local MOD = {

  id = "dda-lua-test-callback",
  version = "2018-03-18"

}

mods[MOD.id] = MOD

MOD.MessageWithLog = function( s )

    LOG.message( s )
    if (game.add_msg) then
      game.add_msg( s )
    end

end

MOD.on_game_loaded = function() 

  MOD.DisplayCallbackMessages("on_game_loaded")

end

MOD.on_savegame_loaded = function()

  MOD.DisplayCallbackMessages("on_savegame_loaded")

end

function MOD.on_new_player_created()

  MOD.DisplayCallbackMessages("on_new_player_created")

end

MOD.on_skill_increased = function()

  MOD.DisplayCallbackMessages("on_skill_increased")

end

MOD.on_turn_passed = function()

  MOD.DisplayCallbackMessages("on_turn_passed")

end

MOD.on_second_passed = function()

  MOD.DisplayCallbackMessages("on_second_passed")

end

MOD.on_minute_passed = function()

  MOD.DisplayCallbackMessages("on_minute_passed")

end

MOD.on_hour_passed = function()

  MOD.DisplayCallbackMessages("on_hour_passed")

end

MOD.on_day_passed = function()

  MOD.DisplayCallbackMessages("on_day_passed")

end

MOD.on_year_passed = function()

  MOD.DisplayCallbackMessages("on_year_passed")

end

MOD.on_mutation_gain = function()

  MOD.DisplayCallbackMessages("on_mutation_gain")

end

MOD.on_mutation_loss = function()

  MOD.DisplayCallbackMessages("on_mutation_loss")

end

MOD.on_stat_change = function()

  MOD.DisplayCallbackMessages("on_stat_change")

end

MOD.on_item_wear = function()

  MOD.DisplayCallbackMessages("on_item_wear")

end

MOD.on_item_takeoff = function()

  MOD.DisplayCallbackMessages("on_item_takeoff")

end

MOD.on_effect_int_changes = function()

  MOD.DisplayCallbackMessages("on_effect_int_change")

end

MOD.on_mission_assignment = function()

  MOD.DisplayCallbackMessages("on_mission_assignment")

end

MOD.on_mission_finished = function()

  MOD.DisplayCallbackMessages("on_mission_finished")

end

MOD.on_mapgen_finished = function() 

  MOD.DisplayCallbackMessages("on_mapgen_finished")

end

MOD.on_weather_changed = function()

  MOD.DisplayCallbackMessages("on_weather_changed")

end

MOD.lua_tripoint = function(tp)

  MOD.MessageWithLog("LUA:tripoint is ["..tostring(tp.x)..";"..tostring(tp.y)..";"..tostring(tp.z).."]")

end

MOD.lua_put_on = function(item)

  MOD.MessageWithLog("LUA:You put on your "..tostring(item:display_name()))

end

MOD.lua_put_off = function(item)

  MOD.MessageWithLog("LUA:You take off your "..tostring(item:display_name()))

end

MOD.DisplayCallbackMessages = function( callback_name )

  local callback_args = {
    { "mapgen_generator_type", nil },
    { "mapgen_terrain_type_id", nil },
    { "mapgen_terrain_coordinates", MOD.lua_tripoint },
    { "skill_increased_source", nil },
    { "skill_increased_id", nil },
    { "skill_increased_level", nil },
    { "mutation_gained", nil },
    { "mutation_lost", nil },
    { "stat_changed", nil },
    { "stat_value", nil },
    { "item_last_worn", MOD.lua_put_on },
    { "item_last_taken_off", MOD.lua_put_off },
    { "effect_changed", nil },
    { "effect_intensity", nil },
    { "effect_bodypart", nil },
    { "mission_finished", nil },
    { "mission_assigned", nil },
    { "weather_new", nil },
    { "weather_old", nil }
  }

  MOD.MessageWithLog ("     callback_name: <color_ltcyan>"..tostring(callback_name).."</color>")
  MOD.MessageWithLog ("     callback_last: <color_yellow>"..tostring(callback_last).."</color>")
  MOD.MessageWithLog ("callback_arg_count: <color_red>"..tostring(callback_arg_count).."</color>")
  MOD.MessageWithLog ("callback_arg_count: <color_red>"..tostring(callback_arg_count).."</color>")
  for k,v in pairs(callback_args) do
    local callback_arg = v[1]
    local callback_arg_value = _G[callback_arg]
    if (callback_arg_value ~= nil) then
      MOD.MessageWithLog (callback_arg..": <color_green>"..tostring(callback_arg_value).."</color>")
      local callback_arg_function = v[2]
      if (callback_arg_function) then
        MOD.MessageWithLog (" callback_function: <color_magenta>"..tostring(callback_arg_function).."</color>")
        callback_arg_function(callback_arg_value)
      end
    end
  end

end

MOD.on_game_loaded()
