local events = {}

local utils = require "system.utils.Utilities"

local eventManager = require "core.Events"


function events.getModificationCount(eventType)
	local eventInfo = eventManager.getEventInfo(eventType) or {}
	return eventInfo.modCount or 0
end

function events.getRegisteredFunctions(eventType)
	local eventInfo = eventManager.getEventInfo(eventType) or {}
	return utils.deepCopy(eventInfo.functions) or {}
end

return events
