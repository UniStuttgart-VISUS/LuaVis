local scriptLoader = {}

local coreutils = require "core.CoreUtils"
local events = require "core.Events"
local logger = require "core.Logger"

-- Contains info about each loaded script
local loadedScripts = {}

-- Contains the global variable storage for each script
local globalVariables = {}

-- Contains error information for each script
local loadErrors = {}

-- Contains all scripts that failed as a result of a cascading script load
local cascadedLoadErrors = {}

-- Dependency graph for load-time intermodular calls between scripts
local dependencyGraph = require("core.DependencyGraph").new()

-- True if a script is currently being loaded, false at runtime
local loadInProgress = false

-- Contains a list of names to be exported to the virtual global table for all scripts
local commonVirtualGlobals = {
	"assert",
	"dbg",
	"error",
	"ipairs",
	"next",
	"pairs",
	"pcall",
	"select",
	"setmetatable",
	"tonumber",
	"tostring",
	"type",
	"unpack",
	"xpcall",
}

-- Contains a list of names to be exported to the virtual global table for privileged ("System/*") scripts
local privilegedVirtualGlobals = {
	"bridge",
	"load",
	"loadstring",
	"setfenv",
	"getfenv",
	"getmetatable",
	"collectgarbage",
	"rawset",
	"rawget",
	"rawequal",
	"newproxy",
}

-- Contains a list of libraries includable using "require" and auto-loaded in the global table
local commonLibraries = {
	"string",
	"math",
	"table",
	"bit",
}

-- Contains a list of libraries includable using "require" by privileged scripts
local privilegedLibraries = {
	"package",
	"coroutine",
	"os",
	"debug",
	"io",
	"ffi",
	"jit",
	"jit.p",
	"jit.v",
	"jit.zone",
	"jit.dump",
}

local libraryReadOnlyWrappers = {}

local commonLibrariesLookupTable
local privilegedLibrariesLookupTable

-- Removes all exports made by a script
local function removeScriptDefinitions(script)

	if script == nil then
		return
	end

	-- Clear exports table
	script.exports = {}

	-- Remove events created by the script
	events.removeScriptMappings(script.name)

end

-- Checks if a string starts with a specific prefix
local function stringStartsWith(str, prefix)
   return string.sub(str, 1, string.len(prefix)) == prefix
end

-- Checks if a script is a core script (not auto-reloaded)
local function isCoreScript(scriptName)
	-- TODO use configurable paths
	return stringStartsWith(scriptName, "core.")
end

-- Checks if a script is a system script (has access to unsafe globals)
local function isPrivilegedScript(scriptName)
	-- TODO use configurable paths
	return isCoreScript(scriptName) or stringStartsWith(scriptName, "system.")
end

-- Checks if a script should be auto-loaded.
local function isLoadableScript(scriptName)
	-- TODO use configurable paths
	return stringStartsWith(scriptName, "system.") or stringStartsWith(scriptName, "luavis.")
end

-- Checks if a library is preloaded and can be returned directly by "require".
-- First return value: is this library requirable at all?
-- Second return value: does the library need privileges to be required?
local function isDirectlyRequirable(libraryName)
	if commonLibrariesLookupTable == nil then
		commonLibrariesLookupTable = {}
		for i = 1, #commonLibraries do
			commonLibrariesLookupTable[commonLibraries[i]] = true
		end
	end
	if privilegedLibrariesLookupTable == nil then
		privilegedLibrariesLookupTable = {}
		for i = 1, #privilegedLibraries do
			privilegedLibrariesLookupTable[privilegedLibraries[i]] = true
		end
	end
	local isCommonRequirable = commonLibrariesLookupTable[libraryName]
	local isPrivilegedRequirable = privilegedLibrariesLookupTable[libraryName]
	return isCommonRequirable or isPrivilegedRequirable, isPrivilegedRequirable
end

