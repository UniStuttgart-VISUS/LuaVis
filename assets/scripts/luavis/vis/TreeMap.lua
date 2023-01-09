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
local max = math.max
local min = math.min
local floor = math.floor

local baseRect = rect(vector2(), screenSize):centerWithin(vector2(12, 8) * 50)

--local baseNode = {label = "", color = {203, 203, 203}, rect = baseRect}
local baseNode = {label = "", color = {50, 50, 50}, rect = baseRect}

local nodes = {
	{label = "A", weight = 30, color = {0, 128, 102}},
	{label = "B", weight = 24, color = {255, 42, 42}},
	{label = "C", weight = 18, color = {255, 153, 85}},
	{label = "D", weight = 14, color = {0, 170, 212}},
	{label = "E", weight = 10, color = {0, 255, 0}},
}
local xnodes = {
	{label = "6", weight = 6, color = {40, 40, 40}},
	{label = "6", weight = 6, color = {35, 35, 35}},
	{label = "4", weight = 4, color = {30, 30, 30}},
	{label = "3", weight = 3, color = {25, 25, 25}},
	{label = "2", weight = 2, color = {20, 20, 20}},
	{label = "2", weight = 2, color = {15, 15, 15}},
	{label = "1", weight = 1, color = {10, 10, 10}},
}

local nodeWeightSum = 0
for i = 1, #nodes do
	nodeWeightSum = nodeWeightSum + nodes[i].weight
end

for i = 1, #nodes do
	nodes[i].area = (baseRect.w * baseRect.h) * nodes[i].weight / nodeWeightSum
end

local remainingNodes = utils.arrayCopy(nodes)
local steps = {}
stepID = 1

local function drawOutlineText(data, outlineColor, outlineThickness)
	local origColor = data.fillColor
	local origX, origY = data.x, data.y
	data.fillColor = outlineColor
	for i = 0, 8 do
		data.x, data.y = origX + floor(i / 3 - 1) * outlineThickness, origY + (i % 3 - 1) * outlineThickness
		draw.text(data)
	end
	data.fillColor = origColor
	data.x = origX
	data.y = origY
	draw.text(data)
end

local function drawNode(node)
	local step = steps[stepID]
	local r = node.rect or step.layout[node] or step.finalLayout[node]
	if r then
		draw.rect(r, node.color)
		drawOutlineText({
			font = draw.Font.SYSTEM,
			text = node.label,
			x = r:center().x,
			y = r:center().y,
			size = 32,
			fillColor = draw.Color.WHITE,
			shadowColor = {0, 0, 0, 0},
			alignX = 0.5,
			alignY = 0.5,
			maxWidth = r.w,
			maxHeight = r.h,
		}, draw.Color.BLACK, 2)
	end
end

local function getAreaSum(nodesToSum)
	local area = 0
	for i = 1, #nodesToSum do
		local node = nodesToSum[i]
		area = area + node.area
	end
	return area
end

local function rectFraction(r, startFraction, sizeFraction, vertical)
	return vertical
		and rect(r.x, r.y + r.h * (1 - startFraction - sizeFraction), r.w, r.h * sizeFraction)
		 or rect(r.x + r.w * startFraction, r.y, r.w * sizeFraction, r.h)
end

local function genLayout(nodesToLayout, r)
	local layout = {}
	local vertical = r.w < r.h
	local nodeAreaSum = getAreaSum(nodesToLayout)
	local layoutFraction = nodeAreaSum / (r.w * r.h)
	local layoutRect = rectFraction(r, 0, layoutFraction, vertical)
	local remainingRect = rectFraction(r, layoutFraction, 1 - layoutFraction, vertical)
	local fraction = 0
	local worst = 0
	for i = 1, #nodesToLayout do
		local node = nodesToLayout[i]
		local nodeFraction = node.area / nodeAreaSum
		local nodeRect = rectFraction(layoutRect, fraction, nodeFraction, not vertical)
		layout[node] = nodeRect
		fraction = fraction + nodeFraction
		worst = max(worst, nodeRect.w / nodeRect.h, nodeRect.h / nodeRect.w)
	end
	return layout, worst, remainingRect
end

local function append(list, item)
	local copy = utils.arrayCopy(list)
	copy[#copy + 1] = item
	return copy
end

local function squarify()
	local remainingNodes = utils.arrayCopy(nodes)
	local prevWorst, prevRemaining, prevLayout
	local finalLayout = {}
	local row = {}
	local layoutRect = baseRect
	while #remainingNodes ~= 0 do
		local rejected = false
		local layout, worst, remaining = genLayout(append(row, remainingNodes[1]), layoutRect)
		if prevWorst == nil or prevWorst > worst then
			table.insert(row, table.remove(remainingNodes, 1))
		else
			row = {}
			rejected = true
			prevWorst = nil
			layoutRect = prevRemaining
			for k, v in pairs(prevLayout) do
				finalLayout[k] = v
			end
		end
		steps[#steps + 1] = {
			rejected = rejected,
			layout = layout,
			worst = worst,
			finalLayout = utils.shallowCopy(finalLayout),
		}
		prevWorst = worst
		prevRemaining = remaining
		prevLayout = layout
	end
end

squarify()

event.render.add("treemap", "vis", function ()
	do return end
	if input.keyPress("left") then
		stepID = max(stepID - 1, 1)
	end
	if input.keyPress("right") then
		stepID = min(stepID + 1, #steps)
	end

	local currentStep = steps[stepID]
	if currentStep then
		drawNode(baseNode)
		for i = 1, #nodes do
			drawNode(nodes[i])
		end
		draw.text {
			font = draw.Font.SYSTEM,
			text = "Step " .. stepID .. (currentStep.rejected and " (rejected)" or ""),
			x = baseNode.rect:bottomLeft().x,
			y = baseNode.rect:bottomLeft().y + 4,
			size = 32,
			outlineThickness = 0,
			alignX = 0,
			alignY = 0,
		}
		draw.text {
			font = draw.Font.SYSTEM,
			text = string.format("Worst ratio = %.3f", currentStep.worst),
			x = baseNode.rect:bottomRight().x,
			y = baseNode.rect:bottomRight().y + 4,
			size = 32,
			outlineThickness = 0,
			alignX = 1,
			alignY = 0,
		}
	end
end)
