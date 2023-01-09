local errors = require "system.debug.ErrorHandler"

local orderedSelector = require "system.events.OrderedSelector"


startupDone = false

event.cycle.add("gameStartup", "startUp", function()
	if not startupDone and not errors.hasError() then
		orderedSelector.new(event.startup, {
			"theme",
			"config",
			"gameContent",
			"keyConfig",
			"hud",
			"menu",
			"tileMap",
			"vision",
			"presentation",
		}, {catchErrors = true, perf = true}).fire()
		-- TODO: check for errors more cleanly
		if not errors.hasError() then
			startupDone = true
		end
	end
end)
