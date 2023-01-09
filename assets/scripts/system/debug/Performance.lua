local performance = {}

local utils = require "system.utils.Utilities"

local luaJITProfiler = require "jit.p"
local luaJITTraceDump = require "jit.dump"


perfEnabled = false

perfCurrent = {}
perfPrevious = {}

function performance.getMemoryUsage(m)
	return bridge.perf.getMemoryUsage(m)
end

function performance.isEnabled()
	return perfEnabled
end

function performance.setEnabled(enabled)
	perfEnabled = enabled
end

function performance.clear(category)
	perfCurrent[category] = {}
end

function performance.add(category, entry, time)
	local categoryTable = perfCurrent[category]
	if categoryTable == nil then
		categoryTable = {}
		perfCurrent[category] = categoryTable
	end
	categoryTable[#categoryTable + 1] = {
		name = entry,
		time = time
	}
end

function performance.getPerformance(category)
	return perfPrevious[category] or {}
end

function performance.getPerformanceSortedByTime(category)
	local result = performance.getPerformance(category)
	table.sort(result, function (a, b)
		return a.time > b.time
	end)
	return result
end

function performance.getFramerate()
	return bridge.perf.getFramerate()
end

function performance.getTickTime()
	return bridge.perf.getTickTime()
end

function performance.getRenderTime()
	return bridge.perf.getRenderTime()
end

function performance.getTargetTime()
	local targetTime = bridge.perf.getTargetTime()
	if targetTime > 0 then
		return targetTime
	else
		return 1 / 60
	end
end

function performance.startLuaJITProfiler(mode)
	luaJITProfiler.start(mode)
end

function performance.stopLuaJITProfiler()
	luaJITProfiler.stop()
end

function performance.startLuaJITTraceDump(mode)
	luaJITTraceDump.on(mode, "LuaJITTraceDump.txt")
end

function performance.stopLuaJITTraceDump()
	luaJITTraceDump.off()
end

event.cycle.add("collectPerformanceInfo", "performance", function ()
	perfPrevious = utils.deepCopy(perfCurrent)
end)

return performance
