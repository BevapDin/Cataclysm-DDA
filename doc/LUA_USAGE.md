# Using Lua

## Using Lua in mods

For each mod the game attempts to run two Lua files from the mod folder:

- `<mod-path>/preload.lua` (run before the JSON data is loaded) and
- `<mod-path>/main.lua` (run after the JSON data has been loaded).

If a file does not exist, it's silently skipped. Typically those files contain global functions needed throughout the mod, or callback handlers (which should be set up there, see "Callbacks" below).

Other Lua files are not automatically run, but you can run them via a Lua function from within those files (see "Running Lua script files").

During the game, Lua code is run either via a callback (triggered by specific events), or via scripts referenced in the JSON definitions (e.g. when activating a tool item).

### Callbacks

The game invokes Lua functions upon specific events (e.g. when a turn ends or when the skill of a character has increased). Each mod can register a handler for those events. When the event happens, the matching handlers of all mods are called (in an arbitrary order). Callbacks may have arbitrary arguments (see documentation of callbacks). Callbacks never return anything (return values are ignored).

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

### Lua in JSON

Some JSON data is interpreted as Lua scripts (see JSON documentation). They behave like any other JSON value (e.g. can be overridden by other mods). The game invokes those script under specific circumstances, and it may provide some arguments. Different from callbacks, those scripts can have return values (and are often expected to have them).

The following formats are recognised as Lua scripts:
- array of strings (if the array contains non-strings, an error is raised): the strings are concatenated with new-line characters between each strings (each entry forms its own line).
- simple string: behaves like an array containing only this entry.
- an object with the member "file", yielding a string. The string is taken as a path to a file that is loaded as the Lua script. Loading happens at the time of the JSON parsing (the file must be readable at that time).

### Running Lua script files

The game provides the global function `game.dofile` to run external Lua script files:

```Lua
game.dofile("some/path/file.lua")
```

The path is relative:
- when run during the startup (when the game data is read, while starting a game), it is relative to the mod folder (e.g. "data/mods/my-mod/"),
- when run while the game is already going (e.g. from a callback or upon a player action), it's relative to the main game folder (where the cataclysm binary resides).

@todo simplify this and make it consistent

## API

The Lua interface to the game is a one-to-one wrapper of the underlying C++ functions/types. It follows the C++ principles:
- array are 0 to N-1 (and not 1 to N as in Lua),

@todo maybe add more here as it becomes necessary

@todo make documentation generator and refer to it here.

