local debug = {}

local draw = require "luavis.vis.Draw"
local renderCycle = require "luavis.cycles.Render"

local input = require "system.game.Input"
local perf = require "system.debug.Performance"
local gfx = require "system.game.Graphics"
local timer = require "system.utils.Timer"
local utils = require "system.utils.Utilities"

overlayEnabled = false
currentDebugOverlay = 1

function debug.setOverlayEnabled(enabled)
	overlayEnabled = enabled
end

function debug.isOverlayEnabled()
	return overlayEnabled
end

function debug.put(...)
	for i = 1, select("#", ...) do
		local obj = select(i, ...)
		if type(obj) ~= "string" then
			obj = utils.inspect(obj)
		end
		debugLines[#debugLines + 1] = obj
	end
end

local function getPerformanceSegments()
	local outputTable = {}
	local function output(segment)
		outputTable[#outputTable + 1] = segment
	end

	local function outputPerf(eventType, count)
		count = count or 5
		local perfList = perf.getPerformanceSortedByTime(eventType)
		for i = 1, math.min(#perfList, count) do
			local v = perfList[i]
			output(string.format("%s: %.3f ms", v.name, v.time / 1000))
		end
	end

	local fps = perf.getFramerate()
	local tickTime = perf.getTickTime()
	local renderTime = perf.getRenderTime()
	local targetTime = perf.getTargetTime()

	output(string.format("FPS: %.2f (CPU: %.2f%%; GPU: %.2f%%)", fps,
		tickTime / targetTime * 100,
		renderTime / targetTime * 100))

	output("")
	outputPerf("tick")
	output("")
	outputPerf("render")

	return outputTable
end

local function drawPerformanceOverlay()
	draw.text {
		font = draw.Font.SYSTEM,
		text = table.concat(getPerformanceSegments(), "\n"),
		x = 0,
		y = 0,
	}
end

event.tick.add("debugKeys", "debugKeys", function ()
	if input.keyPress("F1") then
		overlayEnabled = not overlayEnabled
		perf.setEnabled(overlayEnabled)
	end
	if input.keyPress("F4") then
		draw.setDarkThemeEnabled(not draw.isDarkThemeEnabled())
	end
	if input.keyPress("F9") then
		local frameCount = 1000
		perf.startLuaJITProfiler()
		local tStart = timer.getGlobalTime()
		for i = 1, frameCount do
			renderCycle.draw("3si4m1")
		end
		local tEnd = timer.getGlobalTime()
		perf.stopLuaJITProfiler()
		log.info("Rendered %d frames (%.3f ms per frame)", frameCount, (tEnd - tStart) / 1000 / frameCount)
	end
end)

event.render.add("debugOverlay", "debugOverlay", function ()
	if overlayEnabled then
		drawPerformanceOverlay()
	end
end)

return debug
