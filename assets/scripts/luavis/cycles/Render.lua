local renderCycle = {}

local errorHandler = require "luavis.debug.ErrorHandler"

local orderedSelector = require "system.events.OrderedSelector"

local errors = require "system.debug.ErrorHandler"
local gfx = require "system.game.Graphics"


local renderSelector = orderedSelector.new(event.render, {
	"clear",
	"background",
	"vis",
	"debugOverlay",
}, {catchErrors = true, perf = true})

function renderCycle.draw()
	renderSelector.fire()
end

event.cycle.add("gameRender", "render", function()
	if not errors.hasError() then
		renderCycle.draw()
	end
end)

event.render.add("clear", "clear", function()
	gfx.clear()
	errorHandler.allowOverdraw()
end)

return renderCycle
