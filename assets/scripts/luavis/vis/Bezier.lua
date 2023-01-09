local draw = require "luavis.vis.Draw"
local colorScale = require "luavis.vis.ColorScale"
local vector2 = require "system.utils.Vector2"
local rect = require "system.utils.Rect"
local gfx = require "system.game.Graphics"
local input = require "system.game.Input"
local utils = require "system.utils.Utilities"
local color = require "system.utils.Color"

local screenSize = vector2(gfx.getWidth(), gfx.getHeight())
local screenCenter = screenSize / 2

local pi = math.pi
local max = math.max
local min = math.min
local floor = math.floor

local points = {
	vector2(1.5, 1),
	vector2(1.5, 3),
	vector2(6, 1),
	vector2(5, 5.5),
	vector2(3, 5),
}

local ca, cb = vector2(10, 10), vector2(10, -10)

local function cross(pm, col)
	draw.line(pm + ca, pm - ca, color.setA(col, 128), 2)
	draw.line(pm + cb, pm - cb, color.setA(col, 128), 2)
	draw.line(pm + ca, pm - ca, col, 1)
	draw.line(pm + cb, pm - cb, col, 1)
end

event.render.add("bezier", "vis", function ()
	do return end
	local colors = {
		draw.rgb(20, 20, 20),
		draw.rgb(255, 0, 0),
		draw.rgb(0, 150, 180),
		draw.rgb(0, 0, 255),
	}

	local fac = screenSize.y / 10
	local gv = 220

	for x = 0, 20 do
		draw.line(vector2(fac * x, 0), vector2(fac * x, screenSize.y), draw.rgb(gv, gv, gv))
		if x <= 6 and x > 0 then
			draw.text {
				font = draw.Font.VECTOR,
				x = fac * x - 1,
				y = fac * 6,
				text = tostring(x - 1),
				size = 32,
				alignX = 0.5,
			}
		end
	end
	for y = 0, 10 do
		draw.line(vector2(0, fac * y), vector2(screenSize.x, fac * y), draw.rgb(gv, gv, gv))
		if y < 6 and y > 0 then
			draw.text {
				font = draw.Font.VECTOR,
				x = fac * 1 - 25,
				y = fac * y - 8,
				text = tostring(6 - y),
				size = 32,
				alignY = 0.5,
			}
		end
	end
	draw.line(vector2(fac * 1, fac * 1), vector2(fac * 1, fac * 6), draw.rgb(0, 0, 0), 3)
	draw.line(vector2(fac * 1, fac * 1 - 20), vector2(fac * 1, fac * 1), draw.rgb(0, 0, 0), 0, 15)
	draw.text {
		font = draw.Font.VECTOR,
		x = fac * 1 - 10,
		y = fac * 0,
		text = "y",
		size = 32,
	}
	draw.line(vector2(fac * 1, fac * 6), vector2(fac * 6.5, fac * 6), draw.rgb(0, 0, 0), 3)
	draw.line(vector2(fac * 6.5 + 20, fac * 6), vector2(fac * 6.5, fac * 6), draw.rgb(0, 0, 0), 0, 15)
	draw.text {
		font = draw.Font.VECTOR,
		x = fac * 7,
		y = fac * 6 - 23,
		text = "x",
		size = 32,
	}

	local tempPoints = utils.shallowCopy(points)
	for iteration = 1, #points - 1 do
		local newPoints = {}
		for i = 1, #tempPoints - 1 do
			local pa, pb = tempPoints[i], tempPoints[i + 1]
			draw.line(pa * fac, pb * fac, colors[iteration], 3)
			local pm = pa * 0.25 + pb * 0.75
			newPoints[i] = pm
			cross(pm * fac, colors[iteration])
		end
		if #newPoints == 1 then
			--dbg(newPoints[1].x, newPoints[1].y)
		end
		tempPoints = newPoints
	end
end)
