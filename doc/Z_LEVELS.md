Z-Levels:
=========

It works like this:
The grid in `map` holds a 3-D matrix of submaps. Most map functions have a second version that uses a tripoint instead of point or x,y as coordinates. The (old) function (that uses only 2-D coordinates) dispatch each call to new function with an assumed z-component of 0.

Shifting is still in and is extended to vertical movement. That means there is no despawn of all monsters on vertical movement. It simply uses the same shift logic (slightly simplified) to shift the submaps around and load new submaps.

The mapgen has a simply roof-generation, that generates roofs ("t_roof" on buildings (on top of inside tiles). On top of outside tiles it generates "t_air". It also generates downstairs if the level below has upstairs. The starting shelter has now two upstairs that provide access to the roof.

The shelter also has a second floor, loaded from json. And the roof on top of that.

Terrain and furniture have a new flags:

- ```NOFLOOR```: The tile has no floor to stand on. Items / creatures will fall through. (t_air has that).
- ```TRANSPARENT_FLOOR```: The floor is transparent (but may as well be solid, like glass or like grating). This only allows looking through it. t_air and t_glass_floor have this.
- ```PERMEABLE_FLOOR``` The floor of the tile allows air to pass through (like scent) and liquids, too. t_air and t_floor_grating have this.

The drawing code has been changed to display things that are below. Below content is shown in gray (-1 level) or dark gray (more than -1 level below). Items on different z-levels are not shown, but the tile is highlighted, and the look around lists them. Terrain and furniture have another symbol that is used to when the object is on another z-level (symbol can be specified in json). Currently this defaults to '.' for terrain and ' ' (nothing, not drawn at all) for furniture. Only trees have their normal symbol.

Creatures are currently shown as normal (no color change), this makes it slightly difficult to see if they are on a different z-level.

Vehicles below are shown as normal, only in gray color.

One can use the look around command to look at other z-levels. Or use shift view vertically (new command, unbound per default, can be changed with the key binding menu). This works like the normal view shifting. In look around mode one can change the displayed z-level with `<`/`>` key. This can be changed in data/raw/keybindings.json (entry LEVEL_UP and LEVEL_DOWN), it's the same action that the overmap view uses.


Interaction between z-levels:
==========

Monsters can attack anything on the z-level below it.
If a monster has the ATTACK_ABOVE flag it can attack anything on the z-level above it. Many monsters might be considered to small to attack things above and may not have this flag.

Creatures can at most attack things that are on an adjacent square (`abs(dx) <= 1 && abs(dy) <= 1 && abs(dz) <= 1`). Creatures can attack above if either there is a permeable floor above them or a permeable floor below the target. Similar for attacks below.

Movement:
Terrain/furniture can have the flags `GOES_UP` or `GOES_DOWN` (or both). They indicate a vertical movement without change of the x,y coordinates: going up down vertical leaders.
Another flag `SLOPE` indicates a vertical movement with horizontal movement: going up /down a ramp places one aside of the place one came from. There must of course be a place to stand there. The square above the slope must be free (`NO_FLOOR`). [slopes are not yet implemented!]

Going up / down starirs uses the usual keys (`<`/`>`). Going up / down a slope  is just walking onto / across the slope.
