---@diagnostic disable: need-check-nil
local fileIO = require "system.game.FileIO"
local framebuffer = require "system.game.Framebuffer"
local gfx = require "system.game.Graphics"
local input = require "system.game.Input"

local color = require "system.utils.Color"
local timer = require "system.utils.Timer"
local utils = require "system.utils.Utilities"
local vector2 = require "system.utils.Vector2"

local svglib = require "luavis.vis.SVG"
local draw = require "luavis.vis.Draw"

-- ----------------------------------------------------------
-- Settings to change input dataset and layout.
-- ----------------------------------------------------------
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_circular/graph_1_fixed.lua")			-- Circular
local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_octagonal/graph_1_fixed.lua")		-- Octagonal
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_triangular/graph_1_fixed.lua")		-- Triangular

--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-2,M=1/graph_1_fixed.lua")		-- Ca=10-2,M=1
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-2,M=10/graph_1_fixed.lua")		-- Ca=10-2,M=10
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-3,M=0.2/graph_1_fixed.lua")	-- Ca=10-3,M=0.2
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-3,M=1/graph_1_fixed.lua")		-- Ca=10-3,M=1
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-3,M=10/graph_1_fixed.lua")		-- Ca=10-3,M=10
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-4,M=0.2/graph_1_fixed.lua")	-- Ca=10-4,M=0.2
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-4,M=1/graph_1_fixed.lua")		-- Ca=10-4,M=1
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-4,M=10/graph_1_fixed.lua")		-- Ca=10-4,M=10
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-5,M=0.2/graph_1_fixed.lua")	-- Ca=10-5,M=0.2
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-5,M=1/graph_1_fixed.lua")		-- Ca=10-5,M=1
--local graphData = dofile("assets/scripts/luavis/vis/graphs/graph_Ca=10-5,M=10/graph_1_fixed.lua")		-- Ca=10-5,M=10

local fileMap = {
	{ path = "T:/temp/adrian/Dataset1png", dir = "gfx_1", rtl = true },
	{ path = "T:/temp/adrian/NewDataset", dir = "gfx_2", rtl = false },
	{ path = "S:/Daten/Flow/porous-media_experiment/nikos", dir = "gfx_3", rtl = true }
}

local imgDir = "unknown"
local rightToLeft = true
for _, dataset in ipairs(fileMap) do
	local dir = graphData.imgDir
	dir = dir:gsub(dataset.path, dataset.dir)

	if dir:find(dataset.dir) then
		imgDir = dir:match("(" .. dataset.dir .. "/.*/)[^/]*$")
		rightToLeft = dataset.rtl
	end
end

-- ----------------------------------------------------------
-- Set keyboard events and related settings.
-- ----------------------------------------------------------
local mouseKeys = {}
lastKey = ""
lastTime = 0
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
setKey("A", "unscaledNodes", false, toggle)
setKey("W", "unweightedLinks", false, toggle)
setKey("I", "showInterfaces", true, toggle)
setKey("L", "logScale", false, toggle)
setKey("F", "metricFanciness", false, toggle)
setKey("S", "screenshotMode", false, toggle)
setKey("C", "smoothGraph", false, toggle)
setKey("D", "showDebugInfo", false, toggle)
setKey("Z", "animate", false, toggle)

local printGraph -- forward defined function
setKey("G", "printGraph", nil, function() printGraph() end)

