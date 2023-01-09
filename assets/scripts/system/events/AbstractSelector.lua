local abstractSelector = {}

local errors = require "system.debug.ErrorHandler"
local performance = require "system.debug.Performance"
local timer = require "system.utils.Timer"

local stackTrace = require "system.debug.StackTrace"
local stackTraceRules = require "system.debug.StackTraceRules"
local funcRegistry = require "system.debug.FunctionNameRegistry"

local xpcall = xpcall


local getTime = timer.getGlobalTime

local function generateFireFunc(eventTypeName, entriesFunc, wrapperFunc, options)
	local perfEnabled = options.perf

	if type(wrapperFunc) == "function" then
		-- TODO Allow combining wrapper functions with event perf measurement and error handling
		if options.catchErrors then
			-- Error catching currently does not work with wrapper functions.
			-- The wrapper must take care of error handling itself.
			error("Invalid combination of error catching and wrapper function for event '" .. eventTypeName .. "'", 4)
		end
		if perfEnabled then
			-- The performance measurement option is ignored when a wrapper function is used. 
			-- This is not fatal, but worth warning the developer about.
			log.warn("Invalid combination of perf measurement and wrapper function for event '%s'", eventTypeName)
		end
		return function (arg, entryParameter)
			-- Call the wrapper function for each entry.
			local entries = entriesFunc(entryParameter)
			for i = 1, #entries do
				wrapperFunc(arg, entries[i])
			end
		end
	end

	if options.catchErrors then
		return function (arg, entryParameter)

			local entries = entriesFunc(entryParameter)
			if perfEnabled and performance.isEnabled() then

				-- Reset performance data
				performance.clear(eventTypeName)

				for i = 1, #entries do
					local entry = entries[i]

					-- Begin performance measurement
					local perfTime = getTime()

					-- Execute function
					local success, err = xpcall(entry.func, stackTrace.traceback, arg)

					-- End performance measurement
					performance.add(eventTypeName, entry.name, getTime() - perfTime)

					-- Report errors
					if not success then
						errors.addExecutionError(entry.script, eventTypeName, entry.name, err)
					end
				end
			else
				for i = 1, #entries do
					local entry = entries[i]

					-- Execute function
					local success, err = xpcall(entry.func, stackTrace.traceback, arg)

					-- Report errors
					if not success then
						errors.addExecutionError(entry.script, eventTypeName, entry.name, err)
					end
				end
			end
		end
	else
		return function (arg, entryParameter)

			local entries = entriesFunc(entryParameter)
			if perfEnabled and performance.isEnabled() then

				-- Reset performance data
				performance.clear(eventTypeName)

				for i = 1, #entries do
					local entry = entries[i]

					-- Begin performance measurement
					local perfTime = getTime()

					-- Execute function
					entry.func(arg)

					-- End performance measurement
					performance.add(eventTypeName, entry.name, getTime() - perfTime)
				end
			else
				for i = 1, #entries do
					entries[i].func(arg)
				end
			end
		end
	end
end

local function makeFireFunc(eventTypeName, entriesFunc, wrapperFunc, options)
	local fireFunc = generateFireFunc(eventTypeName, entriesFunc, wrapperFunc, options)
	funcRegistry.add(fireFunc, eventTypeName)
	return fireFunc
end

function abstractSelector.new(args)
	return setmetatable({}, {
		__index = {
			fire = makeFireFunc(args.eventType.getName(), args.entriesFunc, args.wrapperFunc, args.options or {}),
			getEntries = args.entriesFunc,
			compareEntries = args.compareFunc,
			eventType = args.eventType,
		},
		__newindex = function (tbl, key, value)
			error("Attempt to write to selector")
		end
	})
end

stackTraceRules.add("AbstractSelector", -30, function (trace)
	for i, info in ipairs(trace) do
		-- Locate event-related stack frames
		if info.source == script.sourceName and info.name == "fire" and info.namewhat == "field" then

			-- Apply custom event naming
			info.short_src = "[Selector]"
			info.name = funcRegistry.get(info.func)
			info.namewhat = "event"
			info.currentline = -1

			-- Track offset for modifications to related stack frames
			local offset = -1

			-- Hide event-related 'xpcall' noise from stack trace (only applies to events with error catching)
			if trace[i + offset] and rawequal(trace[i + offset].func, xpcall) then
				trace[i + offset].remove = true
				offset = offset - 1
			end

			-- Apply custom event handler naming
			if trace[i + offset] and type(info.vars.locals.entries) == "table" then
				for _, entry in ipairs(info.vars.locals.entries) do
					if rawequal(entry.func, trace[i + offset].func) then
						trace[i + offset].name = entry.name
						trace[i + offset].namewhat = "handler"
						break
					end
				end
			end
		end
	end
end)

return abstractSelector
