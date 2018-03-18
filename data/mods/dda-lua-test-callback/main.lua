local MOD = {

  id = "dda-lua-test-callback",
  version = "2018-03-18"

}

mods[MOD.id] = MOD

function MOD.on_game_loaded() 

  MOD.DisplayCallbackMessages("on_game_loaded")

end

function MOD.on_savegame_loaded()

  MOD.DisplayCallbackMessages("on_savegame_loaded")

end

--[[
function MOD.on_new_player_created()

  MOD.DisplayCallbackMessages("on_new_player_created")

end
]]--

function MOD.on_skill_increased()

  MOD.DisplayCallbackMessages("on_skill_increased")

end

function MOD.on_turn_passed()

  MOD.DisplayCallbackMessages("on_turn_passed")

end

function MOD.on_second_passed()

  MOD.DisplayCallbackMessages("on_second_passed")

end

function MOD.on_minute_passed()

  MOD.DisplayCallbackMessages("on_minute_passed")

end

function MOD.on_hour_passed()

  MOD.DisplayCallbackMessages("on_hour_passed")

end

function MOD.on_day_passed()

  MOD.DisplayCallbackMessages("on_day_passed")

end

function MOD.on_year_passed()

  MOD.DisplayCallbackMessages("on_year_passed")

end

function MOD.on_mutation_gain()

  MOD.DisplayCallbackMessages("on_mutation_gain")

end

function MOD.on_mutation_loss()

  MOD.DisplayCallbackMessages("on_mutation_loss")

end

function MOD.on_stat_change()

  MOD.DisplayCallbackMessages("on_stat_change")

end

function MOD.on_item_wear()

  MOD.DisplayCallbackMessages("on_item_wear")

end

function MOD.on_item_takeoff()

  MOD.DisplayCallbackMessages("on_item_takeoff")

end

function MOD.on_effect_int_changes()

  MOD.DisplayCallbackMessages("on_effect_int_change")

end

function MOD.on_mission_assignment()

  MOD.DisplayCallbackMessages("on_mission_assignment")

end

function MOD.on_mission_finished()

  MOD.DisplayCallbackMessages("on_mission_finished")

end

function MOD.on_mapgen_finished() 

  MOD.DisplayCallbackMessages("on_mapgen_finished")

end

function MOD.on_weather_changed()

  MOD.DisplayCallbackMessages("on_weather_changed")

end

MOD.DisplayCallbackMessages = function(s)

  local callback_args = {

    "mapgen_generator_type",
    "mapgen_terrain_type_id",
    "skill_increased_source",
    "skill_increased_id",
    "skill_increased_level",
    "mutation_gained",
    "mutation_lost",
    "stat_changed",
    "stat_value",
    "item_last_worn",
    "item_last_taken_off",
    "effect_changed",
    "effect_intensity",
    "effect_bodypart",
    "mission_finished",
    "mission_assigned",
    "weather_new",
    "weather_old"

  }  

  if (game.add_msg) then
    MOD.MessageWithLog ("<color_cyan>     function: </color><color_ltcyan>"..tostring(s).."</color>")
    MOD.MessageWithLog ("callback_last: <color_yellow>"..tostring(callback_last).."</color>")
    MOD.MessageWithLog ("callback_arg_count: <color_red>"..tostring(callback_arg_count).."</color>")
    for k,v in pairs(callback_args) do
      local callback_arg = v
      local callback_arg_value = _G[callback_arg]
        if (callback_arg_value ~= nil) then
          MOD.MessageWithLog (callback_arg..": <color_green>"..tostring(callback_arg_value).."</color>")
        end  
    end
  end

end

MOD.MessageWithLog = function( s )

    game.add_msg( s )
    LOG.message( s )

end

MOD.on_game_loaded()
