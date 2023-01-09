local systemEvents = {}

local eventHandlers = {}

systemEvents.Type = {
	TICK = 1,
	EXIT = 2,
	REQUEST_RESOURCE = 3,
}

function systemEvents.registerHandler(eventType, key, func)
	local handlers = eventHandlers[eventType]
	if handlers == nil then
		handlers = {}
		eventHandlers[eventType] = handlers
	end
	handlers[key] = func
end

function systemEvents.fire(eventType, parameter)
	local handlers = eventHandlers[eventType]
	if handlers then
		local success, err = xpcall(function()
			local funcKeys = {}
			for k, _ in pairs(handlers) do
				funcKeys[#funcKeys + 1] = k
			end
			table.sort(funcKeys)
			for _, v in ipairs(funcKeys) do
				handlers[v]()
			end
		end, debug.traceback)
		if not success then
			systemError(err)
		end
	end
end

return systemEvents
