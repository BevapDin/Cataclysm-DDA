# Using Lua from C++

## Build

The Lua library can be accessed via a set of C functions, it also supports callbacks from Lua code via C functions. The details of that interface are hidden in a separate library called `libcatalua.a` and exposed via headers in `src/lua`.

The library is compiled from different files based on whether Lua is enabled or not. The interface is common for both build types. This means one can do `make LUA=1` (Lua build) and `make` (non-Lua build) without issuing `make clean` (but one has to remove the executable and the `libcatalua.a` file). The C++ files do not contain any check for Lua, instead they either assume Lua is enabled, or it's disabled or they don't care and `make` selects which files are to be compiled for the current build.

For non-Lua builds, `make` selects different files to build the library from. Those files assume Lua is not enabled and don't use it. They provide dummy implementations (that essentially do nothing) so the interface is still implemented.


