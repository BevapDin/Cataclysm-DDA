# Using Lua

## Using Lua in mods

For each mod the game attempts to run two Lua files from the mod folder: `<mod-path>/preload.lua` (run before the JSON data is loaded) and `<mod-path>/main.lua` (run after the JSON data has been loaded). If a file does not exist, it's silently skipped. Other Lua files are not automatically run (but you can run them via a Lua function from within those files).

## Callbacks

@todo Write a list of all callbacks (and their parameters) and describe when they get called

## C++ functions and classes