-- Returns the read-only version of a library
local function getReadOnlyLibrary(libraryName)
	if libraryReadOnlyWrappers[libraryName] == nil then
		local library = require(libraryName)
		if not library then
			error("Invalid library '" .. libraryName .. "'")
		end
		libraryReadOnlyWrappers[libraryName] = coreutils.readOnlyTable(library)
	end
	return libraryReadOnlyWrappers[libraryName]
end

local function createErrorTable(errorFunc)
	return setmetatable({}, {
		__index = errorFunc,
		__newindex = errorFunc,
	})
end

-- Creates a wrapper around the return value (export table) of a module.
-- If a module does not export any members, attempts to access it should throw an error.
local function wrapExportResult(scriptName, exportResult)
	if type(exportResult) == "table" then
		return exportResult
	elseif exportResult == nil then
		-- Use a more descriptive error message for the common case of no data being returned.
		local resultType = type(exportResult)
		return createErrorTable(function ()
			error("Module '" .. scriptName .. "' does not export anything", 2)
		end)
	else
		local resultType = type(exportResult)
		return createErrorTable(function ()
			error("Module '" .. scriptName .. "' exports result of invalid type '" .. resultType .. "'", 2)
		end)
	end
end

-- Unloads a script and removes all exports made by it
function scriptLoader.unloadScript(scriptName)

	-- Clear any previous load errors
	loadErrors[scriptName] = nil
	cascadedLoadErrors[scriptName] = nil

	-- Get script from script table
	local script = loadedScripts[scriptName]

	-- Check if script is currently loaded
	if script ~= nil then

		-- Unlink dependencies in the outgoing direction (but preserve dependencies of other scripts on this one)
		dependencyGraph.unlinkOutgoing(script.name)

		-- Remove the script's global variable storage entry
		globalVariables[scriptName] = nil

		-- Remove exports/events
		removeScriptDefinitions(script)

		-- Remove script entry
		loadedScripts[scriptName] = nil
	end
end

-- Initializes a module table
local function generateLazyLoadScriptInterface(sourceScript, targetScript)

	local function getTargetScriptTable()
		if loadInProgress then
			-- Check if this link would add a cycle to the dependency graph
			if dependencyGraph.checkConnection(targetScript, sourceScript) then
				error("Cyclic dependency involving '" .. sourceScript .. "' and '" .. targetScript .. "'", 2)
			end

			-- Add module dependency
			dependencyGraph.addLink(sourceScript, targetScript)

			if not loadedScripts[targetScript] then
				-- Access to unloaded script at load time -> trigger load for other script
				local success = scriptLoader.loadScript(targetScript)
				if not success then
					cascadedLoadErrors[sourceScript] = targetScript
					error("Error occurred during cascading module load of '" .. targetScript .. "'", 2)
				end
			end
		end

		local targetScriptTable = loadedScripts[targetScript]
		if targetScriptTable == nil then
			-- Access to unloaded script at run time results in an error
			error("Attempt to access unloaded module '" .. targetScript .. "'", 2)
		end
		return targetScriptTable
	end

	return setmetatable({}, {
		__index = function(tbl, key)
			local member = getTargetScriptTable().exports[key]
			if member ~= nil then
				return member
			else
				error("Attempt to access non-existent member '" .. key .. "' of module '" .. targetScript .. "'", 2)
			end
		end,

		__call = function(tbl, ...)
			return getTargetScriptTable().exports(...)
		end,

		__newindex = function(tbl, key, value)
			error("Attempt to modify module member '" .. targetScript .. "." .. key .. "'", 2)
		end
	})
end

-- Returns a proxy table for accessing a library or the exports of the specified module
local function requireModule(sourceScript, targetScript)
	-- Check if this module is a built-in library
	local directLibrary, needPrivileges = isDirectlyRequirable(targetScript)
	if directLibrary then
		-- Prevent security-critical libraries from being accessed by common scripts
		if needPrivileges and not isPrivilegedScript(sourceScript) then
			error("Cannot require library '" .. targetScript .. "': permission denied", 3)
		end

		return getReadOnlyLibrary(targetScript)
	end

	-- Not a library: try to load as common script
	-- TODO: more fine-grained permission checks
	if bridge.script.scriptExists(targetScript) then
		if isCoreScript(targetScript) then
			if isPrivilegedScript(sourceScript) then
				-- Return core script directly
				return require(targetScript)
			else
				error("Cannot require core module '" .. targetScript .. "': permission denied", 3)
			end
		elseif isLoadableScript(targetScript) then
			-- Return proxy table
			return generateLazyLoadScriptInterface(sourceScript, targetScript)
		else
			error("Cannot require script '" .. targetScript .. "': not within permitted location", 3)
		end
	end
