---@diagnostic disable: need-check-nil
local fileIO = require "system.game.FileIO"
local framebuffer = require "system.game.Framebuffer"
local gfx = require "system.game.Graphics"
local input = require "system.game.Input"

local color = require "system.utils.Color"
local utils = require "system.utils.Utilities"
local vector2 = require "system.utils.Vector2"

local draw = require "luavis.vis.Draw"

-- ----------------------------------------------------------
-- Settings to change input dataset and layout.
-- ----------------------------------------------------------
local graphData = require "luavis.vis.graphdata.GraphCa4M1"
local splitScreenRatio = 100 / 540

-- ----------------------------------------------------------
-- Set keyboard events and related settings.
-- ----------------------------------------------------------
local mouseKeys, settings = {}, {}

local function setKey(keys, setting, default, func)
	if type(keys) ~= "table" then
		keys = {keys}
	end

	for _, key in ipairs(keys) do
		mouseKeys[tostring(key)] = {setting = setting, func = func}
	end

	settings[setting] = default
end

local function pressedKey()
	for k, v in pairs(mouseKeys) do
		if (input.keyPress(k)) then
			v.func(k, v.setting)
		end
	end
end

local function toggle(_, setting)
	settings[setting] = not settings[setting]
end

setKey("B", "hidePostBreakthrough", false, toggle)
setKey("N", "hideUnreachedNodes", false, toggle)
setKey("W", "unweightedLinks", false, toggle)
setKey("L", "logScale", false, toggle)
setKey("F", "metricFanciness", false, toggle)
setKey("S", "screenshotMode", false, toggle)
setKey("C", "smoothGraph", false, toggle)
setKey("D", "showDebugInfo", false, toggle)

setKey("M", "activeMetricID", 1, function(_, setting) settings[setting] = nil end)
setKey({1, 2, 3, 4, 5, 6, 7}, "activeMetricID", 1, function(key, setting) settings[setting] = tonumber(key) end)

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

local fb_id = "" .. imgW .. "x" .. imgH

local breakthroughThreshold = 10
pcall(function ()
	breakthroughThreshold = graphData.btt
end)

--- @class Node
--- @class N.Time
--- @class N.Id
--- @class N.X
--- @class N.Y
--- @class N.Vx
--- @class N.Vy
--- @class N.Area
--- @class N.In
--- @class N.Out
--- @class N.Pos
--- @class N.Rad
--- @class N.WIn
--- @class N.WOut
--- @class N.Y2
--- @class N.YCount
--- @class N.Alias
--- @class N.Color
--- @class N.Uid
--- @class N.Parent
--- @class N.Child
--- @class N.Break
--- @class N.Color2

--- @type Node[]
--- @field [1] N.Time
--- @field [2] N.Id
--- @field [3] N.X
--- @field [4] N.Y
--- @field [5] N.Vx
--- @field [6] N.Vy
--- @field [7] N.Area
--- @field [8] N.In
--- @field [9] N.Out
--- @field [10] N.Pos
--- @field [11] N.Rad
--- @field [12] N.WIn
--- @field [13] N.WOut
--- @field [14] N.Y2
--- @field [15] N.YCount
--- @field [16] N.Alias
--- @field [17] N.Color
--- @field [18] N.Uid
--- @field [19] N.Parent
--- @field [20] N.Child
--- @field [21] N.Break
--- @field [22] N.Color2

local nodes = utils.deepCopy(graphData.Nodes)
local edges = utils.deepCopy(graphData.Edges)

local breakthrough = 0

