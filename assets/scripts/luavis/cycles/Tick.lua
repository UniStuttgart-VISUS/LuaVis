local tick = {}

local errorHandler = require "luavis.debug.ErrorHandler"

local orderedSelector = require "system.events.OrderedSelector"

local errors = require "system.debug.ErrorHandler"
local timer = require "system.utils.Timer"


local tickSelector = orderedSelector.new(event.tick, {
	"debugKeys",
	"networkClient",
	"networkGameState",
	"networkServer",
	"beatSync",
	"rollback",
	"input",
	"gameOver",
	"chat",
	"menu",
	"hud",
	"lobby",
	"countdown",
	"swipes",
	"sound",
	"music",
	"beatBarsCleanup",
	"vision",
	"vis",
}, {catchErrors = true, perf = true})

local microsecsPerTick = 1000000 / 60

deltaTicks = 0
deltaTime = 0
lastTickTime = nil
remainderTime = 0

local function updateTickTimeData()
	local tickTime = timer.getGlobalTime()
	deltaTime = lastTickTime and tickTime - lastTickTime or 0
	deltaTicks = math.floor((remainderTime + deltaTime) / microsecsPerTick)
	remainderTime = (remainderTime + deltaTime) % microsecsPerTick
	lastTickTime = tickTime
end

function tick.process()
	tickSelector.fire({deltaTicks = deltaTicks, deltaTime = deltaTime})
end


event.cycle.add("gameTick", "tick", function()
	updateTickTimeData()

	-- Allow the user to press escape to clear execution errors and continue
	if errors.hasOnlyExecutionErrors() and errorHandler.checkErrorSkipKey() then
		errors.clearExecutionErrors()
		errors.update()
	end

	if not errors.hasError() then
		tick.process()
	end
end)

function tick.milliseconds(millisecs)
	return tick.seconds(millisecs / 1000)
end

function tick.seconds(secs)
	return math.floor(secs * 60)
end

function tick.getDeltaTicks()
	return deltaTicks
end

function tick.getDeltaTime()
	return deltaTime
end

return tick
