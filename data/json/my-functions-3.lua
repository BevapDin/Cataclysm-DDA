function custom_flowerpot(item, active, pos)
    if not player:has_quality(quality_id('DIG'), 1) then
        game.add_msg('You need a digging tool to use the flowerpot.');
        return 0
    end

    local flower_name = item:get_var('flower_name', '')

    if flower_name:len() > 0 then
        local x, y = pos.x, pos.y
        x, y = game.choose_adjacent('Place the ' .. flower_name .. ' where?', x, y)
        if not x then
            game.add_msg('Never mind.')
            return 0
        end
        local targ = tripoint(x, y, player:posz())
        if map:impassable(targ) or not map:has_flag('DIGGABLE', targ) or map:has_furn(targ) then
            game.add_msg('You can not dig at that place!')
            return 0
        end

        map:furn_set(targ, furn_str_id(item:get_var('furniture', '')))

        local seed_json = item:get_var('seed_json', '')
        if seed_json:len() > 0 then
            local seed = item()
            seed:deserialize(seed_json)
            map:add_item(targ, seed)
        end

        game.add_msg('Placed the ' .. flower_name .. '.')
        item:erase_var('furniture')
        item:erase_var('volume')
        item:erase_var('weight')
        item:erase_var('name')
        item:erase_var('flower_name')
        item:erase_var('seed_json')
        return 0
    else
        local x, y = pos.x, pos.y
        x, y = game.choose_adjacent('Pick up a plant where?', x, y)
        if not x then
            game.add_msg('Never mind.')
            return 0
        end
        local targ = tripoint(x, y, player:posz())
        if not map:has_furn(targ) then
            game.add_msg('There is no plant!')
            return 0
        end
        local flower_name = map:name(targ)
        local fid = map:get_furn(targ)
        local f = fid:str()
        local ms = map:i_at(targ)
        local is_seed = ( f == 'f_plant_seed' or f == 'f_plant_seedling' or
                          f == 'f_plant_mature' or f == 'f_plant_harvest' ) and
                        ms:size() > 0
        local is_plant = f == 'f_mutpoppy' or f == 'f_flower_fungal' or f == 'f_bluebell' or
                         f == 'f_dahlia' or f == 'f_datura' or f == 'f_flower_marloss' or
                         f == 'f_dandelion' or f == 'f_chamomile' or f == 'f_cattails'
        if not is_seed and not is_plant then
            game.add_msg('That ' .. map:name(targ) .. ' can not be picked up!')
            return 0
        end

        map:furn_set(targ, furn_str_id('f_null'))
        item:set_var('furniture', fid:str())
        item:set_var('volume', 40)
        item:set_var('weight', 10000)
        item:set_var('name', 'flowerpot with a ' .. flower_name)
        item:set_var('flower_name', flower_name)
        if is_seed then
            local seed = ms:cppbegin():elem()
            item:set_var('seed_json', seed:serialize())
            ms:erase(ms:cppbegin())
        end
        game.add_msg('Dug out the ' .. flower_name .. '.')
        return 0
    end
end

game.register_iuse('CUSTOM_FLOWERPOT', custom_flowerpot)