end

-- Loads a sandboxed script (implementation function)
local function loadScriptImpl(scriptName)

	-- Get existing entry
	local scriptEntry = loadedScripts[scriptName]

	-- Create entry for loadedScripts table
	if scriptEntry == nil then

		-- Initialize global table for script
		local globalTable = {}

		-- Create virtual globals
		local virtualGlobals = {
			_G = globalTable,
			event = events.getGlobalProxy(scriptName),
			log = logger.createLoggerForScript(scriptName),
			script = {
				name = string.match(scriptName, ".+%.(.+)") or scriptName,
				fullName = scriptName,
				sourceName = "@" .. scriptName,
			},
		}

		-- Provide a lazy-loading "require" function that checks for module load permissions and cyclic dependencies
		virtualGlobals.require = function (moduleName)
			return requireModule(scriptName, moduleName)
		end

		-- Import common globals
		for _, global in ipairs(commonVirtualGlobals) do
			virtualGlobals[global] = _G[global]
		end

		-- Import common libraries (read-only)
		for _, library in ipairs(commonLibraries) do
			virtualGlobals[library] = getReadOnlyLibrary(library)
		end

		-- Add extra virtual globals for privileged scripts
		if isPrivilegedScript(scriptName) then
			for _, global in ipairs(privilegedVirtualGlobals) do
				virtualGlobals[global] = _G[global]
			end
		end

		scriptEntry = {
			name = scriptName,
			virtualGlobals = virtualGlobals,
			globalTable = globalTable,
			exports = {}
		}

		loadedScripts[scriptName] = scriptEntry
	end

	-- Initialize global variable storage for script
	if globalVariables[scriptName] == nil then
		globalVariables[scriptName] = {
			declarations = {},
			initialValues = {},
			currentValues = {},
		}
	end

	-- Cache global variable storage sub-tables
	local varExistenceTable = globalVariables[scriptName].declarations
	local varInitValues = globalVariables[scriptName].initialValues
	local varCurrentValues = globalVariables[scriptName].currentValues

	-- Flag that the script is currently in the process of being loaded.
	-- This is used for tracking cyclic dependencies.
	scriptEntry.loading = true

	-- Unlink dependencies in the outgoing direction (but preserve dependencies of other scripts on this one)
	dependencyGraph.unlinkOutgoing(scriptName)

	-- Get rid of old exported package members or events
	removeScriptDefinitions(scriptEntry)

	-- Clear variable existence table
	scriptEntry.varExistenceTable = {}

	-- Global metatable for use during loading
	local metatableLoad = {

		__newindex = function(tbl, key, value)

			-- Disallow reserved variable names
			if scriptEntry.virtualGlobals[key] ~= nil then
				error("Global variable name '" .. key .. "' is reserved", 2)
			end

			-- Mark variable as "existing"
			varExistenceTable[key] = true

			-- Check if the value is different from the last initializer value
			if not coreutils.deepEquals(varInitValues[key], value) then

				-- Update both current value and initializer value
				varCurrentValues[key] = value
				varInitValues[key] = coreutils.deepCopy(value)
			end
		end,

		__index = function(tbl, key)
			local virtual = scriptEntry.virtualGlobals[key]
			if virtual ~= nil then
				return virtual
			end

			-- Check if variable exists and returns its value
			if varExistenceTable[key] then
				return varCurrentValues[key]
			else
				error("Attempt to access non-existent global variable '" .. key .. "'", 2)
			end
		end
	}

	-- Initialize writable global table
	setmetatable(scriptEntry.globalTable, metatableLoad)

	-- Assign script entry mapping
	loadedScripts[scriptName] = scriptEntry

	-- Load script function
	local scriptFunc, err = loadfile(scriptName)

	-- Script could not be loaded?
	if scriptFunc == nil then
		error(err, 0)
	end

	-- Assign function environment
	setfenv(scriptFunc, scriptEntry.globalTable)

	-- Execute script and acquire exports
	-- TODO: should scripts be prevented from setting metatables or modifying their export table at runtime?
	scriptEntry.exports = wrapExportResult(scriptName, scriptFunc())

	-- Global metatable for use during running
	local metatableRuntime = {

		__newindex = function(tbl, key, value)

			-- Disallow reserved variable names
			if scriptEntry.virtualGlobals[key] ~= nil then
				error("Global variable name '" .. key .. "' is reserved", 2)
			end

			-- Check if variable exists and assign it
			if varExistenceTable[key] then
				varCurrentValues[key] = value
			else
				error("Attempt to write to non-existent global variable '" .. key .. "'", 2)
			end
		end,

		__index = function(tbl, key)
			local virtual = scriptEntry.virtualGlobals[key]
			if virtual ~= nil then
				return virtual
			end

			-- Check if variable exists and return its value
			if varExistenceTable[key] then
				return varCurrentValues[key]
			else
				error("Attempt to access non-existent global variable '" .. key .. "'", 2)
			end
		end
	}

	-- Set runtime metatable
	setmetatable(scriptEntry.globalTable, metatableRuntime)