local patchGaps = true
if patchGaps then
	local sinks = {}
	for i = 1, #nodes do
		local node = nodes[i]
		if node[8] == 0 and node[9] == 1 then
			local nearestDistance, nearestIdx = 30 ^ 2, nil
			local x, y = node[3], node[4]
			for idx, sink in ipairs(sinks) do
				local dx, dy = sink[3] - x, sink[4] - y
				local distance = dx * dx + dy * dy
				if distance < nearestDistance then
					nearestDistance = distance
					nearestIdx = idx
				end
			end
			if nearestIdx then
				local nearestSink = table.remove(sinks, nearestIdx)
				node[8] = 1
				nearestSink[9] = 1
				node[19] = nearestSink[2] + 1
				nearestSink[20] = node[2] + 1
				edges[#edges + 1] = nearestSink[2]
				edges[#edges + 1] = node[2]
			end
		elseif node[8] == 1 and node[9] == 0 then
			sinks[#sinks + 1] = node
		end
	end
end

for i = 1, #edges, 2 do
	local n1, n2 = nodes[edges[i] + 1], nodes[edges[i + 1] + 1]
	local parent = nodes[n2[19]]
	local child = nodes[n1[20]]
	if not parent or parent[7] < n1[7] then
		n2[19] = n1[2] + 1
	end
	if not child or child[7] < n2[7] then
		n1[20] = n2[2] + 1
	end
end

local currentUID = 0
local function nextUID()
	currentUID = currentUID + 1
	return currentUID
end

for i = 1, #nodes do
	local node = nodes[i]
	local parent = nodes[node[19]]
	node[18] = parent and parent[20] == i and parent[18] or nextUID()
end

local patchMissingLabels = false
if patchMissingLabels then
	for i = 1, #nodes do
		local node = nodes[i]
		if node[3] == 0 and node[4] == 0 then
			local parent = nodes[node[19]] or nodes[1]
			node[3] = parent[3]
			node[4] = parent[4]
		end
	end
end

local nodesByTime = {}

for i, node in ipairs(nodes) do
	nodesByTime[node[1]] = nodesByTime[node[1]] or {}
	table.insert(nodesByTime[node[1]], node)
end

local function traceMainChannel(node)
	if node then
		node[21] = true
		return traceMainChannel(nodes[node[19]])
	end
end

pcall(function ()
	local rects = graphData.Rects
	if rects then
		for i, r in ipairs(rects) do
			local x = rightToLeft and r[1] or imgW - r[3]
			if x < breakthroughThreshold then
				breakthrough = nodes[i][1]
				traceMainChannel(nodes[i])
				break
			end
		end
	end
end)

local liveNodes = {}
local preBreakNodes = {}

for i = 1, #nodes do
	local node = nodes[i]
	if (node[8] == 1 and node[9] <= 1) or graphData.Rects[i][1] == 0 then
		node[16] = i
	else
		liveNodes[#liveNodes + 1] = i
		if node[1] <= breakthrough then
			preBreakNodes[#preBreakNodes + 1] = i
		end
	end
end

local simplify = true
if simplify then
	for i = #edges - 1, 1, -2 do
		local n1, n2 = nodes[edges[i] + 1], nodes[edges[i + 1] + 1]
		if n1[16] then
			--n1[16] = n2[16] or n2[2] + 1
		end
	end

	for i = 1, #nodes do
		local node = nodes[i]
		if node[16] == i then
			node[16] = nil
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
			local ts = node[1] + 1
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
	return node[7]
end)

local metNumFingers = {}
local nodeFluidProportion = {}

if graphData.Interfaces then
	local ifaces = graphData.Interfaces

	local metInterfaceFluid = makeMetric("Interface length fluid", function (node)
		return ifaces[node[2] * 2 + 1]
	end)

	for i = 1, #nodes do
		local node = nodes[i]
		local ts = node[1] + 1
		local iface = ifaces[node[2] * 2 + 1]
		if metInterfaceFluid[ts] and metInterfaceFluid[ts] > 0 and iface then
			nodeFluidProportion[i] = iface / metInterfaceFluid[ts]
		else
			nodeFluidProportion[i] = 0
		end
	end

	makeMetric("Interface length solid", function (node)
		return ifaces[node[2] * 2 + 2]
	end)

	metNumFingers = makeMetric("Number of fingers", function (node)
		local ts = node[1] + 1

		-- Only nodes with an interface length of at least 1% of this frame's total interface are counted
		--local nodeFluid = (ifaces[node[2] * 2 + 1] or 0)
		--local totalFluid = metInterfaceFluid[ts] or 0
		--return nodeFluid >= math.min(30, totalFluid * 0.01) and 1 or 0

		-- Only nodes with an area of at least 1% of this frame's total area are counted
		local nodeArea = node[7]
		local totalArea = metArea[ts] or 0
		return nodeArea >= math.min(10, totalArea * 0.01) and 1 or 0
	end)

	makeMetric("Velocity", function (node)
		-- Weigh velocity proportionally to fluid interface of each node
		return node[5] * nodeFluidProportion[node[2] + 1]
		--metric[ts] = math.max(metric[ts] or 0, node[5] or 0)
	end)
end

local metMainArea = makeMetric("Main Channel Area", function (node)
	return node[21] and node[7] or 0
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
				return vector2(node[3] / imgW, node[4] / imgH)
			end,
		radMapper =
			function ()
				return 2, 0.5 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				local y = (node[15] or node[14]) / maxTimestampHeight / 3 + 0.5
				return vector2((node[1] / frameCnt - minRange) / range, y + 0.01)
			end,
		radMapper =
			function ()
				return 2, 0.1 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				local y = node[14] / 40 + 0.5
				return vector2((node[1] / frameCnt - minRange) / range, y + 0.01)
			end,
		radMapper =
			function ()
				return 3, 0.1 * math.sqrt(2)
			end,
	},
	{
		posMapper =
			function (node)
				return vector2((node[1] / frameCnt - minRange) / range, node[4] / imgH)
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
local nodeColorInvis = color.TRANSPARENT

local nodeBaseRadius = 0
local nodeRadiusFactor = 1

local links
local preBreakLinks

local function getNodeTypeColor(node)
	local nIn, nOut = node[8], node[9]
	if nIn == 0 then
		return nodeColorBegin
	elseif nOut == 0 then
		return nodeColorEnd
	end
	if nIn == 1 then
		if nOut == 1 then
			return nodeColorInvis
		else
			return nodeColorSplit
		end
	elseif nOut == 1 then
		return nodeColorMerge
	else
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

	for i = 1, #nodes do
		local node = nodes[i]

		if t ~= node[1] then
			if t then
				timestampHeight[t] = y
			end
			t = node[1]
			y = 0
		else
			y = y + 1
		end
		node[14] = y
		node[15] = nil

		-- Node colors
		local hue = (node[18] * 0.72 + 10.4) * node[18] * 0.7
		local sat = 0.4 + ((node[18] * 0.8 + .35) * node[18]) % 0.5
		node[17] = color.hsv(hue, sat, 1, 1)
		node[22] = color.hsv(hue, sat, 0.8, 0.8)
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

	for i = 1, #nodes do
		local node = nodes[i]
		node[14] = node[14] * 2 - (timestampHeight[node[1]] or 0)
	end

	for i = 1, #edges, 2 do
		local src = nodes[edges[i] + 1]
		local dst = nodes[edges[i + 1] + 1]
		if dst[8] == 1 and dst[9] == 1 then
			dst[15] = src[15] or src[14]
		end
	end

	for i = 1, #nodes do
		local node = nodes[i]
		local nx, ny = node[1], node[15] or node[14]
		local index = ny + 256 * (nx + 1)
		local occupancy = filledSlots[index]
		if not occupancy then
			filledSlots[index] = 0
		else
			occupancy = occupancy + 1
			filledSlots[index] = occupancy
			node[15] = ny + math.ceil(occupancy / 2) * 0.5 * (occupancy % 2 - 0.5) * 2
		end
	end

	for i = 1, #nodes do
		local node = nodes[i]

		node[10] = vector2(offsetX, offsetY) + posMapper(node) * wSize

		local col = utils.clamp(0, (node[1] - frameNum) + 0.5, 1)
		local a = col % 1 * 0.5 + 0.75

		node[11] = color.fade(getNodeTypeColor(node), a)
		node[12] = 2 + utils.clamp(0, math.sqrt(node[7]) / 16, 8)
		node[13] = 10 + a * 5
	end

	links = {}
	preBreakLinks = {}
	for i = 1, #edges, 2 do
		local src = nodes[edges[i] + 1]
		local dst = nodes[edges[i + 1] + 1]
		if dst[16] then
			dst = nodes[dst[16]]
		end
		if src[16] == nil and (src[3] ~= 0 or src[4] ~= 0) then
			local w = (src[13] + dst[13]) * 0.5
			links[#links + 1] = {source = src, dest = dst, weight = w - 10, time = dst[1]}
			if src[1] <= breakthrough then
				preBreakLinks[#preBreakLinks + 1] = links[#links]
			end
		end
	end
end

initGraph()

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
	for i = 1, #nodes do
		local metVal = raw[i]
		if metVal then
			if lgs then
				metVal = lgs(metVal + 1)
			end
			if t ~= nodes[i][1] then
				t = nodes[i][1]
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
		for i = 1, #nodes do
			local t = nodes[i][1]
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

	for i = 1, #nodes do
		local metVal = raw[i]
		if metVal then
			local node = nodes[i]
			local t = node[1]
			local h = height * metVal
			ys[t] = (ys[t] or (metricYOffset - height * (metric[t + 1] or 0))) + h
			gfx.drawBox({offsetX + (t - fMin) * colWidth, ys[t] - h, colWidth, h},
				color.setA(node[17] or -1, t > breakthrough and 180 or 220))
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
	if not settings.screenshotMode then
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
end

-- ----------------------------------------------------------
-- Draw image for the current frame.
-- ----------------------------------------------------------
frameBuffers = {}

imgCacheDir = nil
imgCache = {}

local frameName = nil
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
			local w, h = frameBuffers[fb_id][frameMemIndex].buffer.getSize()
			frameName = frameBuffers[fb_id][frameMemIndex].name .. " - " .. frameBuffers[fb_id][frameMemIndex].buffer.id .. " - " .. w .. "x" .. h
		else
			frameName = nil
		end

		-- Draw image
		local fb = frameBuffers[fb_id][frameMemIndex].buffer
		if not fb.isValid() then error("Invalid framebuffer") end
		if fb then
			gfx.drawTintedSprite(fb.id, {offsetX, offsetY, graphWidth, graphHeight}, {0, 0, imgW, imgH}, {255,255,255,255})
		end
	end
end

-- ----------------------------------------------------------
-- Draw graph and interfaces according to user's settings.
-- ----------------------------------------------------------
local function drawNode(node)
	if (settings.hideUnreachedNodes and node[1] > frameNum) then return end

	local nodeRadius = node[12] * nodeRadiusFactor + nodeBaseRadius
	-- Outline
	local alpha = math.max(color.getA(node[11]), 50)
	if node[1] == frameNum then
		draw.circle(node[10], sizeFactor * (nodeRadius * 1.1 + 2), node[17], 30, settings.smoothGraph)
	end
	draw.circle(node[10], sizeFactor * (nodeRadius + 1), color.rgba(0, 0, 0, alpha), 30, settings.smoothGraph)
	draw.circle(node[10], sizeFactor * nodeRadius, node[22], 30, settings.smoothGraph)

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
end

local function drawLink(link)
	if (settings.hideUnreachedNodes and link.time > frameNum) then return end

	local startWeight, endWeight = 1.5 * sizeFactor, 1.5 * sizeFactor

	if not settings.unweightedLinks and nodeMapperIndex % #nodeMappers == 0 then
		startWeight = sizeFactor * link.weight
		endWeight = 0
	end

	draw.line(link.source[10], link.dest[10], color.hsv(0, 0.0, 0.6, 0.6), startWeight, endWeight, settings.smoothGraph)
end

local function drawGraph()
	for _, link in ipairs(settings.hidePostBreakthrough and preBreakLinks or links) do
		drawLink(link)
	end
	for _, i in ipairs(settings.hidePostBreakthrough and preBreakNodes or liveNodes) do
		drawNode(nodes[i])
	end
end

local function drawActiveInterfaces()
	for _, node in ipairs(nodesByTime[frameNum] or {}) do
		local bbox = graphData.Rects and graphData.Rects[node[2] + 1]
		if bbox then
			local col = node[17]
			local r = {
				offsetX + bbox[1] / imgW * graphWidth,
				offsetY + bbox[2] / imgH * graphHeight,
				(bbox[3] - bbox[1]) / imgW * graphWidth,
				(bbox[4] - bbox[2]) / imgH * graphHeight,
			}
			local margin = sizeFactor * (node[21] and 3 or 1)
			gfx.drawBox({r[1] + margin, r[2], r[3] - margin * 2, margin}, col)
			gfx.drawBox({r[1] + margin, r[2] + r[4] - margin, r[3] - margin * 2, margin}, col)
			gfx.drawBox({r[1], r[2], margin, r[4]}, col)
			gfx.drawBox({r[1] + r[3] - margin, r[2], margin, r[4]}, col)
		end
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
	local mouseX = input.mouseX()
	if (mouseX < offsetX) then mouseX = offsetX end
	if (mouseX > graphWidth + offsetX) then mouseX = graphWidth + offsetX end
	local newFrame = math.min(frameCnt - 1, math.floor((mouseX - offsetX) / colWidth + 0.5 + frameCnt * minRange))

	if input.mouseDown(1) or (settings.hidePostBreakthrough and newFrame > breakthrough) then
		if (settings.hidePostBreakthrough and newFrame > breakthrough) then newFrame = breakthrough end
		if frameNum ~= newFrame then
			frameNum = newFrame

			needsGraphReload = true
		end
	end

	-- Initialize graph if necessary
	if needsGraphReload then initGraph() end

	-- Draw everything
	if settings.screenshotMode then
		gfx.drawBox({0, 0, gfx.getWidth(), gfx.getHeight()}, {255, 255, 255, 255})
	end

	drawImage()
	drawGraph()
	drawActiveInterfaces()

	-- Draw frame and graph information
	if not settings.screenshotMode then
		gfx.drawBox({offsetX + (frameNum - minRange * frameCnt) * colWidth, 0, colWidth, metricHeight + 10}, {255, 255, 255, 255})

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

		local graphName = frameName
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
	end

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
