local enumSelector = require "system.events.EnumSelector"
local array = require "system.utils.Array"
local stringUtils = require "system.utils.StringUtilities"
local stackTrace = require "system.debug.StackTrace"

local resourceSelector = enumSelector.new(event.loadVirtualResource)

bridge.res.addFunctionSource("virtual", "virtual", -1, function (ev)
	local success, err = xpcall(function ()
		local args = {}
		if type(ev.name) == "string" then
			ev.name, args = stringUtils.parseQueryString(ev.name)
		end
		if ev.event == "load" then
			local parameter = {
				name = ev.name,
				args = args,
			}
			resourceSelector.fire(parameter, ev.name)
			ev.data = array.isArray(parameter.data) and parameter.data.id or nil
		elseif ev.event == "check" then
			-- TODO extract to function
			local entries = resourceSelector.getEntries()
			ev.exists = false
			for i = 1, #entries do
				if entries[i].name == ev.name then
					ev.exists = true
					break
				end
			end
		elseif ev.event == "list" then
			-- TODO fix
			local entries = resourceSelector.getEntries()
			ev.list = {}
			for i = 1, #entries do
				local name = entries[i].name
				if type(name) == "string" then
					ev.list[#ev.list + 1] = name
				end
			end
		end
	end, stackTrace.traceback)

	if not success then
		log.error("Error while loading resource '%s': %s", ev.name, err)
	end
end)
