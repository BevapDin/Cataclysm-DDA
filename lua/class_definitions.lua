--[[
Defines which attributes are exported by our C++ classes. The required C functions (used as
callbacks from Lua) are automatically generated by src/lua/generate_bindings.lua

This file is included after generated_class_definitions.lua is included. That file defines the
`classes` and `enums` tables, which contains all classes and enumerations that will be
accessible in Lu.

Both tables can be modified here. Afterward they are used by generate_bindings.lua to generate
the C++ wrapper code.

The key is the name
of the class in C++. Keep in mind that it has to be a valid Lua identifiers, you can add a
typedef in catalua.cpp, e.g. `using cppstring = std::string;`, than add a class with the name
`cppstring` here. It maps to a table with the following values:

Each class requires at least the attributes and the functions table (they can be empty).
Optional values are:
- by_value (boolean, default: false): if true, copy the C++ object into memory managed by Lua
  (the copy may outlive the source C++ object), otherwise only a pointer to the C++ object
  is stored in Lua and it *needs* to stay valid in C++ until the Lua object is gone.
- by_value_and_reference (boolean, default: false): if true, the class is can be exported to Lua
  as value (copy of the object, managed by Lua) *and* as reference (to an object managed by C++
  code). This flag implies "by_value", "by_value" should therefor not be specified explicitly.
- has_equal (boolean, default: false): If true, generate the __eq entry in the metatable which
  will map to the C++ using the operator==.
- new (an array of parameter lists): defines the constructor of the object. This is only useful for
  by_value objects, it allows to create an instance of it in Lua. The entry should be an array,
  each element of it represents one overload of the constructor. Each element should be a list of
  parameters to those overloads (same as the list of arguments to member functions).
- int_id (optional, a string): if the class has an associated int_id (e.g. ter_t has int_id<ter_t>,
  which is typedefed to ter_id), this can be used to define that int_id (for ter_t is should be
  "ter_id"). At the end of this file, this will be used to create an actual entry in the classes
  table for the type name given here.
  This is done because all the int_id objects have essentially the same functions.

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

Regarding reference and the by_value setting:
Instances of the itype class are created by C++ (specifically by the Item_factory). They are
never created elsewhere and once created each instance will stay valid (and at the same address)
until the game has ended. Furthermore it's actually important that each item id maps to exactly
one instance of itype. Similar for mtype.
Therefor those objects are exported as pointer only (by_value is false).

Other objects (e.g. tripoint) can be constructed as values in Lua (by_value is true).

The return value of functions ("rval") behaves a bit special:
- If it ends with a '&', as in `rval = "item&"`, it is assumed the function returns a reference.
  The by_value setting will determine whether to copy the referred object (to Lua memory), or
  to store only a reference in Lua memory.
  Using '&' on native Lua types is not allowed.
- If the return type is a class type (has an entry in classes), it is copied to Lua memory. This
  will fail (when the generated bindings are compiled or when they get linked) if the type does not
  support copying. One should either add `by_value = true` or `by_value_and_reference = true` to the type.
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

global_functions = {
    add_msg = {
        cpp_name = "add_msg_wrapper",
        args     = { "std::string" },
        argnames = { "message" },
        rval = nil,
        desc = "Write a message to the game's standard message window."
    },
    popup = {
        cpp_name = "popup_wrapper",
        args = { "std::string" },
        rval = nil
    },
    string_input_popup = {
        cpp_name = "string_input_popup_wrapper",
        args = { "std::string", "int", "std::string" },
        rval = "std::string"
    },
    create_uimenu = {
        cpp_name = "create_uimenu",
        args = {},
        rval = "uimenu&"
    },
    get_terrain_type = {
        cpp_name = "get_terrain_type",
        args = {"int"},
        rval = "ter_t&"
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
        args = { "std::string", "std::string", "int" },
        rval = "bool"
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
    }
}

-- This adds the int_id wrappers from the class definition as real classes.
-- All int_id<T>s have the same interface, so we only need to add some mark to T, that this class
-- T has an int_id of some name.
-- In the class definition: add "int_id" = "XXX" (XXX is the typedef id that is used by C++).
new_classes = {}
for name, value in pairs(classes) do
    if value.int_id then
        -- This is the common int_id<T> interface:
        local t = {
            by_value = true,
            has_equal = true,
            -- IDs *could* be constructed from int, but where does the Lua script get the int from?
            -- The int is only exposed as int_id<T>, so Lua should never know about it.
            -- Copy and default constructor
            new = { { value.int_id }, { } },
            functions = {
                -- Use with care, only for displaying the value for debugging purpose!
                { name = "to_i", rval = "int", args = { } },
                { name = "obj", rval = name .. "&", args = { } },
            }
        }
        if value.string_id then
            -- Allow conversion from int_id to string_id
            t[#t.functions] = { name = "id", rval = value.string_id, args = { } }
            -- And creation of an int_id from a string_id
            t.new = { { value.string_id }, { } }
        end
        new_classes[value.int_id] = t
    end
    -- Very similar to int_id above
    if value.string_id then
        local t = {
            by_value = true,
            has_equal = true,
            -- Copy and default constructor and construct from plain string.
            new = { { value.string_id }, { }, { "std::string" } },
            functions = {
                { name = "str", rval = "std::string", args = { } },
                { name = "is_valid", rval = "bool", args = { } },
                { name = "obj", rval = name .. "&", args = { } },
            }
        }
        if value.int_id then
            t.functions[#t.functions] = { name = "id", rval = value.int_id, args = { } }
        end
        new_classes[value.string_id] = t
    end
end
for name, value in pairs(new_classes) do
    classes[name] = value
end
new_classes = nil
