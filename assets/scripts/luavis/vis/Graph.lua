---@diagnostic disable: need-check-nil
local fileIO = require "system.game.FileIO"
local framebuffer = require "system.game.Framebuffer"
local gfx = require "system.game.Graphics"
local input = require "system.game.Input"

local color = require "system.utils.Color"
local utils = require "system.utils.Utilities"
local vector2 = require "system.utils.Vector2"

local svglib = require "luavis.vis.SVG"
local draw = require "luavis.vis.Draw"

-- ----------------------------------------------------------
-- Settings to change input dataset and layout.
-- ----------------------------------------------------------
local graphData = require "luavis.vis.graphdata.CurrentGraph"
local splitScreenRatio = 100 / 540

-- ----------------------------------------------------------
-- Set keyboard events and related settings.
-- ----------------------------------------------------------
local mouseKeys = {}
settings = {}

local function setKey(keys, name, default, func)
	if type(keys) ~= "table" then
		keys = {keys}
	end

	for _, key in ipairs(keys) do
		mouseKeys[tostring(key)] = {name = name, func = func}
	end

	if settings[name] == nil then
		settings[name] = default
	end
end

local function pressedKey()
	for k, v in pairs(mouseKeys) do
		if (input.keyPress(k)) then
			v.func(k, v.name)
		end
	end
end

local function toggle(_, name)
	settings[name] = not settings[name]
end

setKey("B", "hidePostBreakthrough", false, toggle)
setKey("N", "hideUnreachedNodes", false, toggle)
setKey("T", "colorByNodeType", false, toggle)
setKey("E", "simplify", true, toggle)
setKey("W", "unweightedLinks", false, toggle)
setKey("I", "showInterfaces", true, toggle)
setKey("L", "logScale", false, toggle)
setKey("F", "metricFanciness", false, toggle)
setKey("S", "screenshotMode", false, toggle)
setKey("C", "smoothGraph", false, toggle)
setKey("D", "showDebugInfo", false, toggle)

local printGraph -- forward defined function
setKey("G", "printGraph", nil, function() printGraph() end)

setKey("M", "activeMetricID", 1, function(_, name) settings[name] = nil end)
setKey({1, 2, 3, 4, 5, 6, 7}, "activeMetricID", 1, function(key, name) settings[name] = tonumber(key) end)

-- ----------------------------------------------------------
-- Parse input path and set layout.
-- ----------------------------------------------------------
local imgDir = (graphData.imgDir
	and (graphData.imgDir:match("(gfx_1/.*/)[^/]*$") or graphData.imgDir:match("(gfx_2/.*/)[^/]*$"))
	or "unknown")

local rightToLeft = not graphData.imgDir:match("(gfx_2/.*/)[^/]*$")

local sizeFactor = gfx.getWidth() / 644

local metricHeight = splitScreenRatio * gfx.getHeight()
local headerHeight = metricHeight + 10

local graphHeight = gfx.getHeight() - headerHeight
local graphWidth = gfx.getWidth() - headerHeight * gfx.getWidth() / gfx.getHeight()
local offsetX = (gfx.getWidth() - graphWidth) / 2
local offsetY = headerHeight

frameCnt = 1
frameNum = 1

-- ----------------------------------------------------------
-- Load image and graph information, and fix graph.
-- ----------------------------------------------------------
local imgW, imgH = 2448, 2050
pcall(function ()
	imgW = graphData.imgW
	imgH = graphData.imgH
end)
local imgSize = vector2(imgW, imgH)

local fb_id = "" .. imgW .. "x" .. imgH

local breakthroughThreshold = 10
pcall(function ()
	breakthroughThreshold = graphData.btt
end)

local nodes = utils.deepCopy(graphData.Nodes)
local edges = utils.deepCopy(graphData.Edges)

for _, node in ipairs(nodes) do
	local function move(dst, src) node[dst], node[src] = node[src], nil end

	-- values stored in the graph file
	move("Time", 1)
	move("Id", 2)
	move("X", 3)
	move("Y", 4)
	move("Velocity", 5)
	move("Modified", 6)
	move("Area", 7)
	move("EdgesIn", 8)
	move("EdgesOut", 9)

	-- values calculated and stored later on
	node["Pos"] = false
	node["Rad"] = false
	node["WIn"] = false
	node["WOut"] = false
	node["Y2"] = false
	node["YCount"] = false
	node["Color"] = false
	node["Uid"] = false
	node["Parent"] = false
	node["Child"] = false
	node["Break"] = false
	node["Color2"] = false
	node["PosOrigSize"] = false
end

