function nymph_attack(m)
	if game.distance(player:posx(), player:posy(), m:posx(), m:posy()) <= 1 then
		if not player:i_at(-1):is_null() then
			game.add_msg("The " .. m:name() .. " steals your " .. player:i_at(-1):tname() .. ".")
			m:add_item(player:i_at(-1))
			player:i_rem(-1)
		end
	end
end

game.register_attack( "nymph_attack", nymph_attack )

