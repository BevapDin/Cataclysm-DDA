-- Tool to automatically generate bindings for lua.
-- The bindings are generated as C++ and lua files, which are
-- then included into the cataclysm source.

-- Conventions:
-- The variable holding the name of a class is named "class_name"
-- The variable holding the data (declarations etc.) of a class is "class"
-- Example code: for class_name, class in pairs(types) do ...

-- The generated helper C++ functions use this naming system:
-- Getter: "get_" .. class_name .. "_" .. member_name
-- Setter: "set_" .. class_name .. "_" .. member_name
-- Member function: "func_" .. class_name .. "_" .. member_name
-- Operators: "op_" .. class_name .. "_" .. operator_id
-- Constructors: "new_" .. class_name
-- Global functions get a "global_" prefix.
-- This allows a data member "foo", as well as a function member "get_foo(...)".
-- They would get "get_class_foo", "set_class_foo" and "func_class_get_foo" wrappers.

-- Generate our C++ source file with all wrappers for accessing variables and functions from lua.
-------------------------------------------------------------------------------------------------
dofile "../../lua/class_definitions.lua"

local br = "\n"
local tab = "    "

function sorted_keys(t, condition)
    if not condition then
        condition = function() return true; end
    end
    local res = { }
    for k, _ in pairs(t) do
        if condition(k) then
            table.insert(res, k)
        end
    end
    table.sort(res)
    return res;
end

BuildInTypeBase = TypeBase:create()

function BuildInTypeBase:create(cpp_name)
    o = { cpp_name = cpp_name }
    setmetatable(o, self)
    self.__index = self
    return o
end
function BuildInTypeBase:get_output_path()
    return nil
end
function BuildInTypeBase:get_forward_declaration()
    return ""
end
function BuildInTypeBase:load_metatable_call()
    return ""
end

types['bool'] = BuildInTypeBase:create('bool')
types['string'] = BuildInTypeBase:create('std::string')
types['int'] = BuildInTypeBase:create('int')
types['float'] = BuildInTypeBase:create('float')

-- All values from `types`, but sorted by their name. Useful for deterministic iteration.
local sorted_types = {}
for _, name in ipairs(sorted_keys(types)) do
    table.insert(sorted_types, types[name])
end

function id_to_simple_string(id)
    return id:gsub("[^a-z0-9A-Z_]", "_")
end

-- Generic helpers to generate C++ source code chunks for use in our lua binding.
---------------------------------------------------------------------------------

-- Convert a given type such as "string" to the corresponding C++ wrapper class,
-- e.g. `LuaType<std::string>`. The wrapper class has various static functions:
-- `get` to get a value of that type from Lua stack.
-- `push` to push a value of that type to Lua stack.
-- `has` to check for a value of that type on the stack.
-- See catalua.h for their implementation.
function Class:member_type_to_cpp_type()
    return "LuaValueOrReference<" .. self:get_cpp_name() .. ">"
end
function Enum:member_type_to_cpp_type()
    return "LuaEnum<" .. self:get_cpp_name() .. ">"
end
function BuildInTypeBase:member_type_to_cpp_type()
    return "LuaType<" .. self.cpp_name .. ">"
end

function TypeBase:retrieve_lua_value(stack_position)
    return self:member_type_to_cpp_type() .. "::get(L, " .. stack_position .. ")"
end
function Class:retrieve_lua_value(stack_position)
    if self.is_script then
        return "catalua::script_reference::from_stack( " .. stack_position .. " );"
    end
    return TypeBase.retrieve_lua_value(self, stack_position)
end

-- Returns an expression that evaluates to `true` if the stack has an object of the given type
-- at the given position.
function has_lua_value(value_type, stack_position)
    if value_type:sub(-1) == "*" then
        return "LuaValueOrReference<" .. get_type(value_type:sub(1, -2)):get_cpp_name() .. ">::has_pointer(L, " .. stack_position .. ")"
    end
    return get_type(value_type):member_type_to_cpp_type() .. "::has(L, " .. stack_position .. ")"
end

-- Returns code to retrieve a lua value from the stack and store it into
-- a C++ variable
function retrieve_lua_value(value_type, stack_position)
    if value_type:sub(-1) == "*" then
        return get_type(value_type:sub(1, -2)):member_type_to_cpp_type() .. "::get_pointer(L, " .. stack_position .. ")"
    end
    local t = get_type(value_type)
    if t.is_script then
        return "catalua::script_reference::from_stack( " .. stack_position .. " );"
    end
    return get_type(value_type):retrieve_lua_value(stack_position)
end
function retrieve_lua_value_to_variable(var_name, value_type, stack_position)
    function type_to_cpp_type(t)
        if t == "bool" then return "bool"
        elseif t == "cstring" then return "const char *"
        elseif t == "string" then return "std::string"
        elseif t == "int" then return "int"
        elseif t == "float" then return "float"
        elseif t:sub(-1) == "*" and has_type(t:sub(1, -2)) then
            return get_type(t:sub(1, -2)):get_cpp_name() .. " *"
        elseif has_type(t) then
            local t = get_type(t)
            if getmetatable(t) == Class then
                return t:get_cpp_name() .. " &"
            else
                return t:get_cpp_name()
            end
        end
        error("'"..t.."' is not a build-in type and is not defined in class_definitions.lua")
    end
    return type_to_cpp_type(value_type) .. " " .. var_name .. " = " .. retrieve_lua_value(value_type, stack_position) .. ";"
