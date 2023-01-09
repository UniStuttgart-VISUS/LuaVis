local gfx = require "system.game.Graphics"
local draw = require "luavis.vis.Draw"
local rect = require "system.utils.Rect"

event.render.add("background", "background", function()
	gfx.drawRect(rect(0, 0, gfx.getWidth(), gfx.getHeight()), draw.Color.BG)
end)

