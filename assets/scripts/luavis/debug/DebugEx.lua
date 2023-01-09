local input = require "system.game.Input"
local perf = require "system.debug.Performance"
local timer = require "system.utils.Timer"
local vector2 = require "system.utils.Vector2"

local iterationCount = 1000000

local function doValue()
	local x, y = 0, 0
	for i = 1, iterationCount do
		x = x + i * 0.75
		y = y - i * 0.5
	end
	dbg("ignore:", x, y)
end

local function doTable()
	local finalPos = {0, 0}
	for i = 1, iterationCount do
		finalPos = {finalPos[1] + i * 0.75, finalPos[2] - i * 0.5}
	end
	dbg("ignore:", finalPos)
end

local function doVector2()
	local finalPos = vector2(0, 0)
	for i = 1, iterationCount do
		finalPos = (finalPos + vector2(i * 0.75, -i * 0.5))
	end
	dbg("ignore:", finalPos)
end

local function doSingleVector2()
	local finalPos = vector2(0, 0)
	for i = 1, iterationCount do
		finalPos.x = finalPos.x + i * 0.75
		finalPos.y = finalPos.y - i * 0.5
	end
	dbg("ignore:", finalPos)
end

local function measure(name, func)
	--perf.startLuaJITProfiler("3si4m1")
	local startTime = timer.getGlobalTime()
	func()
	local endTime = timer.getGlobalTime()
	--perf.stopLuaJITProfiler()
	log.info("%s took %.2f ms", name, (endTime - startTime) / 1000)
end

event.tick.add("debugKeysEx", "debugKeys", function ()
	if input.keyPress("F10") then
		measure("value", doValue)
		measure("table", doTable)
		measure("vector2", doVector2)
		measure("singleVector2", doSingleVector2)
	end
end)
