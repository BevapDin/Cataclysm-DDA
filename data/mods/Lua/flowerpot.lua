function flowerpot(item, active, pos)
	local p = player:pos()
	if not player:has_quality(quality_id("DIG"), 1) then
		game.add_msg("You need some digging tool.")
		return
	end
	if item:get_var("lua:flowerpot", "") == '' then
		if not (map:has_flag_furn("TRANSPARENT", p) and map:has_flag_furn("TINY", p) and map:has_flag_furn("FLAMMABLE_ASH", p)) then
			game.add_msg("Try again while standing on top of a flower or similar.")
			return
		end
		local flower = map:furn(p):obj()
		game.add_msg("You dig up the " .. flower.name .. " and put it into the " .. item:tname() .. ".")
		item:set_var("contained_name", flower.name)
		item:set_var("lua:flowerpot", flower.id:str())
		map:furn_set(p, furn_str_id("f_null"):id())
		player:mod_moves(200)
	else
		if not (map:furn(p) == furn_str_id("f_null"):id()) then
			game.add_msg("Try again while standing on an *empty* spot.")
			return
		end
		if not (map:ter(p) == ter_str_id("t_dirt"):id() or map:ter(p) == ter_str_id("t_grass"):id()) then
			game.add_msg("Try again while standing on grass or dirt.")
			return
		end
		local flower = furn_str_id(item:get_var("lua:flowerpot", "")):id():obj()
		map:furn_set(p, flower.id:id())
		item:erase_var("lua:flowerpot")
		item:erase_var("contained_name")
		player:mod_moves(200)
		game.add_msg("You plant the " .. flower.name .. ".")
	end
end

game.register_iuse("Lua:flowerpot", flowerpot)
