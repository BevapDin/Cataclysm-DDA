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
