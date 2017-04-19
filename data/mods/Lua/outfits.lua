function get_worn_data()
	local worn_data = { }
	for i = -2, -1000,-1 do
		local cloth = player:i_at(i)
		if cloth:is_null() then
			break
		end
		worn_data[#worn_data+1] = cloth:typeId()
--		game.add_msg("Remembered " .. cloth:typeId())
	end
	return worn_data
end

function is_worn(id)
	for i = -2, -1000,-1 do
		local cloth = player:i_at(i)
		if cloth:typeId() == id then
			return true
		elseif cloth:is_null() then
			break
		end
	end
	return false
end

function wear_from_inventory(id)
	for i = 0, 1000 do
		local itm = player:i_at(i)
		if itm:is_null() then
			break
		end
		if itm:typeId() == id then
--			game.add_msg("Wearing " .. (i))
			return player:wear(i)
		end
	end
	return false
end

function wear_from_ground(id)
	local pos = player:pos()
	local stack = map:i_at(pos)
	local iter = stack:cppbegin()
	while not (iter == stack:cppend()) do 
		local e = iter:elem()
		if e:typeId() == id then
			if player:wear_item(e, false) then
				stack:erase(iter)
				return true
			end
		end
		iter:inc()
	end
	return false
end

function set_worn_data(worn_data)
	for i, id in ipairs(worn_data) do
		if is_worn(id) then
		elseif wear_from_inventory(id) then
		elseif wear_from_ground(id) then
		else
			game.add_msg("Could not find item of type " .. id)
		end
	end
end

stored_outfit = { }
function outfits()
	local ui = game.create_uimenu()
	ui:addentry("Save current outfit")
	ui:addentry("Wear an outfit")
	ui:addentry("Cancel")
	ui:query(true)
	local i = ui.ret
	-- TODO: store as flag of the item
	if i == 0 then
		stored_outfit = get_worn_data()
	elseif i == 1 then
		set_worn_data(stored_outfit)
	end
end

game.register_iuse("Lua:outfits", outfits)
