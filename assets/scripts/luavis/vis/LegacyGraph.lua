local draw = require "luavis.vis.Draw"
local colorScale = require "luavis.vis.ColorScale"
local vector2 = require "system.utils.Vector2"
local rect = require "system.utils.Rect"
local gfx = require "system.game.Graphics"
local input = require "system.game.Input"
local utils = require "system.utils.Utilities"

local screenSize = vector2(gfx.getWidth(), gfx.getHeight())
local screenCenter = screenSize / 2

local pi = math.pi

local nodeBaseRadius = 20
local nodeOutlineThickness = 2

local extraInfo = true

local nodeColorScale, linkColorScale

local tempMin, tempMax = 0, 20

local function initColorScales()
	nodeColorScale = colorScale.new({
		{0, 0, 255},
		{255, 255, 255},
		{255, 30, 0},
	}, tempMin, tempMax)
	linkColorScale = colorScale.new({
		draw.colorWithAlpha(draw.Color.FG_YELLOW, 0),
		draw.colorWithAlpha(draw.Color.FG_YELLOW, 255),
	}, 0, 70)
end

initColorScales()

local nodes = {
	{label =  "D", temp = 10.0, pop = 83.0},
	{label = "FR", temp = 12.3, pop = 67.4},
	{label = "UK", temp = 10.3, pop = 66.5},
	{label = "SE", temp =  6.6, pop = 10.2},
	{label = "PT", temp = 17.5, pop = 10.3},
	{label = "IT", temp = 15.2, pop = 60.5},
	{label = "ES", temp = 19.1, pop = 46.8},
	{label = "DK", temp =  8.2, pop =  5.7},
	{label = "FI", temp =  3.4, pop =  5.5},
	{label = "EL", temp = 18.5, pop = 10.8},
}

local adjacency = {
	{30, 15,  5,  2,  2, 10, 15,  1,  1,  7},
	{ 5, 61,  3,  1,  6,  5, 10,  2,  1,  2},
	{15,  5, 25,  2, 10, 12, 28,  2,  5,  6},
	{ 7, 10,  6, 36,  5,  6, 14,  8,  6,  1},
	{ 6,  8, 11,  9, 27,  2, 36,  4,  1,  1},
	{12, 14,  8,  5, 10, 30,  2,  7,  3,  7},
	{ 4, 15,  3,  1, 16,  3, 44,  3,  2,  3},
	{11, 13,  7,  8,  6,  9, 12, 25,  5,  4},
	{ 8, 11,  9, 13,  2,  3, 18,  7, 27,  1},
	{ 3,  2,  3,  1,  1, 12,  2,  1,  1, 70},
}

local nodeOrder = {}
for i, v in ipairs {"PT", "ES", "UK", "SE", "IT", "DK", "FR", "EL", "D", "FI"} do
	nodeOrder[v] = i
end

local links = {}

for sourceID = 1, #adjacency do
	local row = adjacency[sourceID]
	for destID = 1, #row do
		local weight = row[destID]
		if weight > 5 then
			links[#links + 1] = {source = nodes[sourceID], dest = nodes[destID], weight = weight}
		end
	end
end

for i = 1, #nodes do
	local node = nodes[i]
	node.index = nodeOrder[node.label]
	node.pos = vector2(screenCenter.y - nodeBaseRadius * 2, 0):rotate((1 - node.index) / #nodes * 2 * pi) + screenCenter
end

local function drawNode(node)
	local nodeRadius = extraInfo and math.sqrt(node.pop) * 3 or nodeBaseRadius
	draw.circle(node.pos, nodeRadius + nodeOutlineThickness, draw.Color.OUTLINE, 30)
	draw.circle(node.pos, nodeRadius, extraInfo and nodeColorScale(node.temp) or {255, 255, 200}, 30)
	draw.text {
		font = draw.Font.SYSTEM,
		text = node.label,
		x = node.pos.x,
		y = node.pos.y,
		size = nodeRadius * 1.6,
		fillColor = draw.Color.BLACK,
		shadowColor = {255, 255, 255, 32},
		outlineThickness = 0,
		alignX = 0.5,
		alignY = 0.5,
	}
end

local function drawLink(link)
	draw.line(link.source.pos, link.dest.pos, linkColorScale(link.weight), link.weight, 0)
end

local function formatTemperature(value)
	return string.format("%d\004C", value)
end

local function drawColorScale()
	local margin = 10
	local size = vector2(120, 20)
	local left = screenCenter.x - screenCenter.y
	local labels = {formatTemperature(tempMin), formatTemperature(tempMax)}
	nodeColorScale.render(rect(left + margin, screenSize.y - margin - size.y, size.x, size.y), 2, labels)
end

event.themeChanged.add("graph", "colors", initColorScales)

event.render.add("graph", "vis", function ()
	do return end
	for i = 1, #links do
		drawLink(links[i])
	end
	for i = 1, #nodes do
		drawNode(nodes[i])
	end
	if extraInfo then
		drawColorScale()
	end
end)
