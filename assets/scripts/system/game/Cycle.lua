local systemEvents = require "core.SystemEvents"
local orderedSelector = require "system.events.OrderedSelector"

local cycleSelector

systemEvents.registerHandler(systemEvents.Type.TICK, "50-ClientCycle", function ()
	if cycleSelector == nil then
		cycleSelector = orderedSelector.new(event.cycle, {
			"startUp",
			"autoReload",
			"steam",
			"input",
			"tick",
			"render",
			"errors",
			"performance",
		})
	end
	cycleSelector.fire()
end)

systemEvents.registerHandler(systemEvents.Type.EXIT, "50-ClientExit", function ()
	orderedSelector.new(event.systemExit, {
		"exit",
	}).fire()
end)
