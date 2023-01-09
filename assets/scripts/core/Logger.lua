local logLevels = {
	trace = 0,
	debug = 1,
	info = 2,
	warn = 3,
	error = 4,
	critical = 5,
}

return {
	createLoggerForScript = function (scriptName)
		scriptName = string.match(scriptName, ".+%.(.+)") or scriptName
		local logFunctions = {}
		for name, level in pairs(logLevels) do
			logFunctions[name] = function (...)
				bridge.core.log(level, scriptName, string.format(...))
			end
		end
		return setmetatable({}, {
			__index = logFunctions,
			__newindex = function ()
				error("Attempt to modify logger", 2)
			end
		})
	end,
}
