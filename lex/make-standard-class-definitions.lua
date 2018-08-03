#! /usr/bin/lua
    
function sorted_keys(t)
    local res = { }
    for k, _ in pairs(t) do
        if type(k) == "string" then
            table.insert(res, k)
        end
    end
    table.sort(res)
    return res;
end

function comparator(a, b)
    if type(a[2]) == "string" then
        return a[2] < b[2]
    end
    if type(a[2]) == "table" then
        if a[2].name and b[2].name then
            return a[2].name < b[2].name
        end
        if a[2][1] and b[2][1] then
            return a[2][1] < b[2][1]
        elseif a[2][1] and not b[2][1] then
            return true
        elseif not a[2][1] and b[2][1] then
            return false
        end
        return a[3] < b[3]
    end
    error("don't know how to compare: " .. type(a[2]))
end

function sorted_indizes(t, l)
    local res = { }
    for k, v in ipairs(t) do
        if type(k) == "number" then
            table.insert(res, {k, v, serialize(v, l)})
        end
    end
    table.sort(res, comparator)
    return res;
end

function serialize(t, l)
    local tab = string.rep("    ", l)
    if type(t) == "number" then
        return tostring(t)
    elseif type(t) == "boolean" then
        if t then
            return "true"
        else
            return "false"
        end
    elseif type(t) == "string" then
        return '"' .. t:gsub("\\", "\\\\"):gsub("\"", "\\\"") .. '"'
    elseif type(t) == "table" then
        local result = "{\n"
        for _, k in ipairs(sorted_keys(t)) do
            local v = t[k]
            result = result .. tab .. tostring(k) .. " = " .. serialize(v, l + 1) .. ",\n"
        end
        for _, v in ipairs(sorted_indizes(t, l + 1)) do
            result = result .. tab .. v[3] .. ",\n"
        end
        return result .. string.rep("    ", l - 1) .. "}"
    else
        error("unknown type: " .. type(t))
    end
end

function serialize_arguments(t, l)
    local result = "{"
    for _, v in ipairs(t) do
        result = result .. " " .. serialize(v, l + 1) .. ","
    end
    for _, k in ipairs(sorted_keys(t)) do
        error("keys are inside the array: " .. tostring(k))
    end
    return result .. " }"
end

function serialize_function(t, l)
    local result = "{ "
    for _, k in ipairs(sorted_keys(t)) do
        local v = t[k]
        if k == "args" then
            result = result .. " " .. tostring(k) .. " = " .. serialize_arguments(v, l + 1) .. ","
        elseif k == "argnames" then
            result = result .. " " .. tostring(k) .. " = " .. serialize_arguments(v, l + 1) .. ","
        else
            result = result .. " " .. tostring(k) .. " = " .. serialize(v, l + 1) .. ","
        end
    end
    for _, v in ipairs(sorted_indizes(t, l + 1)) do
        error("array indizes in table: " .. tostring(v))
    end
    return result .. " }"
end

function serialize_class(t, l)
    local tab = string.rep("    ", l)
    local result = "{\n"
    for _, k in ipairs(sorted_keys(t)) do
        local v = t[k]
        if k == "functions" then
            result = result .. tab .. tostring(k) .. " = " .. serialize_array(v, l + 1, serialize_function) .. ",\n"
        else
            result = result .. tab .. tostring(k) .. " = " .. serialize(v, l + 1) .. ",\n"
        end
    end
    for _, v in ipairs(sorted_indizes(t, l + 1)) do
        error("array indizes in table: " .. tostring(v))
    end
    return result .. string.rep("    ", l - 1) .. "}"
end

function serialize_sorted_map(t, l, cb)
    if not cb then
        cb = serialize
    end
    local tab = string.rep("    ", l)
    local result = "{\n"
    for _, k in ipairs(sorted_keys(t)) do
        local v = t[k]
        result = result .. tab .. tostring(k) .. " = " .. cb(v, l + 1) .. ",\n"
    end
    for _, v in ipairs(sorted_indizes(t, l + 1)) do
        error("array indizes in table: " .. tostring(v))
    end
    return result .. string.rep("    ", l - 1) .. "}"
end

function serialize_enum(t, l)
	if not t.values then
		return serialize_array(t, l)
	end
	return serialize_array(t.values, l)
end

function serialize_array(t, l, cb)
    if not cb then
        cb = serialize
    end
    local tab = string.rep("    ", l)
    local result = "{\n"
    for _, v in ipairs(t) do
        result = result .. tab .. cb(v, l + 1) .. ",\n"
    end
    for _, k in ipairs(sorted_keys(t)) do
        error("keys are inside the array: " .. tostring(k))
    end
    return result .. string.rep("    ", l - 1) .. "}"
end

function serialize_enums(t)
    return serialize_sorted_map(t, 1, function(a, l) return serialize_enum(a, l) end)
end

function serialize_global_functions(t)
    return serialize_sorted_map(t, 1, function(a, l) return serialize_function(a, l) end)
end

function serialize_classes(t)
    return serialize_sorted_map(t, 1, function(a, l) return serialize_class(a, l) end)
end

dofile("lua/class_definitions.lua")
for class_name, value in pairs(classes) do
    if not value.new then
        value.new = { }
    end
end

print("classes = " .. serialize_classes(classes))
print("global_functions = " .. serialize_global_functions(global_functions))
print("enums = " .. serialize_enums(enums))