-- set up 1-to-1 parent-child relationships with respective largest areas,
-- thus, covering the main channels implicitly
for i = 1, #edges, 2 do
	local src, dest = nodes[edges[i] + 1], nodes[edges[i + 1] + 1]
	local parent = nodes[dest.Parent]
	local child = nodes[src.Child]
	if not parent or parent.Area < src.Area then
		dest.Parent = src.Id + 1
	end
	if not child or child.Area < dest.Area then
		src.Child = dest.Id + 1
	end
end

-- 
local currentUID = 0
local function nextUID()
	currentUID = currentUID + 1
	return currentUID
end

for i, node in ipairs(nodes) do
	local parent = nodes[node.Parent]
	node.Uid = parent and parent.Child == i and parent.Uid or nextUID()
end

-- 
local nodesByTime = {}

for _, node in ipairs(nodes) do
	nodesByTime[node.Time] = nodesByTime[node.Time] or {}
	table.insert(nodesByTime[node.Time], node)
end

-- 
local function traceMainChannel(node)
	if node then
		node.Break = true
		return traceMainChannel(nodes[node.Parent])
	end
end

-- 
local breakthrough = 0

pcall(function ()
	local rects = graphData.Rects
	if rects then
		for i, r in ipairs(rects) do
			local x = rightToLeft and r[1] or imgW - r[3]
			if x < breakthroughThreshold then
				breakthrough = nodes[i].Time
				traceMainChannel(nodes[i])
				break
			end
		end
	end
end)

-- 
local allNodes = {}
local liveNodes = {}
local preBreakNodes = {}

