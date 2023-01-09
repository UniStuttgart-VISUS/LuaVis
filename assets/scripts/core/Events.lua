local coreutils = require "core.CoreUtils"

local C = ffi.C

local modCount = 0
local eventTypes = {}
local scriptEvents = {}

local function addScriptMapping(scriptName, eventTypeKey, eventName)

	local eventTypeTable = eventTypes[eventTypeKey]
	if eventTypeTable ~= nil then
		local func = eventTypeTable.functions[eventName]
		if func ~= nil and func.script ~= scriptName then
			error("Event '" .. eventName .. "' in '" .. eventTypeKey ..
				"' is already defined by script '" .. func.script .. "'", 3)
		end
	end

	local scriptEventTypes = scriptEvents[scriptName]
	if scriptEventTypes == nil then
		scriptEventTypes = {}
		scriptEvents[scriptName] = scriptEventTypes
	end
	local scriptEvents = scriptEventTypes[eventTypeKey]
	if scriptEvents == nil then
		scriptEvents = {}
		scriptEventTypes[eventTypeKey] = scriptEvents
	end
	scriptEvents[eventName] = true
end

local function removeScriptMapping(scriptName, eventTypeKey, eventName)
	local scriptEventTypes = scriptEvents[scriptName]
	if scriptEventTypes ~= nil then
		local scriptEvents = scriptEventTypes[eventTypeKey]
		if scriptEvents ~= nil then
			scriptEvents[eventName] = nil
		end
	end
end

local function removeScriptMappings(scriptName)
	local events = scriptEvents[scriptName]
	if events ~= nil then
		for eventTypeKey, eventNames in pairs(events) do
			local eventType = eventTypes[eventTypeKey]
			if eventType ~= nil then
				for eventName, _ in pairs(eventNames) do
					eventType.functions[eventName] = nil
					removeScriptMapping(scriptName, eventTypeKey, eventName)
				end
			end
		end
		scriptEvents[scriptName] = nil
	end
end

local function clear()
	eventTypes = {}
	scriptEvents = {}
end

local function getEventInfo(eventTypeKey)
	return eventTypes[eventTypeKey]
end

local function incrementModCount()
	modCount = modCount + 1
	return modCount
end

local function getGlobalProxy(scriptName)
	return setmetatable({}, {

		__index = function (tbl, eventTypeKey)
			local eventType = eventTypes[eventTypeKey]
			if eventType == nil then
				eventType = {
					name = eventTypeKey,
					modCount = incrementModCount(),
					functions = {},
					proxies = {},
					generateProxy = function (scriptName)
						return coreutils.readOnlyTable({
							add = function(name, key, func)
								if func == nil then
									removeScriptMapping(scriptName, eventTypeKey, name)
								else
									addScriptMapping(scriptName, eventTypeKey, name)
								end
								eventType.functions[name] = {script = scriptName, key = key, func = func}
								eventType.modCount = incrementModCount()
							end,
							getName = function ()
								return eventTypeKey
							end,
						})
					end
				}
				eventTypes[eventTypeKey] = eventType
			end
			local proxy = eventType.proxies[scriptName]
			if proxy == nil then
				proxy = eventType.generateProxy(scriptName)
				eventType.proxies[scriptName] = proxy
			end
			return proxy
		end,

		__newindex = function (tbl, key, value)
			error("Attempt to write directly to event table")
		end

	})
end

return {
	getGlobalProxy = getGlobalProxy,
	removeScriptMappings = removeScriptMappings,
	clear = clear,
	getEventInfo = getEventInfo,
}