setKey("", "activeMetricID", 1, function() end)
setKey({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, "activeMetricID", 1, function(key, name)
	settings[name] = (tonumber(key) == 0 and 10 or tonumber(key)) end)

local exportMetrics -- forward defined function
setKey("M", "exportMetrics", nil, function() exportMetrics() end)

local ForceAtlas2Loop -- forward defined function
setKey("Q", "forceAtlas2Loop", nil, function() ForceAtlas2Loop() end)

-- ----------------------------------------------------------
-- Parse input path and set layout.
-- ----------------------------------------------------------
local splitScreenRatio = 100 / 540

local sizeFactor = gfx.getWidth() / 644

local metricHeight = splitScreenRatio * gfx.getHeight()
local headerHeight = metricHeight + 10 + 20 * sizeFactor

local graphHeight = gfx.getHeight() - headerHeight
local graphWidth = gfx.getWidth() - headerHeight * gfx.getWidth() / gfx.getHeight()
local offsetX = (gfx.getWidth() - graphWidth) / 2
local offsetY = headerHeight

local font = draw.Font.SEGOE_SEMIBOLD

frameCnt = 1
frameNum = 1

local requestGraphReload = false

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
	node["Layouts"] = {
		MainChannel = { Pos = false, Index = 0 },
		SimpleBreakthrough = { Pos = false },
		ForceAtlas2 = { Pos = false, mass = false, old_d = false, d = false },
	}
	node["Parents"] = {}
	node["Children"] = {}
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
	table.insert(src.Children, dest.Id + 1)
	table.insert(dest.Parents, src.Id + 1)
end

--
local simplifiedNodes = {}
local simplifiedEdges = {}

local tempNodes = {}

for i = 1, #edges, 2 do
	local src, dst = nodes[edges[i] + 1], nodes[edges[i + 1] + 1]

	while src.EdgesIn == 1 and src.EdgesOut == 1 do
		src = nodes[src.Parent]
	end
	while dst.EdgesIn == 1 and dst.EdgesOut == 1 do
		dst = nodes[dst.Child]
	end

	tempNodes[src.Id + 1] = src
	tempNodes[dst.Id + 1] = dst

	table.insert(simplifiedEdges, src.Id + 1)
	table.insert(simplifiedEdges, dst.Id + 1)
end

for _, node in pairs(tempNodes) do
	table.insert(simplifiedNodes, node)
end

-- 
local nodesByTime = {}

for _, node in ipairs(nodes) do
	nodesByTime[node.Time] = nodesByTime[node.Time] or {}
	table.insert(nodesByTime[node.Time], node)
end

-- 
local currentUID = 0
local function nextUID()
	currentUID = currentUID + 1
	return currentUID
end

function spairs(t, order)
    -- collect the keys
    local keys = {}
    for k in pairs(t) do keys[#keys+1] = k end

    -- if order function given, sort by it by passing the table and keys a, b,
    -- otherwise just sort the keys 
    if order then
        table.sort(keys, function(a,b) return order(t, a, b) end)
    else
        table.sort(keys)
    end

    -- return the iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end

for _, node in spairs(nodes, function(t, a, b) return t[a].Time < t[b].Time end) do
	local parent = nodes[node.Parent]
	--node.Uid = parent and (parent.Child == (node.Id + 1) and parent.EdgesOut == 1) and parent.Uid or nextUID()
	node.Uid = parent and (parent.Child == (node.Id + 1)) and parent.Uid or nextUID()
end

-- 
local mainChannelLength = 0

local function traceMainChannel(node, index)
	index = index or 0
	if node then
		node.Break = true
		node.Index = index
		if nodes[node.Parent] then
			local distance = math.sqrt((node.X - nodes[node.Parent].X)^2 + (node.Y - nodes[node.Parent].Y)^2)
			mainChannelLength = mainChannelLength + distance
			nodes[node.Parent].Layouts.MainChannel.Pos = node.Layouts.MainChannel.Pos + distance
		end
		return traceMainChannel(nodes[node.Parent], index + 1)
	end
end

local breakthrough = graphData.breakthroughTime

pcall(function ()
	local rects = graphData.Rects
	local breakX = rightToLeft and imgW or 0
	local breakNode = nil
	local comp = rightToLeft and function(a,b) return a < b end or function(a,b) return a > b end
	if rects then
		for i, r in ipairs(rects) do
			local x = rightToLeft and r[1] or r[3]
			if (nodes[i].Time == breakthrough) and comp(x, breakX) then
				breakX = x
				breakNode = nodes[i]
			end
		end
		if breakNode then
			breakNode.Layouts.MainChannel.Pos = rightToLeft and 0 or imgW
			traceMainChannel(breakNode)
		end
	end
end)

--
local allNodes = {}
local liveNodes = {}
local allPreBreakNodes = {}
local livePreBreakNodes = {}

for i, node in ipairs(nodes) do
	allNodes[#allNodes + 1] = i
	if not (node.EdgesIn == 1 and node.EdgesOut == 1) then
		liveNodes[#liveNodes + 1] = i
		if node.Time <= breakthrough then
			livePreBreakNodes[#livePreBreakNodes + 1] = i
		end
	end
	if node.Time <= breakthrough then
		allPreBreakNodes[#allPreBreakNodes + 1] = i
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

local function binOpMax(v1, v2)
	v1 = v1 or 0
	v2 = v2 or 0

	return math.max(v1, v2)
end

local function makeMetric(name, func, binOp, createRaw)
	local metric = {}
	local raw = {}
	if createRaw == nil then createRaw = true end
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
	}
	if createRaw then
		metricData[id].minRaw = minRaw
		metricData[id].maxRaw = maxRaw
		metricData[id].raw = raw
	end
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
		-- because we want to get an average, and not a sum
		return node.Velocity * nodeFluidProportion[node.Id + 1]
	end, binOpAdd, false)
end

if graphData.Velocities then
	local metric = {}
	for ts = 1, graphData.startTime do
		metric[ts] = 0
	end
	for i = 1, #graphData.Velocities do
		local ts = graphData.startTime + i
		metric[ts] = graphData.Velocities[i]
	end

	id = #metrics + 1
	metrics[id] = metric

	local maxValue = -math.huge
	local minValue = math.huge
	for k, v in pairs(metric) do
		minValue = math.min(minValue, v)
		maxValue = math.max(maxValue, v)
	end

	metricData[id] = {
		id = id,
		min = minValue,
		max = maxValue,
		name = "Velocity Distribution",
	}
end

local metMaxEdgesIn = makeMetric("Max. incoming edges", function (node)
	return node.EdgesIn
end, binOpMax, false)

local metMaxEdgesOut = makeMetric("Max. outgoing edges", function (node)
	return node.EdgesOut
end, binOpMax, false)

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
-- ForceAtlas2 graph layout.
-- ----------------------------------------------------------
local FA2Params = {
	scalingRatio = 20,
	gravity = -0.1,
	jitterTolerance = 1,
	baseMass = 1,
}

-- initialize
local outboundCompensation = 0
local center = vector2(imgW / 2, imgH / 2)

for _, node in ipairs(simplifiedNodes) do
	if node.Layouts.MainChannel.Pos then
		node.Layouts.ForceAtlas2.Pos = vector2(node.Layouts.MainChannel.Pos
			/ mainChannelLength, 0.5):multiply(vector2(imgW, imgH))
	else
		node.Layouts.ForceAtlas2.Pos = vector2(node.X, node.Y)
		-- todo: better start layout?
	end

	node.Layouts.ForceAtlas2.mass = FA2Params.baseMass + node.EdgesIn + node.EdgesOut
	node.Layouts.ForceAtlas2.old_d = vector2(0, 0)
	node.Layouts.ForceAtlas2.d = vector2(0, 0)

	outboundCompensation = outboundCompensation + node.Layouts.ForceAtlas2.mass
end

outboundCompensation = outboundCompensation / #simplifiedNodes

-- functions for calculating repulsing and attracting forces
local function repulsionNodeNode(n1, n2)
	local dist = n1.Layouts.ForceAtlas2.Pos:subtract(n2.Layouts.ForceAtlas2.Pos)
	local distance = dist:length() - (n1.WIn + n1.WOut + n2.WIn + n2.WOut) -- 1.5 is the approximate radius of a node

	local factor = FA2Params.scalingRatio * n1.Layouts.ForceAtlas2.mass
			* n2.Layouts.ForceAtlas2.mass

	if distance > 0 then
		factor = factor / (distance * distance);
	elseif distance < 0 then
		factor = factor * 100
	end

    n1.Layouts.ForceAtlas2.d = n1.Layouts.ForceAtlas2.d:add(dist:multiply(factor))
    n2.Layouts.ForceAtlas2.d = n2.Layouts.ForceAtlas2.d:subtract(dist:multiply(factor))
end

local function repulsionGravity(n)
	local dist = vector2(0, n.Layouts.ForceAtlas2.Pos.y - center.y)
	local distance = dist:length()

	if distance > 0 then
		local factor = FA2Params.scalingRatio * n.Layouts.ForceAtlas2.mass
			* FA2Params.gravity / distance
		
		n.Layouts.ForceAtlas2.d = n.Layouts.ForceAtlas2.d:subtract(dist:multiply(factor))
	end
end

local function attractionNodeNode(n1, n2)
	local dist = n1.Layouts.ForceAtlas2.Pos:subtract(n2.Layouts.ForceAtlas2.Pos)
	local distance = dist:length() - (n1.WIn + n1.WOut + n2.WIn + n2.WOut) -- 1.5 is the approximate radius of a node

	if distance > 0 then
		local factor = -outboundCompensation / (0.5 * (n1.Layouts.ForceAtlas2.mass + n2.Layouts.ForceAtlas2.mass))

		n1.Layouts.ForceAtlas2.d = n1.Layouts.ForceAtlas2.d:add(dist:multiply(factor))
		n2.Layouts.ForceAtlas2.d = n2.Layouts.ForceAtlas2.d:subtract(dist:multiply(factor))
	end
end

local function ForceAtlas2()
	-- initialize
	local speed = 1
	local speedEfficiency = 1

	for _, node in ipairs(simplifiedNodes) do
		node.Layouts.ForceAtlas2.old_d = node.Layouts.ForceAtlas2.d
		node.Layouts.ForceAtlas2.d = vector2(0, 0)
	end

	-- get forces
	for i = 2, #simplifiedNodes do
		for j = 1, i - 1 do
			repulsionNodeNode(simplifiedNodes[i], simplifiedNodes[j])
		end
	end

	for _, node in ipairs(simplifiedNodes) do
		repulsionGravity(node)
	end

	for i = 1, #simplifiedEdges, 2 do
		attractionNodeNode(nodes[simplifiedEdges[i]], nodes[simplifiedEdges[i + 1]])
	end
	
	-- adjust speed
	local totalSwinging = 0
    local totalEffectiveTraction = 0
    for _, node in ipairs(simplifiedNodes) do
        if (not node.Layouts.MainChannel.Pos) then
            local swinging = node.Layouts.ForceAtlas2.old_d:subtract(node.Layouts.ForceAtlas2.d):length()
            totalSwinging = totalSwinging + node.Layouts.ForceAtlas2.mass * swinging;
            totalEffectiveTraction = totalEffectiveTraction + node.Layouts.ForceAtlas2.mass * 0.5
				* node.Layouts.ForceAtlas2.old_d:add(node.Layouts.ForceAtlas2.d):length()
        end
    end

	local estimatedOptimalJitterTolerance = 0.05 * math.sqrt(#simplifiedNodes);
    local minJT = math.sqrt(estimatedOptimalJitterTolerance);
    local jt = FA2Params.jitterTolerance * math.max(minJT, math.min(10,
		estimatedOptimalJitterTolerance * totalEffectiveTraction / math.pow(#simplifiedNodes, 2)));

	if totalSwinging / totalEffectiveTraction > 2.0 then
        if speedEfficiency > 0.05 then
            speedEfficiency = speedEfficiency * 0.5
        end

        jt = math.max(jt, FA2Params.jitterTolerance);
    end

    local targetSpeed = jt * speedEfficiency * totalEffectiveTraction / totalSwinging;

	if totalSwinging > jt * totalEffectiveTraction then
        if speedEfficiency > 0.05 then
            speedEfficiency = speedEfficiency * 0.7
        end
    elseif speed < 1000 then
        speedEfficiency = speedEfficiency * 1.3;
    end

	speed = speed + math.min(targetSpeed - speed, 0.5 * speed);

	-- apply forces
	for _, node in ipairs(simplifiedNodes) do
		if (not node.Layouts.MainChannel.Pos) then
			local swinging = node.Layouts.ForceAtlas2.mass *
				node.Layouts.ForceAtlas2.old_d:subtract(node.Layouts.ForceAtlas2.d):length()
			local factor = 0.1 * speed / (1 + math.sqrt(speed * swinging))

			local df = node.Layouts.ForceAtlas2.d:length()
			factor = math.min(factor * df, 10) / df

			node.Layouts.ForceAtlas2.Pos = node.Layouts.ForceAtlas2.Pos:add(
				node.Layouts.ForceAtlas2.d:multiply(factor))
		end
	end
end

-- call ForceAtlas2 until convergence
ForceAtlas2Loop = function()
	for i = 1, 100 do
		ForceAtlas2()
	end
	requestGraphReload = true
end

-- ----------------------------------------------------------
-- Node mappers hold information for graph layouts.
-- ----------------------------------------------------------
local maxTimestampHeight = 1
local getPosMapper = nil

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
		interpolatable = true,
		simplifiedOnly = false,
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
		interpolatable = true,
		simplifiedOnly = false,
	},
	{
		posMapper =
			function (node)
				return vector2(((node.Time - (rightToLeft and graphData.endTime or graphData.startTime))
					/ (graphData.endTime - graphData.startTime)) * (rightToLeft and (-1) or 1), node.Y / imgH)
			end,
		radMapper =
			function ()
				return 2, 0.5 * math.sqrt(2)
			end,
		interpolatable = true,
		simplifiedOnly = false,
	},
	{
		posMapper =
			function(node)
				return node.Layouts.ForceAtlas2.Pos and node.Layouts.ForceAtlas2.Pos:divide(vector2(imgW, imgH)) or 0
			end,
		radMapper =
			function ()
				return 1, 0.2 * math.sqrt(2)
			end,
		interpolatable = false,
		simplifiedOnly = true,
	},
	{
		posMapper =
			function(node)
				if not node then return vector2(0, 0) end
				
				if not node.Layouts.SimpleBreakthrough.Pos then
					if node.Break then
						node.Layouts.SimpleBreakthrough.Pos =
							vector2(node.Layouts.MainChannel.Pos / mainChannelLength, 0.1)
					else
						local parent = nodes[node.Parent]
						node.Layouts.SimpleBreakthrough.Pos = getPosMapper(4)(parent)
							+ vector2(0, (node.EdgesIn == 1 and node.EdgesOut == 1) and 0 or 0.03)
					end
				end
				
				return node.Layouts.SimpleBreakthrough.Pos
			end,
		radMapper =
			function ()
				return 1, 0.2 * math.sqrt(2)
			end,
		interpolatable = true,
		simplifiedOnly = false,
	},
}

nodeMapperIndex = 0
nodeMapperTargetIndex = 0

getPosMapper = function(index)
	if index ~= math.floor(index) then
		local mapper1 = nodeMappers[math.floor(index) % #nodeMappers + 1]
		local mapper2 = nodeMappers[math.ceil(index) % #nodeMappers + 1]
		if mapper1.interpolatable and mapper2.interpolatable then
			local fac = index - math.floor(index)
			return function (node)
				return mapper1.posMapper(node) * (1 - fac) + mapper2.posMapper(node) * fac
			end,
			function ()
				local a1, b1 = mapper1.radMapper()
				local a2, b2 = mapper2.radMapper()
				return a1 * (1 - fac) + a2 * fac, b1 * (1 - fac) + b2 * fac
			end,
			false
		else
			if mapper1.interpolatable then
				return mapper2.posMapper, mapper2.radMapper, mapper2.simplifiedOnly
			else
				return mapper1.posMapper, mapper1.radMapper, mapper1.simplifiedOnly
			end
		end
	else
		local mapper = nodeMappers[index % #nodeMappers + 1]
		return mapper.posMapper, mapper.radMapper, mapper.simplifiedOnly
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
local nodeColorInvis = color.fromTable {127, 127, 127, 70}

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

	if math.abs(nodeMapperIndex - nodeMapperTargetIndex) > 0.001 then
		nodeMapperIndex = utils.lerp(nodeMapperIndex, nodeMapperTargetIndex, 0.25)
	else
		nodeMapperIndex = nodeMapperTargetIndex
	end

	local posMapper, radMapper = getPosMapper(nodeMapperIndex)
	if radMapper then
		nodeBaseRadius, nodeRadiusFactor = radMapper()
	end
	
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
		math.randomseed(node.Uid)

		local hue = math.random()
		local sat = 0.5 + math.random() % 0.5
		local val = 0.8 + math.random() % 0.2

		node.Color = color.hsv(hue, sat, val, 1)
		node.Color2 = color.hsv(hue, sat, 0.8 * val, 0.8)
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
			if dst.Time <= breakthrough then
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

		gfx.drawBox({offset + 10 * sizeFactor, y, 30 * sizeFactor, 10 * sizeFactor}, node_color)

		draw.text({
			font = font,
			text = text,
			x = offset,
			y = y + 10 * sizeFactor,
			size = sizeFactor * 10,
			fillColor = node_color,
			alignX = 1,
			alignY = 1,
		})
	
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
local metricYOffset = 20 * sizeFactor

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
		for i, node in ipairs(nodes) do
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
			color.setA(fillColor, k ~= frameNum and 70 or 255))
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
				color.setA(settings.colorByNodeType and getNodeTypeColor(node) or node.Color, t ~= frameNum and 70 or 255))
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
				color.setA(fillColor, i - 1 ~= frameNum and 70 or 255))
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
		font = font,
		text = metData.name,
		x = offsetX,
		y = sizeFactor * 15,
		size = sizeFactor * 12,
		fillColor = color.rgb(100, 150, 255),
		alignX = 0,
		alignY = 1,
	}
	
	draw.text {
		font = font,
		text = "0 —",
		x = offsetX - sizeFactor * 5,
		y = headerHeight + sizeFactor * 2,
		size = sizeFactor * 10,
		fillColor = color.rgb(100, 150, 255),
		alignX = 1,
		alignY = 1,
	}
	
	draw.text {
		font = font,
		text = "" .. ((math.floor(metData.max) == metData.max) and metData.max
			or string.format("%.2f", metData.max)) .. " —",
		x = offsetX - sizeFactor * 5,
		y = sizeFactor * 25,
		size = sizeFactor * 10,
		fillColor = color.rgb(100, 150, 255),
		alignX = 1,
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

	local nodeRadius = (settings.unscaledNodes and 1 or node.WIn) * nodeRadiusFactor + nodeBaseRadius

	local alpha = math.max(color.getA(node.Rad), 50)

	-- Mark current frame's nodes
	if node.Time == frameNum then
		draw.circle(node.Pos, sizeFactor * (nodeRadius * 1.1 + 2), settings.colorByNodeType and getNodeTypeColor(node) or node.Color, 30, settings.smoothGraph)
	end

	-- Draw node and emulate black border by drawing a larger, black circle beneath
	draw.circle(node.Pos, sizeFactor * (nodeRadius + 1), color.rgba(0, 0, 0, alpha), 30, settings.smoothGraph)
	draw.circle(node.Pos, sizeFactor * nodeRadius, settings.colorByNodeType and getNodeTypeColor(node) or node.Color2, 30, settings.smoothGraph)

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
			local col = settings.colorByNodeType and getNodeTypeColor(node) or node.Color
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

	local _, _, simplified = getPosMapper(nodeMapperIndex)

	if not simplified then
		for _, link in ipairs(settings.hidePostBreakthrough and preBreakLinks or links) do
			drawLink(link)
		end
		for _, i in ipairs((settings.simplify and (settings.hidePostBreakthrough and livePreBreakNodes or liveNodes))
				or (settings.hidePostBreakthrough and allPreBreakNodes or allNodes)) do

			drawNode(nodes[i])
		end
		if nodeMapperIndex % #nodeMappers == 0 and settings.showInterfaces then
			drawActiveInterfaces()
		end
	else
		for i = 1, #simplifiedEdges, 2 do
			local src = nodes[simplifiedEdges[i]]
			local dst = nodes[simplifiedEdges[i + 1]]
			if src.X ~= 0 or src.Y ~= 0 then
				local w = (src.WOut + dst.WOut) * 0.5
				drawLink({source = src, dest = dst, weight = w - 10, time = dst.Time})
			end
		end
		for _, node in ipairs(simplifiedNodes) do
			drawNode(node)
		end
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
-- Export metrics to CSV file.
-- ----------------------------------------------------------
exportMetrics = function ()
	local filename = frameName:gsub("/", "_") .. ".csv"

	-- Gather metrics and store in CSV file format
	local content = ""
	for ts = graphData.startTime + 1, graphData.endTime + 1 do
		content = content .. "," .. ts
	end
	content = content .. "\n"

	for i=1, #metrics do
		local metric = metrics[i]
		local name = metricData[i].name

		content = content .. name
		for ts = graphData.startTime + 1, graphData.endTime + 1 do
			if metric[ts] then
				content = content .. "," .. metric[ts]
			else
				content = content .. "," .. 0
			end
		end
		content = content .. "\n"
	end

	-- Write file content
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
	local needsGraphReload = requestGraphReload
	requestGraphReload = false

	-- Create image cache when input path changed
	if imgCacheDir ~= imgDir then
		local files = fileIO.listFiles(imgDir, fileIO.List.FILES, fileIO.List.FULL_PATH)
		
		imgCacheDir = imgDir
		imgCache = {}
		for _, name in ipairs(files) do
			if string.sub(name, -3, -1) == "png" then
				table.insert(imgCache, name)
			end
		end

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
		if (settings.hidePostBreakthrough and newFrame > breakthrough) then newFrame = breakthrough end

		if click or (settings.hidePostBreakthrough and frameNum > breakthrough) then
			if frameNum ~= newFrame then
				frameNum = newFrame

				needsGraphReload = true
			end
		end
	end

	if settings.animate then
		frameNum = (frameNum + 1) % frameCnt
	else
		selectFrame(input.mouseX(), input.mouseDown(1))
	end
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

	-- Draw last pressed key, fading out
	if input.text() and input.text()[1] then
		lastKey = string.char(input.text()[1])
		lastTime = timer.getGlobalTime()
	end

	local fadeColor = color.rgba(180, 120, 50, math.min(255, math.max(0, 255 - 0.0005 * (timer.getGlobalTime() - lastTime))))

	draw.circle(vector2(offsetX + graphWidth + 38 * sizeFactor, sizeFactor * 50), sizeFactor * 10, fadeColor, 100, false)

	draw.text {
		font = font,
		text = lastKey:upper(),
		x = offsetX + graphWidth + 31 * sizeFactor,
		y = sizeFactor * 85,
		size = sizeFactor * 20,
		fillColor = fadeColor,
		alignX = 0,
		alignY = 1,
	}

	-- Draw frame and graph information
	if settings.screenshotMode then
		gfx.drawBox({offsetX + (frameNum - minRange * frameCnt) * colWidth, 20 * sizeFactor, colWidth, metricHeight + 10}, {180, 180, 180, 255})
	else
		gfx.drawBox({offsetX + (frameNum - minRange * frameCnt) * colWidth, 20 * sizeFactor, colWidth, metricHeight + 10}, {127, 127, 127, 255})
	end

	draw.text {
		font = font,
		text = "Frame: " .. frameNum + 1 .. " / " .. frameCnt,
		x = offsetX + graphWidth,
		y = sizeFactor * 15,
		size = sizeFactor * 12,
		fillColor = color.rgb(100, 150, 255),
		alignX = 1,
		alignY = 1,
	}

	local graphName = frameInfo
	pcall(function () graphName = graphData.graphName end)

	draw.text {
		font = font,
		text = graphName or (settings.showDebugInfo and imgDir or ""),
		x = offsetX + graphWidth,
		y = sizeFactor * 30,
		size = sizeFactor * 12,
		fillColor = color.rgb(255, 255, 255),
		alignX = 1,
		alignY = 1,
	}

	-- Draw metric(s)
	metricYOffset = 20 * sizeFactor

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
			font = font,
			text = "Mouse: (" .. input.mouseX() .. ", " .. input.mouseY() .. ")",
			x = offsetX + graphWidth,
			y = sizeFactor * 45,
			size = sizeFactor * 12,
			fillColor = color.rgb(100, 150, 255),
			alignX = 1,
			alignY = 1,
		}
	end
end)
