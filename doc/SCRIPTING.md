# Scripting the Cataclysm

This is implemented via [Lua](https://www.lua.org/). It's currently optional, make sure you either build your game with `LUA=1` or you download a build that was build with Lua. Builds without Lua do not support any scripting.

## Lua side

See [Lua usage](LUA_USAGE.md) on how to use Lua in the game. The game exports a strict subset of its C++ classes and functions to Lua. This means function calls in Lua are forwarded directly to the matching C++ function. Documentation for the C++ code also applies to the exported things (especially restrictions on input and output).

If in doubt, look at the C++ code of the same name.

@todo add a Lua documentation generator

## C++ side

See [Lua from C++](LUA_FROM_CPP.md) on how to interact with Lua from C++.


