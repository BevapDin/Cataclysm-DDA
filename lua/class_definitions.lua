--[[
Defines which attributes are exported by our C++ classes. The required C functions (used as
callbacks from Lua) are automatically generated by src/lua/generate_bindings.lua

The classes table contains all classes that will be accessible in Lua. The key is the name
of the class in C++. Keep in mind that it has to be a valid Lua identifiers, you can add a
typedef in catalua.cpp, e.g. `using cppstring = std::string;`, than add a class with the name
`cppstring` here. It maps to a table with the following values:

Each class requires at least the attributes and the functions table (they can be empty).
Optional values are:
- has_equal (boolean, default: false): If true, generate the __eq entry in the metatable which
  will map to the C++ using the operator==.
- new (an array of parameter lists): defines the constructor of the object. The entry should be an array,
  each element of it represents one overload of the constructor. Each element should be a list of
  parameters to those overloads (same as the list of arguments to member functions).
- code_prepend (optional, a string), arbitrary code that is required to compile the generated
  wrapper functions for this type. This usually contains include statements for at least the
  header for the type itself, but may require more includes in order to (implicitly) call
  copy constructors when calling member functions. If a header is missing, you'll get a compiler
  error.
- forward_declaration (optional, a string), how to forward declare the type. E.g. "class foo;"
  (for simple types) or (for template instances):
  "template<typename N> class wrapper;class foo;using foo_wrapper = wrapper<foo>;"
- output_path (optional, a string), the path where to write the generated C++ code. The default
  is based on the class name. Several classes can have the same output path, the generated
  content is put together in that file.

The attributes table contains the members of the C++ class. Each key is the name of the member,
it maps to a map with the following values:
- cpp_name (defaults to the name of the member): an alternatively name for the member,
  used only in the C++ part of the wrapper.
- writable (boolean, default: false): if true, a wrapper to set the member is generated (otherwise
  only the getter is generated and the member is effectively read-only).
- type (required): the type of the member (see below).

The functions table contains the member functions of the C++ class. Each entry (no keys, it's an
array) should have the following values:
- name (required, string): the name of the function as it appears in Lua.
- cpp_name (defaults to the name of the function, string): an alternatively name for the function,
  used only in the C++ part of the wrapper.
- args (an array of strings): the types (see below) of the parameters to the function (in order).
- rval (can be nil when the function returns void): the type (see below) that the function returns.

The global_functions table is basically the same as the class functions table, but it has
(currently) the function name as key of the table (this disallows function overloading, and should
be considered a bug). TODO: fix it

Types can be built in types: int, float, bool, string (std::string in C++), cstring (const char* in C++)
Or any class defined in the classes table.

The return value of functions ("rval") behaves a bit special:
- If it ends with a '&', as in `rval = "item&"`, it is assumed the function returns a reference and
  the reference is stored in Lua. Note that the C++ function may return a reference, but the Lua
  wrappers may handle it as a value.
  Using '&' on native Lua types is not allowed.
- If the return type is a class type (has an entry in classes), it is copied to Lua memory. This
  will fail (when the generated bindings are compiled or when they get linked) if the type does not
  support copying.
- Otherwise it should be a native Lua type, which will be copied.

Example: (Creature::pos returns a const reference to tripoint, game::zombie returns a reference to monster)
`local m = game.zombie(0)` stores a pointer to the monster returned by `game::zombie(0)`. This reference
must not be used after the monster has been removed from the game (e.g. because it died). Calling
the function again will give the very same pointer:
`local n = game.zombie(0)`
`m.setpos(tripoint(10,0,0))` This also affects `n` because `m` and `n` are pointers to the very
same C++ object.
`local p = some_monster:pos()` stores a copy of the result of `some_monster.pos()` in `p`. This
variable can be used long after `some_monster` has been killed and removed from the game. It has
no connection at all to the monster.
--]]

