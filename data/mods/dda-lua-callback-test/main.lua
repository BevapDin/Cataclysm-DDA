local MOD = {

  id = "dda-lua-callback-test",
  version = "2017-09-01"

}

mods[MOD.id] = MOD

function MOD.on_game_loaded() 

  MOD.Test("on_game_loaded")

end

function MOD.on_savegame_loaded()

  MOD.Test("on_savegame_loaded")

end

function MOD.on_stat_change()

  MOD.Test("on_stat_change")

end

function MOD.on_skill_increased()

  MOD.Test("on_skill_increased")

end

function MOD.on_turn_passed()

  MOD.Test("on_turn_passed")

end

function MOD.on_second_passed()

  MOD.Test("on_second_passed")

end

function MOD.on_minute_passed()

  MOD.Test("on_minute_passed")

end

function MOD.on_hour_passed()

  MOD.Test("on_hour_passed")

end

function MOD.on_day_passed()

  MOD.Test("on_day_passed")

end

function MOD.on_year_passed()

  MOD.Test("on_year_passed")

end

function MOD.on_mutation_gain()

  MOD.Test("on_mutation_gain")

end

function MOD.on_mutation_loss()

  MOD.Test("on_mutation_loss")

end

function MOD.on_stat_change()

  MOD.Test("on_stat_change")

end

function MOD.on_item_wear()

  MOD.Test("on_item_wear")

end

function MOD.on_item_takeoff()

  MOD.Test("on_item_takeoff")

end

function MOD.on_effect_int_changes()

  MOD.Test("on_effect_int_change")

end

function MOD.on_mission_assignment()

  MOD.Test("on_mission_assignment")

end

function MOD.on_mission_finished()

  MOD.Test("on_mission_finished")

end

MOD.Test = function(s)

  if (game.add_msg) then
    game.add_msg ("<color_cyan>     function: </color><color_ltcyan>"..tostring(s).."</color>")
    game.add_msg ("callback_last: <color_yellow>"..tostring(callback_last).."</color>")
    game.add_msg ("callback_last: <color_green>"..tostring(callback_arg_count).."</color>")
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

MOD.on_game_loaded()
