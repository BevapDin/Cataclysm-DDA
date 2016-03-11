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

Sponge = {}
Sponge.__index = Sponge
Sponge.sponageable = { "rag", "scrap", "steel_lump", "steel_chunk", "frame", "hdframe",
"splinter", "log", "rock", "rebar", "2x4", "ceramic_shard", "pipe",
"steel_plate", "xlframe", "glass_shard", "glass_sheet" }

function Sponge.create(it)
   local new_object = {}
   setmetatable(new_object, Sponge)
   new_object.it = it
   return new_object
end

function Sponge.can_sponge(item_type_id)
    for _, value in ipairs(Sponge.sponageable) do
        if value == item_type_id then
            return true
        end
    end
    return false
end

function Sponge:add_count(item_type_id, count)
    local old_count = self:get_count(item_type_id)
    self:set_count(item_type_id, old_count + count)
end
function Sponge:get_count(item_type_id)
    if not item_type_id then
        return 0
    end
    return tonumber(self.it:get_var("cnt:" .. item_type_id, 0))
end
function Sponge:set_count(item_type_id, count)
    if not item_type_id then
        return
    end
    if count == 0 then
        self.it:erase_var("cnt:" .. item_type_id)
    else
        self.it:set_var("cnt:" .. item_type_id, string.format("%.0f", count))
    end
    if self:get_count(item_type_id) ~= count then
        error("failed to store the count")
    end
end

-- Collect all spongable items from the map at given position.
-- The items are removed from the map.
-- The collected amount is added to `self.added[item_type_id]`
function Sponge:collect(pos)
    local map_stack = map:i_at(pos)
    local iter = map_stack:cppbegin()
    while iter ~= map_stack:cppend() do
        local e = iter:elem()
        local t = e:typeId()
        if e:damage() == 0 and Sponge.can_sponge(t) then
            -- TODO: check for charges!
            self:add_count(t, 1)
            if not self.added[t] then
                self.added[t] = 0
            end
            self.added[t] = self.added[t] + 1
            iter = map_stack:erase(iter)
        else
            iter:inc()
        end
    end
end

-- Shows a menu containing all the contained items. Returns the select item type id,
-- or nil if nothing was selected.
function Sponge:query_item_type(title)
    local menu = game.create_uimenu()
    menu.title = title
    local sp = {}
    local i = 0
    for _, item_type_id in ipairs(Sponge.sponageable) do
        local count = self:get_count(item_type_id)
        if count > 0 then
            menu:addentry(Sponge.name_with_count(item_type_id, count))
            sp[i] = item_type_id
            i = i + 1
        end
    end
    menu:addentry("Cancel.")
    menu:query(true)
    return sp[menu.selected]
end

function Sponge:query_count(item_type_id, title)
    local count_avail = self:get_count(item_type_id)
    if count_avail <= 0 then
        return nil
    end
    title = title .. " (" .. Sponge.name_with_count(item_type_id, count_avail) .. ")?"
    local answer = game.string_input_popup(title, 80, "")
    if answer:len() == 0 then
        return nil
    end
    if answer == "all" then
        return {
            portion = 1,
            count = count_avail,
            text = "all " .. Sponge.name_with_count(item_type_id, count_avail)
        }
    elseif answer:sub(-1) == "x" then
        local portion = tonumber(answer:sub(1, answer:len() - 1))
        if portion <= 0 or portion > count_avail then
            return nil
        end
        local count_chosen = math.floor(count_avail / portion) * portion
        return {
            portion = portion,
            count = count_chosen,
            text = string.format("%.0f", count_chosen / portion) .. " x " .. Sponge.name_with_count(item_type_id, portion)
        }
    else
        local count_chosen = tonumber(answer)
        if count_chosen <= 0 or count_chosen > count_avail then
            return nil
        end
        return {
            portion = 1,
            count = count_chosen,
            text = Sponge.name_with_count(item_type_id, count_chosen)
        }
    end
end

function Sponge:drop_some(pos)
    local item_type_id = self:query_item_type("Drop which items?")
    self:drop(item_type_id, pos, self:query_count(item_type_id, "Drop how many"))
end

function Sponge:destroy_some()
    local item_type_id = self:query_item_type("Destroy which items?")
    self:destroy(item_type_id, self:query_count(item_type_id, "Destroy how many"))
end

function Sponge:drop(item_type_id, pos, cpair)
    if not item_type_id or not cpair then
        return
    end
    game.add_msg("Dropping " .. cpair.text .. ".")
    -- TODO: charges!
    map:spawn_item(pos, item_type_id, cpair.count)
    self:add_count(item_type_id, -cpair.count)
end

function Sponge:destroy(item_type_id, cpair)
    if not item_type_id or not cpair then
        return
    end
    game.add_msg("Destroying " .. cpair.text .. ".")
    self:add_count(item_type_id, -cpair.count)
end

function Sponge:list()
    for _, item_type_id in ipairs(Sponge.sponageable) do
        local count = self:get_count(item_type_id)
        if count > 0 then
            game.add_msg("The " .. self.it:tname() .. " contains " .. Sponge.name_with_count(item_type_id, count) .. ".")
        end
    end
end

function Sponge.name_with_count(item_type_id, count)
    if not item_type_id then
        return ""
    elseif count == 1 then
        return "a " .. item(item_type_id, 0):tname(1)
    elseif count < 0 then
        return "nothing"
    else
        return string.format("%.0f %s", count, item(item_type_id, 0):tname(count))
    end
end

function Sponge:collect_around(center)
    local p = center
    self.added = {}
    self:collect(tripoint(p.x - 1, p.y - 1, p.z))
    self:collect(tripoint(p.x - 1, p.y    , p.z))
    self:collect(tripoint(p.x - 1, p.y + 1, p.z))
    self:collect(tripoint(p.x    , p.y - 1, p.z))
    self:collect(tripoint(p.x    , p.y    , p.z))
    self:collect(tripoint(p.x    , p.y + 1, p.z))
    self:collect(tripoint(p.x + 1, p.y - 1, p.z))
    self:collect(tripoint(p.x + 1, p.y    , p.z))
    self:collect(tripoint(p.x + 1, p.y + 1, p.z))
    for item_type_id, count in pairs(self.added) do
        game.add_msg("Collected " .. Sponge.name_with_count(item_type_id, count) .. ", now " .. string.format("%.0f", self:get_count(item_type_id)) .. ".")
    end
end

function Sponge:invoke()
    local menu = game.create_uimenu()
    menu.title = "What to do?"
    menu:addentry("Collect surrounding items.")
    menu:addentry("Drop some contained items.")
    menu:addentry("Destroy some contained items.")
    menu:addentry("List contained items.")
    menu:addentry("Cancel.")
    menu:query(true)
    if menu.selected == 0 then
        self:collect_around(player:pos())
    elseif menu.selected == 1 then
        self:drop_some(player:pos())
    elseif menu.selected == 2 then
        self:destroy_some()
    elseif menu.selected == 3 then
        self:list()
    end
    return 0
end

function custom_sponge(it, active, pos)
    local sponge = Sponge.create(it)
    sponge:invoke()
end

game.register_iuse("CUSTOM_SPONGE", custom_sponge)
