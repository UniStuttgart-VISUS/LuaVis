# LuaVis

Implementation of parts of Adrian Zeyfang's Master's thesis for the visualization of graphs derived from porous media datasets.



## Compilation

LuaVis (C++) depends on the following libraries (please use the linked repositories for compatibility):

- [LuaJIT](https://github.com/LuaJIT/LuaJIT)
- [cppfs](https://github.com/straubar/cppfs)
- [SFML](https://github.com/Marukyu/SFML)



To run the LuaVis application with the implemented Lua scripts, additionally, the following Lua libraries have to be provided:

- [SVG-Lua](https://github.com/Jericho1060/svg-lua.git): `svg.lua` or `svg-min.lua` as `assets/scripts/luavis/vis/SVG.lua`



### Windows

After building the dependencies and LuaVis, copy

- the *assets* folder,
- the *config.json* file,
- the *run.bat* script, and
- the DLL files from the dependencies

to the folder of the *LuaVis.exe* executable, such that the directory structure looks as follows:

![](directory_structure.png)



## Usage

Start by executing the *run* script.

The graph shown is the one configured in line 14 of `assets/scripts/luavis/vis/Graph.lua`.

Stored graphs can be found in `assets/scripts/luavis/vis/data` and `assets/scripts/luavis/vis/graphdata`.