end

-- Loads a sandboxed script
function scriptLoader.loadScript(scriptName)

	-- Check for cyclic dependencies
	if loadedScripts[scriptName] and loadedScripts[scriptName].loading then
		return false
	end

	-- Core script or missing script files must not be loaded
	if not isLoadableScript(scriptName) or not bridge.script.scriptExists(scriptName) then
		scriptLoader.unloadScript(scriptName)
		return false
	end

	-- Clear any previous load errors
	loadErrors[scriptName] = nil
	cascadedLoadErrors[scriptName] = nil

	-- Set global "loading" flag and remember previous value
	local loadInProgressPrevious = loadInProgress
	loadInProgress = true

	-- Perform script load
	local success, err = xpcall(function ()
		loadScriptImpl(scriptName)
	end, debug.traceback)

	-- Reset global "loading" flag
	loadInProgress = loadInProgressPrevious

	-- Unset script-specific "loading" flag if it was set by loadScriptImpl
	if loadedScripts[scriptName] then
		loadedScripts[scriptName].loading = false
	end

	-- Error occurred during loading?
	if not success then

		-- Make sure that the script doesn't leave any partial exports/events behind
		removeScriptDefinitions(loadedScripts[scriptName])

		-- Suppress error reporting if another script was responsible for the load failure
		if not cascadedLoadErrors[scriptName] then

			-- Report error
			loadErrors[scriptName] = err

			-- Write the error to the debug bridge (in case it takes down the error handling system)
			bridge.debug.showError(err)
		end
	end

	return success
end

-- Returns a topologically sorted list of scripts that have a hard dependency on the specified script.
function scriptLoader.getDependentScripts(scriptName)
	return dependencyGraph.traverseBackward(scriptName) or {}
end

-- Loads all non-core scripts
function scriptLoader.loadAll()

	-- Clear errors
	loadErrors = {}
	cascadedLoadErrors = {}

	-- Unload all scripts
	loadedScripts = {}
	dependencyGraph.clear()

	-- Remove all registered events
	events.clear()

	-- Load all existing scripts
	for _, scriptName in ipairs(bridge.script.listScripts()) do

		-- Skip already loaded scripts (typically caused by load-time cross-module references)
		if not loadedScripts[scriptName] then
			scriptLoader.loadScript(scriptName)
		end

	end
end

-- Returns all load errors that occurred
function scriptLoader.getLoadErrors()
	return loadErrors
end

return scriptLoader
