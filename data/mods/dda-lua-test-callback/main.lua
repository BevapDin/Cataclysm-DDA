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

MOD.on_player_skill_increased = function(source, skill, level)

  MOD.DisplayCallbackMessages("Your skill " .. skill .. " was increased by " .. source .. " to " .. tostring(level))

end

MOD.on_player_dodge = function(source, diff)

  MOD.DisplayCallbackMessages("on_player_dodge")

end

MOD.on_player_hit = function(source, bp_hit)

  MOD.DisplayCallbackMessages("on_player_hit")

end

MOD.on_player_hurt = function(source, disturb)

  MOD.DisplayCallbackMessages("on_player_hurt")

end

MOD.on_player_mutation_gain = function(mut)

  MOD.DisplayCallbackMessages("Player has gained mutation " .. mut)

end

MOD.on_player_mutation_loss = function()

  MOD.DisplayCallbackMessages("Player has lost mutation " .. mut)

end

MOD.on_player_stat_change = function(stat, value)

  MOD.DisplayCallbackMessages("on_player_stat_change")

end

MOD.on_player_item_wear = function(item)

  MOD.DisplayCallbackMessages("Player wears a " .. item:tname())

end

MOD.on_player_item_takeoff = function(item)

  MOD.DisplayCallbackMessages("Player has taken off a " .. item:tname())

end

MOD.on_player_effect_int_changes = function(eff, intensity, bp)

  MOD.DisplayCallbackMessages("on_player_effect_int_change")

end

MOD.on_player_mission_assignment = function(id)

  MOD.DisplayCallbackMessages("on_player_mission_assignment: " .. tostring(id))

end

MOD.on_player_mission_finished = function(id)

  MOD.DisplayCallbackMessages("on_player_mission_finished: " .. tostring(id))

end

MOD.on_mapgen_finished = function(type_, terrain, pos) 

  MOD.DisplayCallbackMessages("Mapgen for " .. terrain .. " via " .. type_ .. " has finished at ("..tostring(pos.x)..","..tostring(tp.y)..","..tostring(tp.z)..")")

end

MOD.on_weather_changed = function(new, old)

  MOD.DisplayCallbackMessages("Weather changed from " .. old .. " to " .. new)

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

MOD.lua_dodge = function(creature)

  MOD.MessageWithLog("LUA:You <color_red>have dodged</color> by <color_yellow>"..tostring(creature:get_name()).."</color>")

end

MOD.lua_hit = function(creature)

  MOD.MessageWithLog("LUA:You <color_red>were hit</color> by <color_yellow>"..tostring(creature:get_name()).."</color>")

end

MOD.lua_hurt = function(creature)

  MOD.MessageWithLog("LUA:You <color_red>were hurt</color> by <color_yellow>"..tostring(creature:get_name()).."</color>")

end

MOD.DisplayCallbackMessages = function( callback_name )

  local callback_arg_functions = {
    mapgen_terrain_coordinates = MOD.lua_tripoint,
    item_last_worn = MOD.lua_put_on,
    item_last_taken_off = MOD.lua_put_off,
    source_dodge = MOD.lua_dodge,
    source_hit = MOD.lua_hit,
    source_hurt = MOD.lua_hurt
  }

  MOD.MessageWithLog ("Callback name is <color_cyan>"..tostring(callback_name).."</color>")
  local callback_data = _G["callback_data"]
  local callback_data_length = table_length(callback_data) 
  if callback_data_length > 0 then
    MOD.MessageWithLog ("Callback data length is <color_blue>"..tostring(callback_data_length).."</color>")
    for callback_arg_name,callback_arg_value in pairs(callback_data) do
      MOD.MessageWithLog ("Callback argument <color_yellow>"..tostring(callback_arg_name).."</color> is <color_green>"..tostring(callback_arg_value).."</color>")
      local callback_arg_function = callback_arg_functions[callback_arg_name]
      if (callback_arg_function) then
        MOD.MessageWithLog ("callback_function is <color_magenta>"..tostring(callback_arg_function).."</color>")
        callback_arg_function(callback_arg_value)
      end
    end
  else
    MOD.MessageWithLog ("Callback data is <color_red>empty</color>")
  end

end

MOD.on_game_loaded()
