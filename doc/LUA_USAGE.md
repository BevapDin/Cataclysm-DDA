# Using Lua

## Using Lua in mods

For each mod the game attempts to run two Lua files from the mod folder:

- `<mod-path>/preload.lua` (run before the JSON data is loaded) and
- `<mod-path>/main.lua` (run after the JSON data has been loaded).

If a file does not exist, it's silently skipped. Other Lua files are not automatically run, but they you can run them via a Lua function from within those files.

@todo document which function that is and how it works

Typically those files contain global functions needed throughout the mod, or callback handlers (which should be set up there).

## Callbacks

The game invokes Lua functions upon specific events (e.g. when a turn ends or when the skill of a character has increased). Each mod can register a handler for those events. When the event happens, all matching handlers of all mods are called (in an arbitrary order). Callbacks may have arbitrary arguments (see documentation of callbacks). Callbacks never return anything (return values are ignored).

@todo Write a list of all callbacks (and their parameters) and describe when they get called

Callbacks can be registered from Lua code like this:
```Lua
MOD = { }
-- Callback handler, see documentation of callbacks what callbacks are
-- available and what parameters they have.
function MOD.on_skill_increased(skill, level)
end

-- Register callbacks, use the ident of your mod here.
mods["mod-ident"] = MOD
-- @todo write a Lua function to do this
```

## C++ functions and classes


## Lua in JSON

Some JSON data is interpreted as Lua scripts (see JSON documentation). They behave like any other JSON value (e.g. can be overridden by other mods). The game invokes those script under specific circumstances, and it may provide some arguments. Different from callbacks, those scripts can have return values (and are often expected to have them).

The following formats are recognised:
- array of strings (if the array is empty or contains non-strings, an error is raised): the strings are concatenated with new-line characters between each strings (each entry forms its own line).
- simple string: behaves like an array containing only this entry.
- an object with either the entry "file" or "include" (each yielding a string). The string is taken as a path to a file that is loaded as Lua script.

The last format allows loading a Lua file referenced from JSON. If the object contains a "file" member, it indicates the content of the given path should be loaded when the script is run. If the script is never run, the file will never be loaded. If the file is changed, running the script again will use the new content of it. This is useful during development as one does not need to reload the game data. The member "include" indicates the content should be loaded when the JSON is loaded (intended to be used in the finished version of the mod). It's only loaded once. If the file does not exist when it's loaded, an error is raised. 
