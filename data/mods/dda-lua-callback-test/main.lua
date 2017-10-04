local MOD = {

  id = "dda-lua-callback-test",
  version = "2017-10-04"

}

mods[MOD.id] = MOD

function MOD.on_game_loaded() 

  MOD.PrintDebugInfo("on_game_loaded")

end

function MOD.on_savegame_loaded()

  MOD.PrintDebugInfo("on_savegame_loaded")

end

function MOD.on_stat_change()

  MOD.PrintDebugInfo("on_stat_change")

end

function MOD.on_skill_increased()

  MOD.PrintDebugInfo("on_skill_increased")

end

function MOD.on_turn_passed()

  MOD.PrintDebugInfo("on_turn_passed")

end

function MOD.on_second_passed()

  MOD.PrintDebugInfo("on_second_passed")

end

function MOD.on_minute_passed()

  MOD.PrintDebugInfo("on_minute_passed")

end

function MOD.on_hour_passed()

  MOD.PrintDebugInfo("on_hour_passed")

end

function MOD.on_day_passed()

  MOD.PrintDebugInfo("on_day_passed")

end

function MOD.on_year_passed()

  MOD.PrintDebugInfo("on_year_passed")

end

function MOD.on_mutation_gain()

  MOD.PrintDebugInfo("on_mutation_gain")

end

function MOD.on_mutation_loss()

  MOD.PrintDebugInfo("on_mutation_loss")

end

function MOD.on_stat_change()

  MOD.PrintDebugInfo("on_stat_change")

end

function MOD.on_item_wear()

  MOD.PrintDebugInfo("on_item_wear")

end

function MOD.on_item_takeoff()

  MOD.PrintDebugInfo("on_item_takeoff")

end

function MOD.on_effect_int_changes()

  MOD.PrintDebugInfo("on_effect_int_change")

end

function MOD.on_mission_assignment()

  MOD.PrintDebugInfo("on_mission_assignment")

end

function MOD.on_mission_finished()

  MOD.PrintDebugInfo("on_mission_finished")

end

function MOD.on_mapgen_finished() 

  MOD.PrintDebugInfo("on_mapgen_finished")

end

MOD.PrintDebugInfo = function(s)

  if (game.add_msg) then
    game.add_msg ("<color_cyan>     function: </color><color_ltcyan>"..tostring(s).."</color>")
    game.add_msg ("callback_last: <color_yellow>"..tostring(callback_last).."</color>")
    if (callback_arg_count) then
      game.add_msg ("callback_arg_count: <color_green>"..tostring(callback_arg_count).."</color>")
      for i = 1, callback_arg_count do
      local callback_arg = _G["callback_arg"..i]
      local callback_arg_color = "green"
        if (callback_arg == nil) then
          callback_arg_color = "red"
        end
        game.add_msg ("callback_arg"..i..": <color_"..callback_arg_color..">"..tostring(callback_arg).."</color>")
      end
    end
  end

end

MOD.on_game_loaded()
