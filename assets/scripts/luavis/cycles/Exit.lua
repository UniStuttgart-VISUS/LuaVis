local orderedSelector = require "system.events.OrderedSelector"

event.systemExit.add("exitGame", "exit", function()
	orderedSelector.new(event.exit, {
		"config",
	}).fire()
end)