end

-- Returns code to take a C++ variable of the given type and push a lua version
-- of it onto the stack.
function Class:push_lua_value(in_variable)
    return "LuaValue<" .. self:get_cpp_name() .. ">::push( L, " .. in_variable .. " );"
end
function Class:push_ref_lua_value(in_variable)
    return "LuaPointer<" .. self:get_cpp_name() .. ">::push( L, " .. in_variable .. " );"
end
function Enum:push_lua_value(in_variable)
    return "LuaEnum<" .. self:get_cpp_name() .. ">::push( L, " .. in_variable .. " );"
end
function BuildInTypeBase:push_lua_value(in_variable)
    return "LuaType<" .. self.cpp_name .. ">::push( L, " .. in_variable .. " );"
end
function push_lua_value(in_variable, value_type)
    if value_type:sub(-2) == "*&" then
        return "LuaPointer<" .. get_type(value_type:sub(1, -3)):get_cpp_name() .. ">::push( L, " .. in_variable .. " );"
    end
    if value_type:sub(-1) == "*" then
        return "LuaPointer<" .. get_type(value_type:sub(1, -2)):get_cpp_name() .. ">::push( L, " .. in_variable .. " );"
    end

    if value_type:sub(-1) == "&" then
        -- A reference is to be pushed. Copying the referred to object may not be allowed  (it may
        -- be a reference to a global game object).
        local t = get_type(value_type:sub(1, -2))
        if t.is_script then
            return "catalua::script_reference::push_on_stack( " .. in_variable .. " );"
        elseif getmetatable(t) == Class then
            return t:push_ref_lua_value(in_variable)
        else
            return t:push_lua_value(in_variable)
        end
    end
    if has_type(value_type) then
        return get_type(value_type):push_lua_value(in_variable)
    end
    -- A native Lua type.
    return member_type_to_cpp_type(value_type) .. "::push( L, " .. in_variable .. " );"
end

function generate_getter_code(name, attribute, tab)
    local cpp_name = attribute.cpp_name or name
    local cpp_output = ""
    -- adding the "&" to the type, so push_lua_value knows it's a reference.
    cpp_output = cpp_output .. tab .. "    " .. push_lua_value("instance." .. cpp_name, attribute.type .. "&") .. br
    cpp_output = cpp_output .. tab .. "    " .. "return 1;" .. br
    return cpp_output
end

function generate_setter_code(name, attribute, tab)
    local cpp_name = attribute.cpp_name or name
    local member_type = attribute.type
    local cpp_output = ""
    cpp_output = cpp_output .. tab .. "    " .. "instance." .. cpp_name .. " = " .. retrieve_lua_value(member_type, 3) .. ";" .. br
    cpp_output = cpp_output .. tab .. "    " .. "return 0;" .. br
    return cpp_output
end

-- Generates a function wrapper for a global function. "function_to_call" can be any string
-- that works as a "function", including expressions like "g->add_msg"
function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int global_"..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        -- Needs to be auto, can be a proxy object, a real reference, or a POD.
        -- This will be resolved when the functin is called. The C++ compiler will convert
        -- the auto to the expected parameter type.
        text = text .. tab .. retrieve_lua_value_to_variable( "parameter" .. i, arg, i) .. br
    end

    local func_invoc = function_to_call .. "("
    for i, arg in ipairs(args) do
        func_invoc = func_invoc .. "parameter"..i
        if next(args, i) then func_invoc = func_invoc .. ", " end
    end
    func_invoc = func_invoc .. ")"

    if rval then
        text = text .. tab .. push_lua_value(func_invoc, rval)..br
        text = text .. tab .. "return 1; // 1 return values"..br
    else
        text = text .. tab .. func_invoc .. ";"..br
        text = text .. tab .. "return 0; // 0 return values"..br
    end
    text = text .. "}"..br

    return text
end

