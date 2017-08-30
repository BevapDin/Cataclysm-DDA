local MOD = {

  id = "dda-lua-callback-test",
  version = "2017-08-31"

}

mods[MOD.id] = MOD

function MOD.on_game_loaded() 

  MOD.Init("on_game_loaded")

end

function MOD.on_new_player_created()

  MOD.Update("on_new_player_created")

end

function MOD.on_skill_increased()

  MOD.Update("on_skill_increased")

end

function MOD.on_turn_passed()

  MOD.Update("on_turn_passed")

end

function MOD.on_second_passed()

  MOD.Update("on_second_passed")

end

function MOD.on_minute_passed()

  MOD.Update("on_minute_passed")

end

function MOD.on_hour_passed()

  MOD.Update("on_hour_passed")

end

function MOD.on_day_passed()

  MOD.Update("on_day_passed")

end

function MOD.on_year_passed()

  MOD.Update("on_year_passed")

end

function MOD.on_mutation_gain()

  MOD.Update("on_mutation_gain")

end

function MOD.on_mutation_loss()

  MOD.Update("on_mutation_loss")

end

function MOD.on_stat_change()

  MOD.Update("on_stat_change")

end

function MOD.on_item_wear()

  MOD.Update("on_item_wear")

end

function MOD.on_item_takeoff()

  MOD.Update("on_item_takeoff")

end

function MOD.on_effect_int_changes()

  MOD.Update("on_effect_int_change")

end

function MOD.on_mission_assignment()

  MOD.Update("on_mission_assignment")

end

function MOD.on_mission_finished()

  MOD.Update("on_mission_finished")

end

MOD.Init = function(s)

  if (game.add_msg) then
    game.add_msg ("Initialization logic goes here...")
    game.add_msg (tostring(s))
  end

end

MOD.Update = function(s)

  if (game.add_msg) then
    game.add_msg ("Update logic goes here...")
    game.add_msg (tostring(s))
    game.add_msg ("callback_last:"..tostring(callback_last))
    game.add_msg ("callback_arg1:"..tostring(callback_arg1))
    game.add_msg ("callback_arg2:"..tostring(callback_arg2))
    game.add_msg ("callback_arg3:"..tostring(callback_arg3))
  end

end

MOD.on_game_loaded()
