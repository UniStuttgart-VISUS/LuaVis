local function init()

	if luaInitializationPerformed == nil then

		-- Set to false first.
		-- This value indicates an initialization error, since the assignment to "true" never runs in that case.
		luaInitializationPerformed = false

		-- Error reporting function.
		-- Prevents scripts from being called in further ticks without interrupting current executions
		function systemError(err)
			bridge.core.showError(err)
		end

		-- Replacement for "loadfile" to use World of Sand resource manager.
		function loadfile(scriptName)
			local result = bridge.script.load(scriptName)
			local t = type(result)
			if t == "string" then
				return nil, result
			elseif t == "function" then
				return result
			else
				return nil, "Invalid type '" .. t .. "' returned for script '" .. scriptName .. "'"
			end
		end

		-- Forward Lua standard library include calls to require, use custom script loader function for anything else.
		local origRequire = require

		-- Cache the Lua standard libraries into tables
		local luaLibNames = {"package", "coroutine", "string", "os", "math", "table", "debug", "bit", "io", "ffi", "jit"}
		local luaLibs = {}
		for _, libName in ipairs(luaLibNames) do
			if luaLibs[libName] == nil then
				local lib = _G[libName] or require(libName)
				if lib == nil then
					error("Failed to initialize library " .. libName)
				else
					luaLibs[libName] = lib
				end
			end
		end

		-- Add libs from package.preload
		for libName, lib in pairs(package.preload) do
			luaLibs[libName] = lib()
		end

		local includedScripts = {}

		-- Replacement for "require"
		function require(script, forceReload)
			if luaLibs[script] ~= nil then
				return luaLibs[script]
			else
				local includedScript = includedScripts[script]
				if includedScript == nil or forceReload == true then
					-- LuaJIT's utility libraries need to be loaded using the built-in require function
					local scriptFile = script
					local jitScriptName = string.match(scriptFile, "^jit%.(.+)")
					if jitScriptName then
						scriptFile = "core.jit." .. jitScriptName
					end
					local func, err = loadfile(scriptFile)
					if err then
						error(err, 2)
					else
						includedScript = func()
						includedScripts[script] = includedScript
					end
				end
				return includedScript
			end
		end

		-- Initialize FFI API
		ffi.cdef(bridge.script.getFFIHeader())

		-- Add global debug output function
		dbg = require("core.Debug")

		-- Prevent invalid global variable accesses
		require("core.ReadOnlyGlobal")

		-- Load system event handler and register it for event handling
		local systemEvents = require("core.SystemEvents")
		bridge.script.setEventFunction(systemEvents.fire)

		-- Load other scripts
		local scriptLoader = require("core.ScriptLoader")
		scriptLoader.loadAll()

		-- Generates a function that executes a specific table of functions in order of their keys
		local function generateCallbackFunction(functions)
			return function ()
				local success, err = xpcall(function()
					local funcKeys = {}
					for k, _ in pairs(functions) do
						funcKeys[#funcKeys + 1] = k
					end
					table.sort(funcKeys)
					for _, v in ipairs(funcKeys) do
						functions[v]()
					end
				end, debug.traceback)
				if not success then
					systemError(err)
				end
			end
		end

		-- Enable JIT trace visualization.
		--require("jit.dump").start()

		-- Enable JIT profiler.
		--require("jit.p").start("F")

		-- Initialization successful
		luaInitializationPerformed = true

	elseif luaInitializationPerformed == false then

		-- Don't load anything if the Lua initialization failed.
		error("Initialization failed; Lua state needs to be reset")

	end

end

local success, err = xpcall(init, debug.traceback)
if not success and systemError then
	systemError(err)
end

return success
