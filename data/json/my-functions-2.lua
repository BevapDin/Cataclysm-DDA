function move_items(source, dest)
    local ms = map:i_at(source)
    while not ms:empty() do
        map:add_item(dest, ms:cppbegin():elem())
        ms:erase(ms:cppbegin())
    end
end

function move_all(source, dest)
    local old_ter = map:ter(dest)
    local old_furn = map:furn(dest)

    if not map:i_at(dest):empty() then
        error("map at " .. tostring(dest.x) .. "," .. tostring(dest.y) .. "," .. tostring(dest.z) .. " has items!")
    end

    map:ter_set(dest, map:ter(source))
    map:furn_set(dest, map:furn(source))

    move_items(source, dest)

    map:ter_set(source, old_ter)
    map:furn_set(source, old_furn)
end

function move_map_parts(source1, source2, destination)
    if source1.x > source2.x then
        local x = source1.x
        source1.x = source2.x
        source2.x = x
    end
    if source1.y > source2.y then
        local y = source1.y
        source1.y = source2.y
        source2.y = y
    end
    local lx = source2.x - source1.x
    local ly = source2.y - source1.y

    if source1.x <= destination.x and source2.x <= destination.x then
        for ex = 0, lx do
            local x = lx - ex
            if source1.y <= destination.y and source2.y <= destination.y then
                for ey = 0, ly do
                    local y = ly - ey
                    local source = tripoint(source1.x + x, source1.y + y, source1.z)
                    local dest = tripoint(destination.x + x, destination.y + y, destination.z)
                    move_all(source, dest)
                end
            else
                for y = 0, ly do
                    local source = tripoint(source1.x + x, source1.y + y, source1.z)
                    local dest = tripoint(destination.x + x, destination.y + y, destination.z)
                    move_all(source, dest)
                end
            end
        end
    else
        for x = 0, lx do
            if source1.y <= destination.y and source2.y <= destination.y then
                for ey = 0, ly do
                    local y = ly - ey
                    local source = tripoint(source1.x + x, source1.y + y, source1.z)
                    local dest = tripoint(destination.x + x, destination.y + y, destination.z)
                    move_all(source, dest)
                end
            else
                for y = 0, ly do
                    local source = tripoint(source1.x + x, source1.y + y, source1.z)
                    local dest = tripoint(destination.x + x, destination.y + y, destination.z)
                    move_all(source, dest)
                end
            end
        end
    end
end

function move_rect()
    local source1 = g:look_debug()
    local source2 = g:look_debug()
    local destination = g:look_debug()

    if map:inbounds(source1) and map:inbounds(source2) and map:inbounds(destination) then
        move_map_parts(source1, source2, destination)
    end
end

function move_tile()
    local source = g:look_debug()
    local destination = g:look_debug()

    if map:inbounds(source) and map:inbounds(destination) then
        move_map_parts(source, source, destination)
    end
end