--[[
To allow function overloading, we need to create a function that somehow detects the input
types and calls the matching C++ overload.
First step is to restructure the input: instead of
functions = {
    { name = "f", ... },
    { name = "f", ... },
    { name = "f", ... },
}
we want a decision tree. The leafs of the tree are the rval entries (they can be different for
each overload, but they do not affect the overload resolution).
Example: functions = {
    r = { rval = "int", ... }
    "string" = {
        r = { rval = "int", ... }
    },
    "int" = {
        r = { rval = "bool", ... }
        "int" = {
            r = { rval = "void", ... }
        },
        "string" = {
            r = { rval = "int", ... }
        }
    }
}
Means: `int f()` `int f(string)` `bool f(int)` void f(int, int)` `int f(int, string)`
A table under the index 'r' is a leaf (this assumes no type of name 'r' is ever used).
The leafs have the following entries:
- rval: the return value of the function (can be nil).
- cpp_name: the name of the C++ function to call (can be different from the Lua name of the function).
- class_name: the name of the C++ class name that contains the function (optional, only present if
  the function is a class member).
--]]
function generate_overload_tree(types)
    function generate_overload_path(root, args)
        for _, arg in pairs(args) do
            if not root[arg] then
                root[arg] = { }
            end
            root = root[arg]
        end
        return root
    end
    -- Input is a list of function declarations (multiple entries with the same name are
    -- possible). Result is a table with one entry (the overload resolution tree) for each
    -- function name.
    function convert_function_list_to_tree_list(function_list, class_name)
        if not function_list then
            return { }
        end
        local functions_by_name = { }
        for _, func in ipairs(function_list) do
            if not func.name then
                error("Every function of " .. class_name .. " needs a name, doesn't it?")
            end
            -- Create the table now. In the next loop below, we can simply assume it already exists
            functions_by_name[func.name] = {}
        end
        -- This creates the mentioned tree: each entry has the key matching the parameter type,
        -- and the final table (the leaf) has a `r` entry.
        for _, func in ipairs(function_list) do
            local base = functions_by_name[func.name]
            if not func.static then
                -- non-static functions have an implicit first argument `this` of type class_name
                if not base[class_name] then
                    base[class_name] = { }
                end
                base = base[class_name]
            end
            local leaf = generate_overload_path(base, func.args)
            leaf.r = { rval = func.rval, cpp_name = func.cpp_name or func.name, class_name = class_name, static = func.static }
        end
        return functions_by_name
    end

    for class_name, value in pairs(types) do
        if value.functions then
            value.functions = convert_function_list_to_tree_list(value.functions, class_name)
        end
        if value.new then
            local new_root = {}
            for _, func in ipairs(value.new) do
                local leaf = generate_overload_path(new_root, func)
                leaf.r = { rval = nil, cpp_name = value:get_cpp_name() .. "::" .. value:get_cpp_name(), class_name = class_name }
            end
            value.new = new_root
        end
    end
end

--[[
This (recursive) function handles function overloading:
- function_name: actual function name (only for showing an error message)
- args: tree of argument (see generate_overload_tree)
- indentation: level of indentation of the source code (only used for nice formatting)
- stack_index: index (1-based, because this is Lua) of the currently handled argument.
- cbc: callback that returns the C++ code that actually calls the C++ function.
  Its parameters:
  - indentation: level of indentation it should use
  - stack_index: number of parameters it can use (C++ variables parameter1, parameter2 and so on)
  - data: some data associated with this particular overload. See documentation of
    leafs of the decision tree generated by `generate_overload_tree`.
--]]
function insert_overload_resolution(function_name, args, cbc, indentation, stack_index)
    local ind = string.rep("    ", indentation)
    local text = ""
    -- Number of choices that can be made for function overloading
    local count = 0
    for _ in pairs(args) do count = count + 1 end
    local more = (count ~= 1)
    -- If we can chose several overloads at this point (more=true), we have to add if statements
    -- and have to increase the indentation inside of them.
    -- Otherwise (no choices), we keep everything at the same indentation level.
    -- (e.g. no overload at all, or this parameter is the same for all overloads).
    local nsi = stack_index + 1 -- next stack_index
    local ni = more and (indentation + 1) or indentation -- next indentation
    local mind = more and (ind .. tab) or ind -- more indentation
    local valid_types = "" -- list of acceptable types, for the error message
    for _, arg_type in ipairs(sorted_keys(args)) do
        local more_args = args[arg_type]
        if arg_type == "r" then
            -- handled outside this loop
        else
            if more then
                text = text..ind.."if("..has_lua_value(arg_type, nsi)..") {"..br
            end
            -- Needs to be auto, can be a proxy object, a real reference, or a POD.
            -- This will be resolved when the functin is called. The C++ compiler will convert
            -- the auto to the expected parameter type.
            text = text..mind..retrieve_lua_value_to_variable("parameter"..stack_index, arg_type, nsi)..br
            text = text..insert_overload_resolution(function_name, more_args, cbc, ni, nsi)
            if more then
                text = text..ind.."}"..br
            end
            valid_types = valid_types.." or "..arg_type
        end
    end
    -- An overload can be called at this level, all required parameters are already extracted.
    if args.r then
        if more then
            text = text .. ind .. "if(lua_gettop(L) == "..stack_index..") {"..br
        end
        -- If we're here, any further arguments will ignored, so raise an error if there are any left over
        text = text..mind.."if(lua_gettop(L) > "..stack_index..") {"..br
        text = text..mind..tab.."throw std::runtime_error( \"Too many arguments to "..function_name..", expected only "..stack_index..", got \" + std::to_string( lua_gettop( L ) ) );"..br
        text = text..mind.."}"..br
        text = text .. cbc(ni, stack_index - 1, args.r)
        if more then
            text = text .. ind .. "}"..br
        end
        valid_types = valid_types .. " or nothing at all"
    end
    -- If more is false, there was no branching (no `if` statement) generated, but a return
    -- statement had already been made, so this error would be unreachable code.
    if more then
        valid_types = valid_types:sub(5) -- removes the initial " or "
        text = text .. ind .. "throw std::runtime_error( \"Unexpected type, expected are "..valid_types.."\" );"..br
    end
    return text
end

-- Generate a wrapper around a class function(method) that allows us to call a method of a specific
-- C++ instance by calling the method on the corresponding lua wrapper, e.g.
-- monster:name() in lua translates to monster.name() in C++
function Class:generate_function_wrapper(function_name, func)
    local class_name = self.name
    local text = "static int func_" .. id_to_simple_string(class_name) .. "_" .. function_name .. "(lua_State *L) {"..br

    local cbc = function(indentation, stack_index, data)
        local tab = string.rep("    ", indentation)

        local func_invoc
        --[[
        The first parameter is implicitly the `this` object. The static case is require for:
        a) parameter0 is a proxy object with a conversion to `T&` operator that is invoked by the cast.
        b) If we call a function of the parent class, we need to call it through
        a reference to the parent class, otherwise we get into trouble with default parameters:
        Example:
        class A {     void f(int = 0); }
        class B : A { void f(int); }
        This won't work: B b; b.f();
        But this will:   B b; static_cast<A&>(b).f()
        --]]
        -- The actual C++ function parameters start at different stack index:
        -- Static: `foo::func(parameter0, parameter1, parameter2, ...)`
        -- Non-static: `parameter0.func(parameter1, parameter2, ...)`
        local start_index
        if data.static then
            func_invoc = get_type(data.class_name):get_cpp_name() .. '::' .. data.cpp_name .. '('
            start_index = 0
        else
            func_invoc = "static_cast<"..get_type(data.class_name):get_cpp_name().."&>( parameter0 )"
            func_invoc = func_invoc .. "."..data.cpp_name .. "("
            start_index = 1
        end

        for i = start_index,stack_index do
            func_invoc = func_invoc .. "parameter"..i
            if i < stack_index then func_invoc = func_invoc .. ", " end
        end
        func_invoc = func_invoc .. ")"

        local text
        if data.rval then
            text = tab .. push_lua_value(func_invoc, data.rval)..br
            text = text .. tab .. "return 1; // 1 return values"..br
        else
            text = tab .. func_invoc .. ";"..br
            text = text .. tab .. "return 0; // 0 return values"..br
        end
        return text
    end

    text = text .. insert_overload_resolution(function_name, func, cbc, 1, 0)

    text = text .. "}"..br

    return text
end

function Class:generate_constructor()
    local cpp_name = self:get_cpp_name()
    local class_name = self.name
    local text = "static int new_" .. id_to_simple_string(class_name) .. "(lua_State *L) {"..br

    local cbc = function(indentation, stack_index, data)
        local tab = string.rep("    ", indentation)

        local text = tab .. "LuaValue<" .. cpp_name .. ">::push(L"

        for i = 1,stack_index do
            text = text .. ", parameter"..i
        end

        text = text .. ");"..br
        text = text .. tab .. "return 1; // 1 return values"..br
        return text
    end

    text = text .. insert_overload_resolution(cpp_name .. "::" .. cpp_name, self.new, cbc, 1, 1)

    text = text .. "}"..br

    return text
end

function Class:generate_destructor()
    local cpp_output = ""
    local cpp_class_name = self:get_cpp_name()
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "void LuaValue<" .. cpp_class_name .. ">::call_destructor( " .. cpp_class_name .. " &object ) {" .. br
    -- This avoids problems where cpp_class_name is actually "foo::bar<some>" and the destructor call
    -- would be `object.~foo::bar<some>`, which causes compiler errors.
    cpp_output = cpp_output .. tab .. "using T = " .. cpp_class_name .. ";" .. br
    cpp_output = cpp_output .. tab .. "object.~T();" .. br
    cpp_output = cpp_output .. "}" .. br
    return cpp_output
end

function Class:generate_operator(operator_id, cppname)
    local cpp_class_name = self:get_cpp_name()
    local class_name = self.name
    local text = "static int op_" .. id_to_simple_string(class_name) .. "_" .. operator_id .. "(lua_State *L) {"..br

    text = text .. tab .. "const " .. cpp_class_name .. " &lhs = " .. retrieve_lua_value(class_name, 1) .. ";"..br
    text = text .. tab .. "const " .. cpp_class_name .. " &rhs = " .. retrieve_lua_value(class_name, 2) .. ";"..br
    text = text .. tab .. push_lua_value("( &lhs " .. cppname .. " &rhs ) || ( lhs " .. cppname .. " rhs )", "bool")..br
    text = text .. tab .. "return 1; // 1 return values"..br
    text = text .. "}"..br

    return text
end

function Enum:get_parent()
    return nil
end

function Class:get_parent()
    if self.parent == nil then
        return nil
    end
    if type(self.parent) == "string" then
        self.parent = get_type(self.parent)
    end
    return self.parent
end

function Class:get_children()
    if not self.children then
        self.children = { }
        for _, t in ipairs(sorted_types) do
            if getmetatable(t) == Class then
                if t:get_parent() == self then
                    for _, c in ipairs(t:get_children()) do
                        table.insert(self.children, c)
                    end
                    table.insert(self.children, t)
                end
            end
        end
    end
    return self.children
end

generate_overload_tree(types)

function Class:generate_accessors(attributes, class_name)
    local cpp_output = ""
    -- Generate getters and setters for our player attributes.
    for _, key in ipairs(sorted_keys(attributes)) do
        local attribute = attributes[key]
        cpp_output = cpp_output .. generate_getter(class_name, key, attribute.type, attribute.cpp_name or key)
        if attribute.writable then
            cpp_output = cpp_output .. generate_setter(class_name, key, attribute.type, attribute.cpp_name or key)
        end
    end
    return cpp_output
end

function Class:generate_function_wrappers(functions, cur_class_name)
    local cpp_output = ""
    for _, function_name in ipairs(sorted_keys(functions)) do
        local func = functions[function_name]
        cpp_output = cpp_output .. self:generate_function_wrapper(function_name, func, cur_class_name)
    end
    return cpp_output
end

for _, name in ipairs(global_functions) do
    local func = global_functions[name]
    cpp_output = cpp_output .. generate_global_function_wrapper(name, func.cpp_name, func.args, func.rval)
end

--[[
Merges function declaration of the a parent class into the function declarations of a derived
class. Existing declarations of the same signature are left as they are.
--]]
function merge_parent_class_functions(derived_functions, parent_functions)
    for index, func in pairs(parent_functions) do
        if index == 'r' then
            if not derived_functions.r then
                derived_functions.r = {
                    rval = func.rval,
                    static = func.static,
                    cpp_name = func.cpp_name,
                    class_name = func.class_name
                }
            end
        else
            if not derived_functions[index] then
                derived_functions[index] = { }
            end
            merge_parent_class_functions(derived_functions[index], func)
        end
    end
end

--[[
The wrapper does not export the class relationship, that means the Lua part does not
know anything about class inheritance. Lua can therefor not do the dynamic dispatch
of function calls to the parent class.
This loop inserts the functions exported by the parent class to the child class.

Example:
```C++
class Parent {
    void func();
};
class Child : public Parent {
};
```
This would produce only one wrapper function for `Parent::func`. Trying to call `func`
on a wrapped Child object would not work as there is no `Child::func` exported.
The loop copies the declaration of `Parent::func` into `Child`, which in turn triggers
exporting `Child::func`.
--]]
for class_name, class in pairs(types) do
    if class.functions then
        local derived_functions = class.functions
        while class:get_parent() do
            merge_parent_class_functions(derived_functions, class:get_parent().functions)
            class = class:get_parent()
        end
    end
end

-- luaL_Reg is the name of the struct in C which this creates and returns.
function luaL_Reg(cpp_name, lua_name)
    return tab .. '{"' .. lua_name .. '", ' .. cpp_name .. '},' .. br
end
-- Creates the LuaValue<T>::FUNCTIONS array, containing all the public functions of the class.
function Class:generate_functions_static(cpp_type)
    local class_name = self.name
    local cpp_output = ""
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const luaL_Reg " .. cpp_type .. "::FUNCTIONS[] = {" .. br
    if self.new then
        cpp_output = cpp_output .. luaL_Reg("new_" .. id_to_simple_string(class_name), "__call")
    end
    if self.has_equal then
        cpp_output = cpp_output .. luaL_Reg("op_" .. id_to_simple_string(class_name) .. "_eq", "__eq")
    end
    local class = self
    while class do
        for _, name in ipairs(sorted_keys(self.functions)) do
            cpp_output = cpp_output .. luaL_Reg("func_" .. id_to_simple_string(class_name) .. "_" .. name, name)
        end
        class = class:get_parent()
    end
    cpp_output = cpp_output .. tab .. "{NULL, NULL}" .. br -- sentinel to indicate end of array
    cpp_output = cpp_output .. "};" .. br
    return cpp_output
end
function Class:generate_accessors_impl(attributes, names, lower, upper, indentation, cbc)
    local pivot = math.floor(lower + (upper - lower) / 2)
    local name = names[pivot]
    local tab = string.rep("    ", indentation)

    local cpp_output = ""
    cpp_output = cpp_output .. tab .. "const int c = std::strcmp( name, \"" .. name .. "\" );" .. br
    if lower < pivot then
        cpp_output = cpp_output .. tab .. "if( c < 0 ) {" .. br
        cpp_output = cpp_output .. self:generate_accessors_impl(attributes, names, lower, pivot - 1, indentation + 1, cbc)
        cpp_output = cpp_output .. tab .. "}" .. br
    end

    cpp_output = cpp_output .. tab .. "if( c == 0 ) {" .. br
    cpp_output = cpp_output .. cbc(name, attributes[name], tab)

    cpp_output = cpp_output .. tab .. "}" .. br

    if upper > pivot then
        cpp_output = cpp_output .. tab .. "if( c > 0 ) {" .. br
        cpp_output = cpp_output .. self:generate_accessors_impl(attributes, names, pivot + 1, upper, indentation + 1, cbc)
        cpp_output = cpp_output .. tab .. "}" .. br
    end

    return cpp_output
end

function Class:generate_accessors(function_name, attributes, cbc)
    local names = sorted_keys(attributes, function(name) return function_name == "get_member" or attributes[name].writable; end)
    local cpp_output = ""
    local instance_type = "const " .. self:get_cpp_name()
    if function_name == "set_member" then
        instance_type = self:get_cpp_name()
    end

    cpp_output = cpp_output .. "template<>" .. br
    if #names == 0 then
        -- Without parameter names here because they are not used and
        -- this avoids warnings from the C++ compiler.
        cpp_output = cpp_output .. "int LuaValue<" .. self:get_cpp_name() .. ">::" .. function_name .. "( lua_State *, " .. instance_type .. " &, const char * ) {" .. br
    else
        cpp_output = cpp_output .. "int LuaValue<" .. self:get_cpp_name() .. ">::" .. function_name .. "( lua_State *const L, " .. instance_type .. " &instance, const char *const name ) {" .. br
        cpp_output = cpp_output .. self:generate_accessors_impl(attributes, names, 1, #names, 1, cbc)
    end
    cpp_output = cpp_output .. "    throw std::runtime_error( \"unknown attribute\" );" .. br
    cpp_output = cpp_output .. "}" .. br
    return cpp_output
end

function Class:generate_constants()
    local cpp_output = ""
    local cpp_class_name = self:get_cpp_name()
    local cpp_name = "LuaValue<" .. cpp_class_name .. ">"
    local metatable_name = self.name .. "_metatable"
    -- The children must be complete types, so include their header.
    -- @todo only include that child specific header
    for _, class in ipairs(self:get_children()) do
        cpp_output = cpp_output .. class:get_code_prepend() .. br
    end
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const char * const " .. cpp_name .. "::METATABLE_NAME = \"" .. metatable_name .. "\";" .. br
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. cpp_class_name .. " *" .. cpp_name .. "::get_subclass( lua_State* const S, int const i) {"..br
    for _, class in ipairs(self:get_children()) do
        local cpp_child_name = "LuaValue<" .. class:get_cpp_name() .. ">";
        cpp_output = cpp_output .. tab .. "if("..cpp_child_name.."::has(S, i)) {" .. br
        cpp_output = cpp_output .. tab .. tab .. "return &"..cpp_child_name.."::get( S, i );" .. br
        cpp_output = cpp_output .. tab .. "}" .. br
    end
    cpp_output = cpp_output .. tab .. "(void)S; (void)i;" .. br -- just in case to prevent warnings
    cpp_output = cpp_output .. tab .. "return nullptr;" .. br
    cpp_output = cpp_output .. "}" .. br
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. cpp_class_name.."* LuaPointer<"..cpp_class_name..">::get_subclass( lua_State* const S, int const i) {"..br
    for _, class in ipairs(self:get_children()) do
        local cpp_child_name = "LuaPointer<" .. class:get_cpp_name() .. ">";
        cpp_output = cpp_output .. tab .. "if("..cpp_child_name.."::has(S, i)) {" .. br
        cpp_output = cpp_output .. tab .. tab .. "return "..cpp_child_name.."::get( S, i );" .. br
        cpp_output = cpp_output .. tab .. "}" .. br
    end
    cpp_output = cpp_output .. tab .. "(void)S; (void)i;" .. br -- just in case to prevent warnings
    cpp_output = cpp_output .. tab .. "return nullptr;" .. br
    cpp_output = cpp_output .. "}" .. br
    cpp_output = cpp_output .. self:generate_functions_static(cpp_name)
    return cpp_output
end

function Class:generate_functions()
    local cur_class_name = self.name
    local cpp_class_name = self:get_cpp_name()
    local cpp_output = ""
    cpp_output = cpp_output .. self:generate_destructor()
    local attributes = {}
    local class = self
    while class do
        for k,v in pairs(class.attributes) do
            attributes[k] = v
        end
        class = class:get_parent()
    end
    cpp_output = cpp_output .. self:generate_accessors("get_member", attributes, generate_getter_code)
    cpp_output = cpp_output .. self:generate_accessors("set_member", attributes, generate_setter_code)
    if self.new then
        cpp_output = cpp_output .. self:generate_constructor()
    end
    if self.has_equal then
        cpp_output = cpp_output .. self:generate_operator("eq", "==")
    end
    cpp_output = cpp_output .. self:generate_function_wrappers(self.functions, cur_class_name)
    return cpp_output
end

-- Checks whether we have a copy constructor, note that `new` is now in the format
-- of the overload resolution tree, see generate_overload_tree
function Class:can_copy()
    if self.new then
        if self.new[self.name] then
            if self.new[self.name].r then
                return true
            end
        end
    end
    return false
end

function Class:generate_code()
    local cpp_output = "// This file was automatically generated by lua/generate_bindings.lua"..br
    local cpp_class_name = self:get_cpp_name()
    cpp_output = cpp_output .. "extern \"C\" {"..br
    cpp_output = cpp_output .. "#include <lua.h>"..br
    cpp_output = cpp_output .. "}"..br
    cpp_output = cpp_output .. "#include \"type.h\""..br
    cpp_output = cpp_output .. "#include \"enum.h\""..br
    cpp_output = cpp_output .. "#include \"value_or_reference.h\""..br
    cpp_output = cpp_output .. br
    -- Needed for `std::exception` and derived
    cpp_output = cpp_output .. "#include <stdexcept>" .. br
    -- Needed for `std::reference_wrapper`
    cpp_output = cpp_output .. "#include <functional>" .. br
    -- Required for std::strcmp as used by the generate_accessors_impl function
    cpp_output = cpp_output .. "#include <cstring>"..br
    cpp_output = cpp_output .. br
    cpp_output = cpp_output .. "class lua_engine;" .. br
    cpp_output = cpp_output .. "lua_State *get_lua_state( const lua_engine & );" .. br
    cpp_output = cpp_output .. "template<typename T> void push_wrapped_onto_stack( const lua_engine &, const T & );" .. br
    cpp_output = cpp_output .. "template<typename T> T get_wrapped_from_stack( const lua_engine &, int );" .. br
    cpp_output = cpp_output .. br
    cpp_output = cpp_output .. self:get_code_prepend() .. br
    cpp_output = cpp_output .. self:generate_functions()
    cpp_output = cpp_output .. self:generate_constants()

    if self:can_copy() then
        cpp_output = cpp_output .. "template<> void push_wrapped_onto_stack( const lua_engine &engine, const " .. cpp_class_name .. " &val ) {" .. br
        cpp_output = cpp_output .. "    LuaValue<" .. cpp_class_name .. ">::push( get_lua_state( engine ), val );" .. br
        cpp_output = cpp_output .. "}" .. br

        cpp_output = cpp_output .. "template<> " .. cpp_class_name .. " get_wrapped_from_stack<" .. cpp_class_name .. ">( const lua_engine &engine, const int index ) {" .. br
        cpp_output = cpp_output .. "    if( LuaValueOrReference<" .. cpp_class_name .. ">::has( get_lua_state( engine ), index ) ) {" .. br
        cpp_output = cpp_output .. "        return LuaValueOrReference<" .. cpp_class_name .. ">::get( get_lua_state( engine ), index );" .. br
        cpp_output = cpp_output .. "    }" .. br
        cpp_output = cpp_output .. "    throw std::runtime_error( \"unexpected value on Lua stack\" );" .. br
        cpp_output = cpp_output .. "}" .. br
    end

    -- Allow pushing references to const and to non-const values alike (Lua doesn't have the concept of "const").
    cpp_output = cpp_output .. "template<> void push_wrapped_onto_stack( const lua_engine &engine, const std::reference_wrapper<const " .. cpp_class_name .. "> &val ) {" .. br
    cpp_output = cpp_output .. "    LuaPointer<" .. cpp_class_name .. ">::push( get_lua_state( engine ), &val.get() );" .. br
    cpp_output = cpp_output .. "}" .. br
    cpp_output = cpp_output .. "template<> void push_wrapped_onto_stack( const lua_engine &engine, const std::reference_wrapper<" .. cpp_class_name .. "> &val ) {" .. br
    cpp_output = cpp_output .. "    LuaPointer<" .. cpp_class_name .. ">::push( get_lua_state( engine ), &val.get() );" .. br
    cpp_output = cpp_output .. "}" .. br

    -- Don't return a reference to an object managed by Lua (created by value) as we can't know its
    -- lifetime from within the calling C++ code.
    cpp_output = cpp_output .. "template<> " .. cpp_class_name .. " &get_wrapped_from_stack<" .. cpp_class_name .. "&>( const lua_engine &engine, const int index ) {" .. br
    cpp_output = cpp_output .. "    if( LuaValueOrReference<" .. cpp_class_name .. ">::has( get_lua_state( engine ), index ) ) {" .. br
    cpp_output = cpp_output .. "        return LuaValueOrReference<" .. cpp_class_name .. ">::get( get_lua_state( engine ), index );" .. br
    cpp_output = cpp_output .. "    }" .. br
    cpp_output = cpp_output .. "    throw std::runtime_error( \"unexpected value on Lua stack\" );" .. br
    cpp_output = cpp_output .. "}" .. br

    return cpp_output
end

function Enum:generate_code()
    local cpp_name = "LuaEnum<" .. self:get_cpp_name() .. ">"
    local cpp_output = "// This file was automatically generated by lua/generate_bindings.lua"..br
    cpp_output = cpp_output .. "extern \"C\" {"..br
    cpp_output = cpp_output .. "#include <lua.h>"..br
    cpp_output = cpp_output .. "}"..br
    cpp_output = cpp_output .. "#include \"enum.h\""..br
    cpp_output = cpp_output .. br
    cpp_output = cpp_output .. "class lua_engine;" .. br
    cpp_output = cpp_output .. "lua_State *get_lua_state( const lua_engine & );" .. br
    cpp_output = cpp_output .. "template<typename T> void push_wrapped_onto_stack( const lua_engine &, const T & );" .. br
    cpp_output = cpp_output .. "template<typename T> T get_wrapped_from_stack( const lua_engine &, int );" .. br
    cpp_output = cpp_output .. br
    cpp_output = cpp_output .. self:get_code_prepend() .. br
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const "..cpp_name.."::EMap "..cpp_name.."::BINDINGS = {"..br
    for _, name in ipairs(self.values) do
        cpp_output = cpp_output .. tab.."{\""..name.."\", "..self:get_cpp_name().."::"..name.."},"..br
    end
    cpp_output = cpp_output .. "};" .. br
    cpp_output = cpp_output .. "template<> void push_wrapped_onto_stack( const lua_engine &engine, const " .. self:get_cpp_name() .. " &val ) {" .. br
    cpp_output = cpp_output .. "    " .. cpp_name .. "::push( get_lua_state( engine ), val );" .. br
    cpp_output = cpp_output .. "}" .. br
    cpp_output = cpp_output .. "template<> " .. self:get_cpp_name() .. " get_wrapped_from_stack<" .. self:get_cpp_name() .. ">( const lua_engine &engine, const int index ) {" .. br
    cpp_output = cpp_output .. "    if( !" .. cpp_name .. "::has( get_lua_state( engine ), index ) ) {" .. br
    cpp_output = cpp_output .. "        throw std::runtime_error( \"unexpected value on Lua stack\" );" .. br
    cpp_output = cpp_output .. "    }" .. br
    cpp_output = cpp_output .. "    return " .. cpp_name .. "::get( get_lua_state( engine ), index );" .. br
    cpp_output = cpp_output .. "}" .. br
    return cpp_output
end


function Class:load_metatable_call()
    local cpp_name = "LuaValue<" .. self:get_cpp_name() .. ">"
    -- If the class has a constructor, it should be exposed via a global name (which is the class name)
    if self.new then
        return cpp_name .. "::load_metatable( L, \"" .. self:get_cpp_name() .. "\" );"
    else
        return cpp_name .. "::load_metatable( L, nullptr );"
    end
end

function Enum:load_metatable_call()
    -- Enumerations are always exported globally
    return "LuaEnum<" .. self:get_cpp_name() .. ">::export_global( L, \"" .. self.name .. "\" );"
end

function generate_main_init_function()
    local cpp_output = "// This file was automatically generated by lua/generate_bindings.lua"..br
    cpp_output = cpp_output .. "extern \"C\" {"..br
    cpp_output = cpp_output .. "#include <lua.h>"..br
    cpp_output = cpp_output .. "}"..br
    cpp_output = cpp_output .. "#include \"type.h\""..br
    cpp_output = cpp_output .. "#include \"enum.h\""..br
    cpp_output = cpp_output .. "#include \"value_or_reference.h\""..br
    cpp_output = cpp_output .. global_functions_code_prepend .. br
    cpp_output = cpp_output .. "#include \"string_id.h\"" .. br
    cpp_output = cpp_output .. "#include \"int_id.h\"" .. br
    for _, t in ipairs(sorted_types) do
        cpp_output = cpp_output .. t:get_forward_declaration() .. br
    end

    -- Create a function that calls load_metatable on all the registered LuaValue's
    cpp_output = cpp_output .. "void load_metatables(lua_State* const L) {" .. br
    for _, t in ipairs(sorted_types) do
        cpp_output = cpp_output .. tab .. t:load_metatable_call() .. br
    end
    cpp_output = cpp_output .. "}" .. br

    cpp_output = cpp_output .. "#include <stdexcept>" .. br
    cpp_output = cpp_output .. "class lua_engine;" .. br
    cpp_output = cpp_output .. "lua_State *get_lua_state( const lua_engine & );" .. br
    cpp_output = cpp_output .. "template<typename T> void push_wrapped_onto_stack( const lua_engine &, const T & );" .. br
    cpp_output = cpp_output .. "template<typename T> T get_wrapped_from_stack( const lua_engine &, int );" .. br

    -- Create a lua registry with the global functions
    cpp_output = cpp_output .. "static const struct luaL_Reg gamelib [] = {"..br

    for _, name in ipairs(global_functions) do
        cpp_output = cpp_output .. tab .. '{"'..name..'", catch_exception_for_lua_wrapper<&global_'..name..'>},'..br
    end

    cpp_output = cpp_output .. tab .. "{NULL, NULL}"..br.."};"..br

    return cpp_output
end

function writeFile(path,data)
    local file = io.open(path, "r")
    if file then
        local existing_content = file:read("*a")
        file:close()
        if existing_content == data then
            return
        end
    end
    file = io.open(path, "wb")
    if not file then
        error("could not open " .. path)
    end
    file:write(data)
    file:close()
end

-- Generate all code that goes into the same output file.
-- This may contain wrappers for multiple types.
function generate_code_for_output_path(path)
    local result = ""
    -- @todo the common prefix (includes and the like) should only be contained
    -- once, but currently each invocation of generate_code_for creates it.
    for _, t in ipairs(sorted_types) do
        if path == t:get_output_path() then
            result = result .. t:generate_code()
        end
    end
    return result
end

local generated_files = { }
function generate_and_write_for_path(path)
    if not path then
        return
    end
    if generated_files[path] then
        return
    end
    generated_files[path] = true
    writeFile(path, generate_code_for_output_path(path))
end

for _, t in ipairs(sorted_types) do
    generate_and_write_for_path(t:get_output_path())
end

writeFile("catabindings.gen.cpp", generate_main_init_function())
