--dofile("./class_definitions.lua")

outdated_metatable = {
    __index = function(userdata, key)
        error("Attempt to access outdated gamedata.")
    end,

    __newindex = function(table, key, value)
        error("Attempt to access outdated gamedata.")
    end
}

-- table containing our mods
mods = { }

function mod_callback(callback_name, ...)
    -- global unpack function has been moved to table in Lua 5.3
    local unpack = table.unpack or unpack
    local t = {...}
    for modname, mod_instance in pairs(mods) do
        if type(mod_instance[callback_name]) == "function" then
            mod_instance[callback_name](unpack(t))
        end
    end
end

function resolve_name(name)
    local a = _G
    for key in string.gmatch(name, "([^%.]+)(%.?)") do
        if a[key] then
            a = a[key]
        else
            return nil
        end
    end
    return a
end

function function_exists(name)
    return type(resolve_name(name)) == 'function'
end

function table_length(T)
  local count = 0
  if (T ~= nil) then
      for _ in pairs(T) do
        count = count + 1
      end
  end
  return count
end