Class = {
}
Class.__index = Class
function register_class(name, data)
    setmetatable(data, Class)
    data.name = name
    classes[name] = data
end

function Class:get_output_path()
    if self.output_path then
        return self.output_path
    end
    return output_path_for_id(self.name)
end

function Class:get_code_prepend()
    if self.code_prepend then
        return self.code_prepend
    end
    return ""
end

function Class:get_cpp_name()
    if self.cpp_name then
        return self.cpp_name
    end
    return self.name
end

function Class:get_forward_declaration()
    if self.forward_declaration then
        return self.forward_declaration
    end
    -- @todo could be a struct!
    return "class " .. self:get_cpp_name() .. ";"
end

Enum = {
}
Enum.__index = Enum
function register_enum(name, data)
    setmetatable(data, Enum)
    data.name = name
    enums[name] = data
end

function Enum:get_output_path()
    if self.output_path then
        return self.output_path
    end
    return output_path_for_id(self.name)
end

function Enum:get_code_prepend()
    if self.code_prepend then
        return self.code_prepend
    end
    return ""
end

function Enum:get_cpp_name()
    if self.cpp_name then
        return self.cpp_name
    end
    return self.name
end

function Enum:get_forward_declaration()
    if self.forward_declaration then
        return self.forward_declaration
    end
    -- @todo could be different underlying type
    return "enum " .. self:get_cpp_name() .. " : int;"
end

-- Yields the `output_path` of the entity (class/enum) of the given name.
function output_path_of(name)
    if classes[name] then
        return classes[name]:get_output_path()
    end
    if enums[name] then
        return enums[name]:get_output_path()
    end
    error(name .. " is not a class/enum name")
end

-- Yields the `code_prepend` of the entity (class/enum) of the given name.
function code_prepend_of(name)
    if classes[name] then
        return classes[name]:get_code_prepend()
    end
    if enums[name] then
        return enums[name]:get_code_prepend()
    end
    error(name .. " is not a class/enum name")
end

-- Yields the `code_prepend` of the entity (class/enum) of the given name.
function cpp_name_of(name)
    if classes[name] then
        return classes[name]:get_cpp_name()
    end
    if enums[name] then
        return enums[name]:get_cpp_name()
    end
    error(name .. " is not a class/enum name")
end

-- Yields the `forward_declaration` of the entity (class/enum) of the given name.
function forward_declaration_of(name)
    if classes[name] then
        return classes[name]:get_forward_declaration()
    end
    if enums[name] then
        return enums[name]:get_forward_declaration()
    end
    error(name .. " is not a class/enum name")
end

function ref_or_val(t)
    if classes[t] then
        return t .. '&'
    else
        return t
    end
end

function output_path_for_id(id)
    return id:gsub("[^%w_.]", "") .. ".gen.cpp"
end

-- Adds the declaration for an C++ iterator class to the exported classes.
-- @param container_type The C++ type id of the container class.
-- @param element_type The C++ type of the container elements. This type must be exported
-- as well. Dereferencing the iterator will give a reference to that type.
function make_std_iterator_class(container_type, element_type)
    local iterator_type = container_type .. '::iterator'
    register_class(iterator_type, {
        has_equal = true,
        -- @todo check what other members are needed
        output_path = output_path_of(element_type),
        new = {
            { iterator_type },
        },
        attributes = {
        },
        functions = {
            { name = "elem", rval = ref_or_val(element_type), cpp_name = "operator*", args = { } },
            { name = "inc", rval = nil, cpp_name = "operator++", args = { } },
        },
    } )
    -- @todo maybe add more data to this?
    return iterator_type
end