for i, node in ipairs(nodes) do
	allNodes[#allNodes + 1] = i
	if not ((node.EdgesIn == 1 and node.EdgesOut == 1) or graphData.Rects[i][1] == 0) then
		liveNodes[#liveNodes + 1] = i
		if node.Time <= breakthrough then
			preBreakNodes[#preBreakNodes + 1] = i
		end
	end
end

-- ----------------------------------------------------------
-- Create metrics based on graph information.
-- ----------------------------------------------------------
local metrics = {}
local metricData = {}

local function binOpAdd(v1, v2)
	return (v1 or 0) + (v2 or 0)
end

local function makeMetric(name, func, binOp)
	local metric = {}
	local raw = {}
	if type(func) == "function" then
		binOp = binOp or binOpAdd
		for i = 1, #nodes do
			local node = nodes[i]
			local ts = node.Time + 1
			local value = func(node)
			metric[ts] = binOp(value, metric[ts])
			raw[i] = value
		end
	else
		metric = func
	end
	id = #metrics + 1
	metrics[id] = metric
	local maxValue = -math.huge
	local minValue = math.huge
	local min, max = math.min, math.max
	for k, v in pairs(metric) do
		minValue = min(minValue, v)
		maxValue = max(maxValue, v)
	end
	local maxRaw = -math.huge
	local minRaw = math.huge
	for k, v in pairs(raw) do
		minRaw = min(minRaw, v)
		maxRaw = max(maxRaw, v)
	end
	metricData[id] = {
		id = id,
		min = minValue,
		max = maxValue,
		name = name,
		minRaw = minRaw,
		maxRaw = maxRaw,
		raw = raw,
	}
	return metric
end

local metArea = makeMetric("Area", function (node)
	return node.Area
end)

local metNumFingers = {}
local nodeFluidProportion = {}

if graphData.Interfaces then
	local ifaces = graphData.Interfaces

	local metInterfaceFluid = makeMetric("Interface length fluid", function (node)
		return ifaces[node.Id * 2 + 1]
	end)

	for i, node in ipairs(nodes) do
		local ts = node.Time + 1
		local iface = ifaces[node.Id * 2 + 1]
		if metInterfaceFluid[ts] and metInterfaceFluid[ts] > 0 and iface then
			nodeFluidProportion[i] = iface / metInterfaceFluid[ts]
		else
			nodeFluidProportion[i] = 0
		end
	end

	makeMetric("Interface length solid", function (node)
		return ifaces[node.Id * 2 + 2]
	end)

	metNumFingers = makeMetric("Number of fingers", function (node)
		local ts = node.Time + 1

		-- Only nodes with an interface length of at least 1% of this frame's total interface are counted
		--local nodeFluid = (ifaces[node.Id * 2 + 1] or 0)
		--local totalFluid = metInterfaceFluid[ts] or 0
		--return nodeFluid >= math.min(30, totalFluid * 0.01) and 1 or 0

		-- Only nodes with an area of at least 1% of this frame's total area are counted
		local nodeArea = node.Area
		local totalArea = metArea[ts] or 0
		return nodeArea >= math.min(10, totalArea * 0.01) and 1 or 0
	end)

	makeMetric("Velocity", function (node)
		-- Weigh velocity proportionally to fluid interface of each node
		return node.Velocity * nodeFluidProportion[node.Id + 1]
		--metric[ts] = math.max(metric[ts] or 0, node.Velocity or 0)
	end)
end

local metMainArea = makeMetric("Main Channel Area", function (node)
	return node.Break and node.Area or 0
end)

do
	local ratio = {}
	for k, v in pairs(metArea) do
		if metMainArea[k] then
			ratio[k] = v > 0 and metMainArea[k] / v or 0
		end
	end
	makeMetric("Main Channel Area Ratio", ratio)
end

local minRange = graphData.minRange
local maxRange = graphData.maxRange

if true then
	minRange, maxRange = 0, 1
end

if rightToLeft then
	minRange, maxRange = maxRange, minRange
end

local range = maxRange - minRange

-- ----------------------------------------------------------
-- Node mappers hold information for graph layouts.
-- ----------------------------------------------------------
local maxTimestampHeight = 1

local nodeMappers = {
	{
		posMapper =
			function (node)
				return vector2(node.X / imgW, node.Y / imgH)
			end,
		radMapper =
			function ()
				return 2, 0.5 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				local y = (node.YCount or node.Y2) / maxTimestampHeight / 3 + 0.5
				return vector2((node.Time / frameCnt - minRange) / range, y + 0.01)
			end,
		radMapper =
			function ()
				return 2, 0.1 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				local y = node.Y2 / 40 + 0.5
				return vector2((node.Time / frameCnt - minRange) / range, y + 0.01)
			end,
		radMapper =
			function ()
				return 3, 0.1 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				return vector2((node.Time / frameCnt - minRange) / range, node.Y / imgH)
			end,
		radMapper =
			function ()
				return 1.5, 0.2 * math.sqrt(2)
			end,
	},
}

nodeMapperIndex = 0
nodeMapperTargetIndex = 0

local function getPosMapper(index)
	if index ~= math.floor(index) then
		local map1 = nodeMappers[math.floor(index) % #nodeMappers + 1].posMapper
		local map2 = nodeMappers[math.ceil(index) % #nodeMappers + 1].posMapper
		local fac = index - math.floor(index)
		return function (node)
			return map1(node) * (1 - fac) + map2(node) * fac
		end
	else
		return nodeMappers[index % #nodeMappers + 1].posMapper
	end
end

-- ----------------------------------------------------------
-- Initialize the graph.
-- ----------------------------------------------------------
local nodeColorSplit = color.fromTable {100, 255, 255}
local nodeColorMerge = color.fromTable {255, 160, 100}
local nodeColorBegin = color.fromTable {60, 255, 60}
local nodeColorEnd   = color.fromTable {255, 60, 60}
local nodeColorMulti = color.fromTable {200, 100, 200}
local nodeColorModified = color.fromTable {255, 237, 160}
local nodeColorInvis = color.TRANSPARENT

local nodeBaseRadius = 0
local nodeRadiusFactor = 1

local links
local preBreakLinks

local function getNodeTypeColor(node)
	local nIn, nOut = node.EdgesIn, node.EdgesOut
	
	--[[
	if node.Modified == 1 then -- modified flag set
		return nodeColorModified
	end
	--]]
	
	if nIn == 0 then -- no incoming edges -> source
		return nodeColorBegin
	elseif nOut == 0 then -- no outgoing edges -> sink
		return nodeColorEnd
	elseif nIn == 1 and nOut == 1 then -- one incoming and one outgoing edge -> hide it
		return nodeColorInvis
	elseif nIn == 1 then -- one incoming and multiple outoing edges -> split
		return nodeColorSplit
	elseif nOut == 1 then -- one outgoing and multiple incoming edges -> merge
		return nodeColorMerge
	else -- multiple incoming and outgoing edges -> multiple
		return nodeColorMulti
	end
end

local function initGraph()
	local t
	local y = 0

	local radMapper = nodeMappers[(nodeMapperTargetIndex) % #nodeMappers + 1].radMapper
	if radMapper then
		nodeBaseRadius, nodeRadiusFactor = radMapper()
	end
	
	if math.abs(nodeMapperIndex - nodeMapperTargetIndex) > 0.001 then
		nodeMapperIndex = utils.lerp(nodeMapperIndex, nodeMapperTargetIndex, 0.25)
	else
		nodeMapperIndex = nodeMapperTargetIndex
	end

	local posMapper = getPosMapper(nodeMapperIndex)

	local wSize = vector2(graphWidth, graphHeight)

	local timestampHeight = {}
	local filledSlots = {}

	for _, node in ipairs(nodes) do
		if t ~= node.Time then
			if t then
				timestampHeight[t] = y
			end
			t = node.Time
			y = 0
		else
			y = y + 1
		end
		node.Y2 = y
		node.YCount = false

		-- Node colors
		local hue = (node.Uid * 0.72 + 10.4) * node.Uid * 0.7
		local sat = 0.4 + ((node.Uid * 0.8 + .35) * node.Uid) % 0.5

		node.Color = color.hsv(hue, sat, 1, 1)
		node.Color2 = color.hsv(hue, sat, 0.8, 0.8)
	end

	if t then
		timestampHeight[t] = y
	end

	do
		local left, right = minRange, maxRange
		if left > right then
			left, right = right, left
		end
		maxTimestampHeight = 1
		for k, v in pairs(timestampHeight) do
			if k / frameCnt >= left and k / frameCnt < right then
				maxTimestampHeight = math.max(maxTimestampHeight, v)
			end
		end
	end

	for _, node in ipairs(nodes) do
		node.Y2 = node.Y2 * 2 - (timestampHeight[node.Time] or 0)
	end

	for i = 1, #edges, 2 do
		local src = nodes[edges[i] + 1]
		local dst = nodes[edges[i + 1] + 1]
		if dst.EdgesIn == 1 and dst.EdgesOut == 1 then
			nodes[edges[i + 1] + 1].YCount = src.YCount or src.Y2
		end
	end

	for _, node in ipairs(nodes) do
		local nx, ny = node.Time, node.YCount or node.Y2
		local index = ny + 256 * (nx + 1)
		local occupancy = filledSlots[index]
		if not occupancy then
			filledSlots[index] = 0
		else
			occupancy = occupancy + 1
			filledSlots[index] = occupancy
			node.YCount = ny + math.ceil(occupancy / 2) * 0.5 * (occupancy % 2 - 0.5) * 2
		end
	end

	for _, node in ipairs(nodes) do
		node.Pos = vector2(offsetX, offsetY) + posMapper(node) * wSize
		node.PosOrigSize = posMapper(node) * imgSize

		local col = utils.clamp(0, (node.Time - frameNum) + 0.5, 1)
		local a = col % 1 * 0.5 + 0.75

		node.Rad = color.fade(getNodeTypeColor(node), a)
		node.WIn = 2 + utils.clamp(0, math.sqrt(node.Area) / 16, 8)
		node.WOut = 10 + a * 5
	end

	links = {}
	preBreakLinks = {}
	for i = 1, #edges, 2 do
		local src = nodes[edges[i] + 1]
		local dst = nodes[edges[i + 1] + 1]
		if src.X ~= 0 or src.Y ~= 0 then
			local w = (src.WOut + dst.WOut) * 0.5
			links[#links + 1] = {source = src, dest = dst, weight = w - 10, time = dst.Time}
			if src.Time <= breakthrough then
				preBreakLinks[#preBreakLinks + 1] = links[#links]
			end
		end
	end
end

initGraph()

-- ----------------------------------------------------------
-- Draw node legend.
-- ----------------------------------------------------------
local function drawColorLegend()
	local drawItem = function(offset, text, node_color)
		local y = offsetY + 7 * sizeFactor

		draw.text {
			font = draw.Font.SYSTEM,
			text = text,
			x = offset,
			y = y + 10 * sizeFactor,
			size = sizeFactor * 10,
			fillColor = node_color,
			outlineColor = color.BLACK,
			outlineThickness = 2,
			alignX = 1,
			alignY = 1,
		}
	
		gfx.drawBox({offset + 10 * sizeFactor, y, 30 * sizeFactor, 10 * sizeFactor}, node_color)
	end

	if settings.colorByNodeType then
		local margin = 90
		drawItem(offsetX + (50 + 0 * margin) * sizeFactor, "split", nodeColorSplit)
		drawItem(offsetX + (50 + 1 * margin) * sizeFactor, "merge", nodeColorMerge)
		drawItem(offsetX + (50 + 2 * margin) * sizeFactor, "source", nodeColorBegin)
		drawItem(offsetX + (50 + 3 * margin) * sizeFactor, "sink", nodeColorEnd)
		drawItem(offsetX + (50 + 4 * margin) * sizeFactor, "multi", nodeColorMulti)
	end
end

-- ----------------------------------------------------------
-- Draw selected or all metric(s).
-- ----------------------------------------------------------
local metricYOffset = 0

local function drawMetricFancy(metData)
	local lgs = settings.logScale and math.log
	local colWidth = graphWidth / frameCnt / range
	local fMin = minRange * frameCnt
	local height = metricHeight / (lgs and lgs(metData.maxRaw + 1) or metData.maxRaw)
	local fillColor = color.hsv(metData.id * 0.3 + 0.5, (metData.id * 0.15 + 0.4) % 1, 1)
	local breakColor = color.hsv(metData.id * 0.3 + 0.5, (metData.id * 0.15 + 0.4) % 1, 0.7)
	local raw = metData.raw
	local ys = {}
	local t
	local floor = math.floor
	local min = math.min
	for i, node in ipairs(nodes) do
		local metVal = raw[i]
		if metVal then
			if lgs then
				metVal = lgs(metVal + 1)
			end
			if t ~= node.Time then
				t = node.Time
				ys = {}
			end
			local y = metricYOffset - height * metVal
			local intensity = (ys[floor(y)] or 5) + 1
			ys[floor(y)] = intensity
			gfx.drawBox({offsetX + (t - fMin) * colWidth, y, colWidth, 1},
				color.setA(t > breakthrough and breakColor or fillColor, min(255, intensity * 10) ))
		end
	end
end

local function drawMetricUnfancyLog(raw, metric, metData)
	local lgs = settings.logScale and math.log
	local colWidth = graphWidth / frameCnt / range
	local fMin = minRange * frameCnt

	local heights = {}
	setmetatable(heights, {__index = function() return 0 end})
 
	if raw then
		for _, node in ipairs(nodes) do
			local t = node.Time
			heights[t] = heights[t] + raw[i]
		end
	else
		for i = 0, frameCnt do
			heights[i - 1] = metric[i]
		end
	end

	local max = 0
	for _, v in pairs(heights) do
		max = math.max(max, v)
	end

	for k, v in pairs(heights) do
		v = lgs(v + 1) * metricHeight / lgs(max + 1)

		-- Different color styles
		--local fillColor = color.hsv(metData.id * 0.3 + 0.5, (metData.id * 0.15 + 0.4) % 1, 0.5 + k * frameCnt, settings.activeMetricID and 0.8 or 0.5)
		--local fillColor = color.hsv(4 * 0.3 + 0.5, (4 * 0.15 + 0.4) % 1, 0.5 + k * frameCnt, settings.activeMetricID and 0.8 or 0.5)
		--local fillColor = color.hsv((metData.id * 0.3 + 0.5) % 1, (metData.id * 0.15 + 0.4) % 1, k / frameCnt, settings.activeMetricID and 0.8 or 0.5)
		local fillColor = color.hsv((metData.id * 0.3 + 0.5) % 1, (metData.id * 0.15 + 0.4) % 1, 1, settings.activeMetricID and 0.8 or 0.5)

		gfx.drawBox({offsetX + (k - fMin) * colWidth, metricYOffset, colWidth, -v},
			color.setA(fillColor, k > breakthrough and 180 or 220))
	end
end

local function drawMetricUnfancy(metric, metData)
	if settings.logScale then drawMetricUnfancyLog(metData.raw, nil, metData); return end

	local colWidth = graphWidth / frameCnt / range
	local fMin = minRange * frameCnt
	local raw = metData.raw
	local height = metricHeight / metData.max
	local ys = {}

	for i, node in ipairs(nodes) do
		local metVal = raw[i]
		if metVal then
			local t = node.Time
			local h = height * metVal
			ys[t] = (ys[t] or (metricYOffset - height * (metric[t + 1] or 0))) + h
			gfx.drawBox({offsetX + (t - fMin) * colWidth, ys[t] - h, colWidth, h},
				color.setA(node.Color or -1, t > breakthrough and 180 or 220))
		end
	end
end

local function drawMetricUnfancy2(metric, metData)
	if settings.logScale then drawMetricUnfancyLog(nil, metric, metData); return end

	local colWidth = graphWidth / frameCnt / range
	local fMin = minRange * frameCnt
	local height = metricHeight / metData.max
	local fillColor = color.hsv(metData.id * 0.3 + 0.5, (metData.id * 0.15 + 0.4) % 1, 1, settings.activeMetricID and 0.8 or 0.5)
	local breakColor = color.fade(fillColor, 0.7)
	
	for i = 0, frameCnt do
		local metVal = metric[i]
		if metVal then
			gfx.drawBox({offsetX + (i - 1 - fMin) * colWidth, metricYOffset - height * metVal, colWidth, metVal * height},
				color.setA(fillColor, i > breakthrough + 1 and 180 or 220))
		end
	end
end

local function drawMetric(metric, metData)
	metricYOffset = metricYOffset + metricHeight + 5
	if settings.metricFanciness then
		drawMetricFancy(metData)
	elseif metData.raw and next(metData.raw) then
		drawMetricUnfancy(metric, metData)
	else
		drawMetricUnfancy2(metric, metData)
	end
	
	draw.text {
		font = draw.Font.SYSTEM,
		text = metData.name,
		x = sizeFactor * 20,
		y = sizeFactor * metData.id * 15,
		size = sizeFactor * 12,
		fillColor = color.hsv(metData.id * 0.3 + 0.5, (metData.id * 0.15 + 0.4) % 1 * 0.5, 1, 1),
		outlineColor = color.BLACK,
		outlineThickness = 1,
		alignX = 0,
		alignY = 1,
	}
end

-- ----------------------------------------------------------
-- Draw image for the current frame.
-- ----------------------------------------------------------
frameBuffers = {}

imgCacheDir = nil
imgCache = {}

local frameName = nil
local frameInfo = nil
local numCacheEntries = 25

local function drawImage()
	local imgIndex = frameNum
	local imgPath = imgCache[imgIndex + 1]

	if imgPath then
		-- Load image into framebuffer
		local frameMemIndex = imgIndex % numCacheEntries + 1

		if frameBuffers[fb_id][frameMemIndex].name ~= imgPath then
			frameBuffers[fb_id][frameMemIndex].name = imgPath
			frameBuffers[fb_id][frameMemIndex].buffer.load(imgPath)
		end

		if settings.showDebugInfo then
			frameInfo = frameBuffers[fb_id][frameMemIndex].name .. " - " .. frameBuffers[fb_id][frameMemIndex].buffer.id .. " - " .. fb_id
		else
			frameInfo = nil
		end

		frameName = frameBuffers[fb_id][frameMemIndex].name

		-- Draw image
		local fb = frameBuffers[fb_id][frameMemIndex].buffer
		if fb then
			gfx.drawTintedSprite(fb.id, {offsetX, offsetY, graphWidth, graphHeight}, {0, 0, imgW, imgH}, {255,255,255,255})
		end
	end
end

-- ----------------------------------------------------------
-- Draw graph and interfaces according to user's settings.
-- ----------------------------------------------------------
local nodeCircles = {} -- array of {.position, .radius, .color, .marked}
local linkLines = {} -- array of {.source, .target, .color}
local interfaceRects = {} -- array of {.lower, .larger, .color, .marked}

local function drawNode(node)
	if (settings.hideUnreachedNodes and node.Time > frameNum) then return end

	local nodeRadius = node.WIn * nodeRadiusFactor + nodeBaseRadius

	local alpha = math.max(color.getA(node.Rad), 50)

	-- Mark current frame's nodes
	if node.Time == frameNum then
		draw.circle(node.Pos, sizeFactor * (nodeRadius * 1.1 + 2), settings.colorByNodeType and getNodeTypeColor(node) or node.Color, 30, settings.smoothGraph)
	end

	-- Draw node and emulate black border by drawing a larger, black circle beneath
	draw.circle(node.Pos, sizeFactor * (nodeRadius + 1), color.rgba(0, 0, 0, alpha), 30, settings.smoothGraph)
	draw.circle(node.Pos, sizeFactor * nodeRadius, settings.colorByNodeType and getNodeTypeColor(node) or node.Color2, 30, settings.smoothGraph)

	--draw.text {
	--	font = draw.Font.SYSTEM,
	--	text = node.label,
	--	x = sizeFactor * node.pos.x,
	--	y = sizeFactor * node.pos.y,
	--	size = sizeFactor * nodeRadius * 1.6,
	--	fillColor = draw.Color.BLACK,
	--	shadowColor = {255, 255, 255, 32},
	--	outlineThickness = 0,
	--	alignX = 0.5,
	--	alignY = 0.5,
	--}

	table.insert(nodeCircles, {position = node.PosOrigSize, radius = nodeRadius, color = {color.getRGBA(node.Color2)}, marked = (node.Time == frameNum)})
end

local function drawLink(link)
	if (settings.hideUnreachedNodes and link.time > frameNum) then return end

	local startWeight, endWeight = 1.5 * sizeFactor, 1.5 * sizeFactor

	if not settings.unweightedLinks and nodeMapperTargetIndex % #nodeMappers == 0 then
		startWeight = sizeFactor * link.weight
		endWeight = 0
	end

	draw.line(link.source.Pos, link.dest.Pos, color.hsv(0, 0.0, 0.6, 0.6), startWeight, endWeight, settings.smoothGraph)

	table.insert(linkLines, {source = link.source.PosOrigSize, target = link.dest.PosOrigSize, color = {color.getRGBA(color.hsv(0, 0.0, 0.6, 0.6))}})
end

local function drawActiveInterfaces()
	for _, node in ipairs(nodesByTime[frameNum] or {}) do
		local bbox = graphData.Rects and graphData.Rects[node.Id + 1]
		if bbox then
			local col = node.Color
			local r = {
				offsetX + bbox[1] / imgW * graphWidth,
				offsetY + bbox[2] / imgH * graphHeight,
				(bbox[3] - bbox[1]) / imgW * graphWidth,
				(bbox[4] - bbox[2]) / imgH * graphHeight,
			}
			local margin = sizeFactor * (node.Break and 3 or 1)
			gfx.drawBox({r[1] + margin, r[2], r[3] - margin * 2, margin}, col)
			gfx.drawBox({r[1] + margin, r[2] + r[4] - margin, r[3] - margin * 2, margin}, col)
			gfx.drawBox({r[1], r[2], margin, r[4]}, col)
			gfx.drawBox({r[1] + r[3] - margin, r[2], margin, r[4]}, col)

			table.insert(interfaceRects, {lower = vector2(bbox[1], bbox[2]), larger = vector2(bbox[3], bbox[4]), color = {color.getRGBA(col)}, marked = node.Break})
		end
	end
end

local function drawGraph()
	nodeCircles = {}
	linkLines = {}
	interfaceRects = {}

	for _, link in ipairs(settings.hidePostBreakthrough and preBreakLinks or links) do
		drawLink(link)
	end
	for _, i in ipairs(settings.simplify and (settings.hidePostBreakthrough and preBreakNodes or liveNodes) or allNodes) do
		drawNode(nodes[i])
	end
	if nodeMapperIndex % #nodeMappers == 0 and settings.showInterfaces then
		drawActiveInterfaces()
	end
end

local function hexColor(rgba)
	local hex = "#"
	for i = 1, 3 do
		local minor = rgba[i] % 16
		local major = (rgba[i] - minor) / 16
		hex = hex .. string.format("%x", major) .. string.format("%x", minor)
	end
	return hex
end

printGraph = function ()
	local filename = frameName:gsub("/", "_")
	local timestamp = os.date('%Y-%m-%d-%H-%M-%S')
	filename = filename .. "_" .. timestamp .. ".svg"

	-- Create SVG file from nodeCircles and linkLines
	local svg_graph = svglib:create(imgW, imgH)

	for _, link in ipairs(linkLines) do
		svg_graph:addLine(link.source.x, link.source.y, link.target.x, link.target.y, hexColor(link.color), 4, hexColor(link.color))
	end
	
	for _, node in ipairs(nodeCircles) do
		local radius = 4 * node.radius
		svg_graph:addCircle(radius, node.position.x, node.position.y, "#000000", 3, hexColor(node.color))
		if node.marked then
			local h, s, v, a = color.toHSV(color.rgba(unpack(node.color)))
			s = math.min(1, s * 1.2)
			v = math.min(1, v * 1.2)
			svg_graph:addCircle(radius * 1.2, node.position.x, node.position.y, hexColor({color.getRGBA(color.hsv(h, s, v, a))}), 4, "transparent")
		end
	end
	
	for _, interfaces in ipairs(interfaceRects) do
		svg_graph:addRect(interfaces.lower.x, interfaces.lower.y, interfaces.larger.x - interfaces.lower.x,
			interfaces.larger.y - interfaces.lower.y, hexColor(interfaces.color), interfaces.marked and 6 or 3, "transparent")
	end

	local content = svg_graph:draw()

	local file = io.open(filename, "w")
	if file then
		local success = file:write(content)
		if success then
			file:flush()
		else
			log.error("Unable to write SVG file.")
		end
		file:close()
	else
		log.error("Unable to create SVG file.")
	end
end

-- ----------------------------------------------------------
-- Register render callback for actual rendering.
-- ----------------------------------------------------------
event.render.add("graph2", "vis", function ()
	local needsGraphReload = false

	-- Create image cache when input path changed
	if imgCacheDir ~= imgDir then
		imgCacheDir = imgDir
		imgCache = fileIO.listFiles(imgDir, fileIO.List.FILES, fileIO.List.RECURSIVE, fileIO.List.FULL_PATH)

		local nameSortCache = {}
		local function padNum(n)
			return string.rep("0", 5 - #n) .. n
		end
		for _, name in ipairs(imgCache) do
			nameSortCache[name] = name:gsub("%d+", padNum)
		end

		table.sort(imgCache, function (name1, name2)
			return nameSortCache[name1] < nameSortCache[name2]
		end)

		frameNum = 0
		frameCnt = #imgCache

		if frameBuffers[fb_id] == nil then
			frameBuffers[fb_id] = {}
			for i = 1, numCacheEntries do
				frameBuffers[fb_id][i] = {name = nil, buffer = framebuffer.new(imgW, imgH)}
			end
		end

		needsGraphReload = true
	end
	
	-- Handle keybord events to change settings
	pressedKey()

	-- Scrolling will change the graph layout
	if input.scrollY() < 0 then
		nodeMapperTargetIndex = nodeMapperTargetIndex + 1
	elseif input.scrollY() > 0 then
		nodeMapperTargetIndex = nodeMapperTargetIndex - 1
	end

	if nodeMapperIndex ~= nodeMapperTargetIndex then
		needsGraphReload = true
	end

	-- Handle mouse click events that will change the selected frame
	local colWidth = graphWidth / frameCnt / range
	local curFramePos = offsetX + (frameNum - minRange * frameCnt) * colWidth

	local selectFrame = function(x, click)
		if (x < offsetX) then x = offsetX end
		if (x > graphWidth + offsetX) then x = graphWidth + offsetX end

		local newFrame = math.min(frameCnt - 1, math.floor((x - offsetX) / colWidth + 0.5 + frameCnt * minRange))

		if click or (settings.hidePostBreakthrough and newFrame > breakthrough) then
			if (settings.hidePostBreakthrough and newFrame > breakthrough) then newFrame = breakthrough end
			if frameNum ~= newFrame then
				frameNum = newFrame

				needsGraphReload = true
			end
		end
	end

	selectFrame(input.mouseX(), input.mouseDown(1))
	if input.keyPress("P") or input.keyPress("O") then
		local right = (input.keyPress("P") and not rightToLeft) or (input.keyPress("O") and rightToLeft)
		local nextFramePos = curFramePos + (right and colWidth or (-colWidth))

		selectFrame(nextFramePos, true)
	end

	-- Initialize graph if necessary
	if needsGraphReload then initGraph() end

	-- Draw everything
	if settings.screenshotMode then
		gfx.drawBox({0, 0, gfx.getWidth(), gfx.getHeight()}, {255, 255, 255, 255})
	end

	drawImage()
	drawGraph()
	drawColorLegend()

	-- Draw frame and graph information
	if settings.screenshotMode then
		gfx.drawBox({offsetX + (frameNum - minRange * frameCnt) * colWidth, 0, colWidth, metricHeight + 10}, {0, 0, 0, 255})
	else
		gfx.drawBox({offsetX + (frameNum - minRange * frameCnt) * colWidth, 0, colWidth, metricHeight + 10}, {255, 255, 255, 255})
	end

	draw.text {
		font = draw.Font.SYSTEM,
		text = "Frame: " .. frameNum + 1 .. " / " .. frameCnt,
		x = gfx.getWidth() - sizeFactor * 20,
		y = sizeFactor * 15,
		size = sizeFactor * 12,
		fillColor = color.rgb(100, 150, 255),
		outlineColor = color.BLACK,
		outlineThickness = 2,
		alignX = 1,
		alignY = 1,
	}

	local graphName = frameInfo
	pcall(function () graphName = graphData.graphName end)

	draw.text {
		font = draw.Font.SYSTEM,
		text = graphName,
		x = gfx.getWidth() - sizeFactor * 20,
		y = sizeFactor * 30,
		size = sizeFactor * 12,
		fillColor = color.rgb(255, 255, 255),
		outlineColor = color.BLACK,
		outlineThickness = 2,
		alignX = 1,
		alignY = 1,
	}

	-- Draw metric(s)
	metricYOffset = 0

	if settings.activeMetricID == nil then
		for i=1, #metrics do
			drawMetric(metrics[i], metricData[i])
		end
	end

	local activeMetric = metrics[settings.activeMetricID]
	if activeMetric then
		drawMetric(activeMetric, metricData[settings.activeMetricID])
	end

	-- Debug output
	if settings.showDebugInfo and not settings.screenshotMode then
		draw.text {
			font = draw.Font.SYSTEM,
			text = "Mouse: (" .. input.mouseX() .. ", " .. input.mouseY() .. ")",
			x = gfx.getWidth() - sizeFactor * 20,
			y = sizeFactor * 45,
			size = sizeFactor * 12,
			fillColor = color.rgb(100, 150, 255),
			outlineColor = color.BLACK,
			outlineThickness = 2,
			alignX = 1,
			alignY = 1,
		}
	end
end)
