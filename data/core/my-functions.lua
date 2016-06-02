check_empty = function(p)
    if not map:i_at(p):empty() then
        error("There are items at ("..tostring(p.x)..","..tostring(p.y)..","..tostring(p.z)..")")
    end
    if map:field_at(p):fieldCount() > 0 then
        error("There are fields at ("..tostring(p.x)..","..tostring(p.y)..","..tostring(p.z)..")")
    end
end

copymap = function(p0, dx, dy)
    game.add_msg("copymap("..tostring(p0.x)..","..tostring(p0.y)..","..tostring(p0.z)..","..tostring(dx)..","..tostring(dy)..")")
    local t = {}
    local f = {}
    for x = p0.x,p0.x + dx - 1 do
        for y = p0.y,p0.y + dx - 1 do
            p = tripoint(x, y, p0.z)
            check_empty(p)
            t[#t+1] = map:ter(p)
            f[#f+1] = map:furn(p)
        end
    end
    return { t = t, f = f, dx = dx, dy = dy }
end

pastemap = function(tab, p0)
    local t = tab.t
    local f = tab.f
    local dx = tab.dx
    local dy = tab.dy
    local i = 1
    game.add_msg("pastemap("..tostring(p0.x)..","..tostring(p0.y)..","..tostring(p0.z)..","..tostring(dx)..","..tostring(dy)..")")
    for x = p0.x,p0.x + dx - 1 do
        for y = p0.y,p0.y + dx - 1 do
            p = tripoint(x, y, p0.z)
            map:ter_set(p, t[i])
            map:furn_set(p, f[i])
            check_empty(p)
            i = i + 1
        end
    end
end

pos_adjusted = function(p)
    -- adjust p so it always points to the topleft of a overmap terrain (24x24 tiles):
    -- 1. adjust so it points to the top left of a submap.
    p.x = p.x - (p.x % 12)
    p.y = p.y - (p.y % 12)
    -- 2. if the reality bubble (to which p is local!) starts in the middle of an overmap terrain,
    -- further adjust the coordiantes.
    local a = map:get_abs_sub()
    if a.x % 2 == 0 then
        if p.x % 24 ~= 0 then p.x = p.x - 12 end
    else
        if p.x % 24 == 0 then p.x = p.x - 12 end
    end
    if a.y % 2 == 0 then
        if p.y % 24 ~= 0 then p.y = p.y - 12 end
    else
        if p.y % 24 == 0 then p.y = p.y - 12 end
    end
    return p
end

player_pos_adjusted = function()
    return pos_adjusted(player:pos())
end

mread = function()
    local a = player_pos_adjusted()
    return copymap(a, 24, 24)
end

mwrite = function(t)
    local a = player_pos_adjusted()
    pastemap(t, a)
end

mwriteswap = function(t)
    local a = player_pos_adjusted()
    local nt = copymap(a, t.dx, t.dy)
    pastemap(t, a)
    return nt
end

mswap = function()
    local p1 = pos_adjusted(g:look_around())
    local p2 = pos_adjusted(g:look_around())
    local t1 = copymap(p1, 24, 24)
    local t2 = copymap(p2, 24, 24)
    pastemap(t1, p2)
    pastemap(t2, p1)
end

sponageable = { "rag", "scrap", "steel_lump", "steel_chunk", "frame", "hdframe",
"splinter", "log", "rock", "rebar", "2x4", "ceramic_shard", "pipe",
"steel_plate", "xlframe", "glass_shard", "glass_sheet" }

function has_value (tab, val)
    for index, value in ipairs (tab) do
        if value == val then
            return true
        end
    end

    return false
end

function sponge_2(it, pos, added)
    local map_stack = map:i_at(pos)
    local iter = map_stack:cppbegin()
    while iter ~= map_stack:cppend() do
        local e = iter:elem()
        local t = e:typeId()
        if e:damage() == 0 and has_value(sponageable, t) then
            local var = "cnt:" .. t
            local cnt = it:get_var(var, 0)
            cnt = cnt + 1
            it:set_var(var, cnt)
            if it:get_var(var, 0) ~= cnt then
                error("failed to store the count")
            end
            if not added[t] then
                added[t] = 0
            end
            added[t] = added[t] + 1
            iter = map_stack:erase(iter)
        else
            iter:inc()
        end
    end
end

function unsponge_specific(it, t, query)
    local var = "cnt:" .. t
    local count_avail = it:get_var(var, 0)
    if count_avail > 0 then
        local count_todo = count_avail
        if query then
            local foo = game.string_input_popup("How many " .. item(t, 0):tname() .. " to drop?", 80, tostring(count_avail))
            count_todo = tonumber(foo)
            if count_todo > count_avail then
                game.popop("Too much. Aborting.")
                count_todo = 0
            elseif count_todo < 0 then
                count_todo = 0
            end
        end
        if count_todo > 0 then
            game.add_msg("Dropping " .. tostring(count_todo) .. " " .. item(t, 0):tname(count_todo) .. ".")
            map:spawn_item(player:pos(), t, count_todo)
            if count_avail == count_todo then
                it:erase_var(var)
            else
                it:set_var(var, count_avail - count_todo)
            end
        end
    end
end

function unsponge(it)
    local menu = game.create_uimenu()
    menu.title = "Drop which items?"
    local sp = {}
    local i = 0
    for _, t in ipairs(sponageable) do
        local cnt = it:get_var("cnt:" .. t, 0)
        if cnt > 0 then
            menu:addentry("Drop up to " .. tostring(cnt) .. " " .. item(t, 0):tname(cnt) .. ".")
            sp[i] = t
            i = i + 1
        end
    end
    menu:addentry("Cancel.")
    menu:query(true)
    if sp[menu.selected] then
        unsponge_specific(it, sp[menu.selected], true)
    end
end

function unsponge_destroy(it)
    local menu = game.create_uimenu()
    menu.title = "Destroy which items?"
    local sp = {}
    local i = 0
    for _, t in ipairs(sponageable) do
        local cnt = it:get_var("cnt:" .. t, 0)
        if cnt > 0 then
            menu:addentry("Destroy up to " .. tostring(cnt) .. " " .. item(t, 0):tname(cnt) .. ".")
            sp[i] = t
            i = i + 1
        end
    end
    menu:addentry("Cancel.")
    menu:query(true)
    if sp[menu.selected] then
        local t = sp[menu.selected]
        local var = "cnt:" .. t
        local count_avail = it:get_var(var, 0)
        local foo = game.string_input_popup("Multiples of " .. item(t, 0):tname() .. " to destroy?", 80, "")
        local mult = tonumber(foo)
        if mult <= 0 or mult > count_avail or count_avail <= 0 then
            return
        end
        local count_todo = math.floor(count_avail / mult) * mult
        game.add_msg("Destroying " .. tostring(count_todo / mult) .. " x " .. tostring(mult) .. " " .. item(t, 0):tname(count_todo) .. ".")
        if count_avail == count_todo then
            it:erase_var(var)
        else
            it:set_var(var, count_avail - count_todo)
        end
    end
end

function list_sponged(it)
    for _, t in ipairs(sponageable) do
        local var = "cnt:" .. t
        local cnt = it:get_var(var, 0)
        if cnt > 0 then
            game.add_msg(it:tname() .. " contains " .. tostring(cnt) .. " " .. item(t, 0):tname(cnt) .. ".")
        end
    end
end

function custom_sponge(it, active, pos)
    local menu = game.create_uimenu()
    menu.title = "What to do?"
    menu:addentry("Collect surrounding items.")
    menu:addentry("Drop all contained items.")
    menu:addentry("Drop some contained items.")
    menu:addentry("Destroy some contained items.")
    menu:addentry("List contained items.")
    menu:addentry("Cancel.")
    menu:query(true)
    if menu.selected == 0 then
        local p = player:pos()
        local added = {}
        sponge_2(it, tripoint(p.x - 1, p.y - 1, p.z), added)
        sponge_2(it, tripoint(p.x - 1, p.y    , p.z), added)
        sponge_2(it, tripoint(p.x - 1, p.y + 1, p.z), added)
        sponge_2(it, tripoint(p.x    , p.y - 1, p.z), added)
        sponge_2(it, tripoint(p.x    , p.y    , p.z), added)
        sponge_2(it, tripoint(p.x    , p.y + 1, p.z), added)
        sponge_2(it, tripoint(p.x + 1, p.y - 1, p.z), added)
        sponge_2(it, tripoint(p.x + 1, p.y    , p.z), added)
        sponge_2(it, tripoint(p.x + 1, p.y + 1, p.z), added)
        for t, c in pairs(added) do
            game.add_msg("Collected " .. tostring(c) .. " " .. item(t, 0):tname(c) .. ".")
        end
    elseif menu.selected == 1 then
        for _, t in ipairs(sponageable) do
            unsponge_specific(it, t, false)
        end
    elseif menu.selected == 2 then
        unsponge(it)
    elseif menu.selected == 3 then
        unsponge_destroy(it)
    elseif menu.selected == 4 then
        list_sponged(it)
    end
    return 0
end

game.register_iuse("CUSTOM_SPONGE", custom_sponge)