function make_std_list_class(element_type)
    local container_type = "std::list<" .. element_type .. ">"
    local iterator_type = make_std_iterator_class(container_type, element_type)
    register_class(container_type, {
        -- @todo check what other members are needed
        output_path = output_path_of(element_type),
        code_prepend = code_prepend_of(element_type) .. "\n#include <list>",
        new = {
            { },
            { container_type },
        },
        attributes = {
        },
        functions = {
            { name = "size", rval = "int", args = { } },
            { name = "erase", rval = iterator_type, args = { iterator_type } },
            { name = "push_back", rval = nil, args = { element_type } },
            { name = "insert", rval = nil, args = { iterator_type, element_type } },
            { name = "cppbegin", rval = iterator_type, cpp_name = "begin", args = { } },
            { name = "cppend", rval = iterator_type, cpp_name = "end", args = { } },
        },
    } )
    -- @todo maybe add more data to this?
    return container_type
end

function make_std_vector_class(element_type)
    local container_type = "std::vector<" .. element_type .. ">"
    local iterator_type = make_std_iterator_class(container_type, element_type)
    register_class(container_type, {
        -- @todo check what other members are needed
        output_path = output_path_of(element_type),
        code_prepend = code_prepend_of(element_type) .. "\n#include <vector>",
        new = {
            { },
            { container_type },
        },
        attributes = {
        },
        functions = {
            { name = "at", rval = element_type, args = { "int" } },
            { name = "size", rval = "int", args = { } },
            { name = "erase", rval = iterator_type, args = { iterator_type } },
            { name = "push_back", rval = nil, args = { element_type } },
            { name = "insert", rval = nil, args = { iterator_type, element_type } },
            { name = "cppbegin", rval = iterator_type, cpp_name = "begin", args = { } },
            { name = "cppend", rval = iterator_type, cpp_name = "end", args = { } },
        },
    } )
    -- @todo maybe add more data to this?
    return container_type
end

function make_std_set_class(element_type)
    local container_type = "std::set<" .. element_type .. ">"
    local iterator_type = make_std_iterator_class(container_type, element_type)
    register_class(container_type, {
        -- @todo check what other members are needed
        output_path = output_path_of(element_type),
        code_prepend = code_prepend_of(element_type) .. "\n#include <set>",
        new = {
            { },
            { container_type },
        },
        attributes = {
        },
        functions = {
            { name = "size", rval = "int", args = { } },
            { name = "erase", rval = iterator_type, args = { iterator_type } },
            { name = "insert", rval = nil, args = { element_type } },
            { name = "count", rval = "int", args = { element_type } },
            { name = "cppbegin", rval = iterator_type, cpp_name = "begin", args = { } },
            { name = "cppend", rval = iterator_type, cpp_name = "end", args = { } },
        },
    } )
    -- @todo maybe add more data to this?
    return container_type
end

