function journal(item, active, pos)
	local ui = game.create_uimenu()
	ui:addentry("Write to your journal")
	ui:addentry("Read journal")
	ui:addentry("Cancel")
	ui:query(true)
	local i = ui.ret
	if i == 0 then
		local t = "<color_red>" .. calendar.turn:textify_period() .. "</color> into the cataclysm:\n"
		local line = game.string_input_popup("New journal entry:", 100, t)
		if not (line == '') then
			item:set_var("lua:journal", item:get_var("lua:journal") .. t .. line .. "\n\n")
			player:mod_moves(200)
		end
	elseif i == 1 then
		game.popup(item:get_var("lua:journal", "[empty]"))
	end
end

game.register_iuse("Lua:journal", journal)
