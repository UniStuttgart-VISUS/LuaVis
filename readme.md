# LuaVis

Interactive framework for the visualization of flow graphs in porous media.



## Compilation

LuaVis (C++) depends on the following libraries (please use the linked repositories for compatibility):

- [LuaJIT](https://github.com/LuaJIT/LuaJIT/tree/v2.1)
- [cppfs](https://github.com/straubar/cppfs)
- [SFML](https://github.com/Marukyu/SFML)
- [zlib](https://github.com/madler/zlib)



**Note:** Use CMake install to install the project and copy all relevant files!



## Usage

Start LuaVis by executing the *run* script.

The graph shown is the one configured in the beginning of `assets/scripts/luavis/vis/Graph.lua`.



**Note:** Your graphs and their associated images must be in a subdirectory of `assets`. You can use symbolic links to point to other directories.



## License information

LuaVis additionally needs the [SVG-Lua](https://github.com/Jericho1060/svg-lua.git) library, which is included as `assets/scripts/luavis/vis/SVG.lua`. Please also see its [license](assets/scripts/luavis/vis/SVG_LICENSE).