-- This adds the int_id wrappers from the class definition as real classes.
-- All int_id<T>s have the same interface, so we only need to add some mark to T, that this class
-- T has an int_id of some name.
-- In the class definition: add "int_id" = "XXX" (XXX is the typedef id that is used by C++).
function make_id_classes(class_name, int_id_name, string_id_name)
    local class = classes[class_name]
    if int_id_name and not classes[int_id_name] then
        -- This is the common int_id<T> interface:
        local t = {
            forward_declaration = class:get_forward_declaration() .. "using " .. int_id_name .. " = int_id<" .. class:get_cpp_name() .. ">;",
            code_prepend = class:get_code_prepend(),
            output_path = class:get_output_path(),
            has_equal = true,
            -- IDs *could* be constructed from int, but where does the Lua script get the int from?
            -- The int is only exposed as int_id<T>, so Lua should never know about it.
            attributes = { },
            -- Copy and default constructor
            new = { { int_id_name }, { } },
            functions = {
                -- Use with care, only for displaying the value for debugging purpose!
                { name = "to_i", rval = "int", args = { } },
                { name = "obj", rval = class_name .. "&", args = { } },
            }
        }
        if string_id_name then
            -- Allow conversion from int_id to string_id
            t[#t.functions] = { name = "id", rval = string_id_name, args = { } }
            -- And creation of an int_id from a string_id
            t.new = { { string_id_name }, { } }
        end
        register_class( int_id_name, t )
    end
    -- Very similar to make_int_id above
    if string_id_name and not classes[string_id_name] then
        local t = {
            forward_declaration = class:get_forward_declaration() .. "using " .. string_id_name .. " = string_id<" .. class:get_cpp_name() .. ">;",
            code_prepend = class:get_code_prepend(),
            output_path = class:get_output_path(),
            has_equal = true,
            -- Copy and default constructor and construct from plain string.
            new = { { string_id_name }, { }, { "string" } },
            attributes = { },
            functions = {
                { name = "str", rval = "string", args = { } },
                { name = "is_valid", rval = "bool", args = { } },
                { name = "obj", rval = class_name .. "&", args = { } },
            }
        }
        if int_id_name then
            t.functions[#t.functions] = { name = "id", rval = int_id_name, args = { } }
        end
        register_class( string_id_name, t )
    end
end

function dofile_if_exists(f)
    local o = io.open(f, "r")
    if o ~= nil then
        io.close(o)
        dofile(f)
    end
end

classes = {}
enums = {}
-- Both are the same file, but this script might get run from the root directory
-- (the first path would work) or from within "src/lua" (the second path would work).
-- @todo fix this
dofile_if_exists("lua/generated_class_definitions.lua")
dofile_if_exists("../../lua/generated_class_definitions.lua")

register_class("item_stack_iterator", {
    has_equal = true,
    cpp_name = "std::list<item>::iterator",
    code_prepend = "#include \"item.h\"\n#include <list>",
    new = {
        { "item_stack_iterator" },
    },
    attributes = {
    },
    functions = {
        { name = "inc", cpp_name = "operator++", rval = nil, args = { } },
        { name = "elem", cpp_name = "operator*", rval = "item&", args = { } },
    },
} )

register_class("volume", {
    cpp_name = "units::volume",
    code_prepend = "#include \"units.h\"",
    output_path = "units.gen.cpp",
    attributes = {
        },
    functions = {
        { name = "value", rval = "int", args = { } },
    },
} )

register_class("mass", {
    cpp_name = "units::mass",
    code_prepend = "#include \"units.h\"",
    output_path = "units.gen.cpp",
    attributes = {
    },
    functions = {
        { name = "value", rval = "int", args = { } },
    },
} )

classes["ter_t"].output_path = "mapdata.gen.cpp"
classes["furn_t"].output_path = "mapdata.gen.cpp"

-- Headers that are required in order to compile the global functions wrapper
global_functions_code_prepend = "#include \"field.h\"\n#include \"bodypart.h\"\n#include \"itype.h\"\n#include \"creature.h\"\n#include \"output.h\"\n#include \"calendar.h\"\n#include \"pldata.h\"\n#include \"units.h\""

global_functions = {
    add_msg = {
        cpp_name = "add_msg_wrapper",
        args     = { "string" },
        argnames = { "message" },
        rval = nil,
        desc = "Write a message to the game's standard message window."
    },
    query_yn = {
        cpp_name = "query_yn_wrapper",
        args     = { "string" },
        argnames = { "message" },
        rval = "bool"
    },
    popup = {
        cpp_name = "popup_wrapper",
        args = { "string" },
        rval = nil
    },
    string_input_popup = {
        cpp_name = "string_input_popup_wrapper",
        args = { "string", "int", "string" },
        rval = "string"
    },
    rng = {
        cpp_name = "rng",
        args = {"int", "int"},
        rval = "int"
    },
    one_in = {
        cpp_name = "one_in",
        args = {"int"},
        rval = "bool"
    },
    distance = {
        cpp_name = "rl_dist",
        args = {"int", "int", "int", "int"},
        rval = "int"
    },
    trig_dist = {
        cpp_name = "trig_dist",
        args = {"int", "int", "int", "int"},
        rval = "int"
    },
    add_item_to_group = {
        cpp_name = "item_controller->add_item_to_group",
        args = { "string", "string", "int" },
        rval = "bool"
    },
    get_monster_at = {
        cpp_name = "get_monster_at",
        args = { "tripoint" },
        rval = "monster&",
        desc = "Returns a reference to monster at given tripoint, *or* nil if there is no monster."
    },
    get_critter_at = {
        cpp_name = "get_critter_at",
        args = { "tripoint" },
        rval = "Creature&",
        desc = "Returns a reference to creature at given tripoint, *or* nil if there is no creature."
    },
    get_npc_at = {
        cpp_name = "g->critter_at<npc>",
        args = { "tripoint" },
        rval = "npc&",
        desc = "Returns a reference to npc at given tripoint, *or* nil if there is no npc."
    },
    create_monster = {
        cpp_name = "create_monster",
        args = { "mtype_id", "tripoint" },
        rval = "monster&",
        desc = "Creates and spawns a new monster of given type. Returns a refernce to it, *or* nil if it could not be spawned."
    },
    get_calendar_turn = {
        cpp_name = "get_calendar_turn_wrapper",
        args = {},
        rval = "calendar&"
    },
    get_time_duration = {
        cpp_name = "get_time_duration_wrapper",
        args = { "int" },
        rval = "time_duration",
        desc = "Constructs `time_duration` with given `int` value (which is number of turns). You can also use TURNS(n), MINUTES(n), HOURS(n) and DAYS(n) wrapper functions from `autoexec.lua`."
    },
	-- Returns id of overmap terrain on given overmap with given tripoint in global overmap terrain coordinates.
	-- Use `game.get_omt_id (g:get_cur_om(), player:global_omt_location())` to return id of overmap terrain of current player location.
    get_omt_id = {
        cpp_name = "get_omt_id",
        args = { "overmap", "tripoint" },
        rval = "string"
    },
	-- Returns enum, indicating direction of overmap terrain on given overmap with given tripoint in global overmap terrain coordinates.
	-- Possible return values are in `overmap_direction` in `enums` section above.
	-- Use `game.get_omt_dir (g:get_cur_om(), player:global_omt_location())` to return direction of overmap terrain of current player location.
    get_omt_dir = {
        cpp_name = "get_omt_dir",
        args = { "overmap", "tripoint" },
        rval = "overmap_direction"
    }
}

function table_unpack_wrapper(args)
    if args then
        if table.unpack then
            return table.unpack(args)
        elseif unpack then
            return unpack(args)
        else
            return nil
        end
    else
        return nil
    end
end

-- This extracts optional arguments.
-- Example:
--     { name = "add_effect", rval = nil, args = { "efftype_id", "time_duration" }, optional_args = { "body_part", "bool", "int", "bool" } },
-- As a result of previous definition is extracted, forrowing definitions is gained.
--     { name = "add_effect", rval = nil, args = { "efftype_id", "time_duration", "body_part" } },
--     { name = "add_effect", rval = nil, args = { "efftype_id", "time_duration", "body_part", "bool" } },
--     { name = "add_effect", rval = nil, args = { "efftype_id", "time_duration", "body_part", "bool", "int" } },
--     { name = "add_effect", rval = nil, args = { "efftype_id", "time_duration", "body_part", "bool", "int", "bool" } },
for class_name, value in pairs(classes) do
    local new_functions = { }
    for _, func in ipairs(value.functions) do
        if func.optional_args then
            local i = 0
            while i <= #func.optional_args do
                local t = {
                    name = func.name,
                    rval = func.rval,
                    args = { table_unpack_wrapper(func.args) } -- copy args
                }
                local j = 1
                while j <= i do
                    table.insert(t.args, func.optional_args[j])
                    j = j + 1
                end
                table.insert(new_functions, t)
                i = i + 1
            end
        end
    end
    for _, new_func in ipairs(new_functions) do
        table.insert(value.functions, new_func)
    end
end
