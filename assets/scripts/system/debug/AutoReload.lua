local gfx = require "system.game.Graphics"
local errors = require "system.debug.ErrorHandler"
local config = require "system.game.Config"
local ecsSchema = require "system.game.EntitySchema"

local scriptLoader = require "core.ScriptLoader"

initialized = false


local function escapeString(str)
    return string.gsub(str, "([^%w])", "%%%1")
end

local function extractScriptName(fileName)
	local scriptPath = escapeString(config.getString("wos.game.assets.scripts.path"))
	local scriptExtension = escapeString(config.getString("wos.game.assets.scripts.extension"))
	return string.match(fileName, "^" .. scriptPath .. "/(.*)" .. scriptExtension .. "$")
end

local function resourcePathToScriptPath(fileName)
	return string.gsub(extractScriptName(fileName), "/", ".")
end

local function isScriptResource(fileName)
	return extractScriptName(fileName) ~= nil
end

local function resourcePathToEntitySchemaPath(fileName)
	local ecsPath = escapeString(config.getString("wos.game.assets.ecs.path"))
	local ecsExtension = escapeString(config.getString("wos.game.assets.ecs.extension"))
	return string.match(fileName, "^" .. ecsPath .. "/(.*)" .. ecsExtension .. "$")
end

local function isEntitySchemaResource(fileName)
	return resourcePathToEntitySchemaPath(fileName) ~= nil
end

local function reload(resource)
	-- Discard loaded image
	gfx.deleteImage(resource)

	-- Reload ECS schema
	if isEntitySchemaResource(resource) then
		-- Clear errors (in case they are caused by schema issues)
		errors.clearExecutionErrors()
		ecsSchema.loadFromFile(resource)
	end

	-- Reload script
	if isScriptResource(resource) then

		-- Reloading any script may fix run-time errors on any other script
		errors.clearExecutionErrors()

		-- Get current script name for loading
		local scriptName = resourcePathToScriptPath(resource)

		-- Load the script that underwent file changes
		scriptLoader.loadScript(scriptName)

		-- Grab all scripts that have the loaded script as a dependency and reload them.
		-- The function returns them in topologically sorted order, resolving the dependency chain correctly.
		for _, dependentScript in ipairs(scriptLoader.getDependentScripts(scriptName)) do
			scriptLoader.loadScript(dependentScript)
		end
	end

	-- Re-check error message
	errors.update()
end

local function reloadAll()
	-- Discard all loaded graphics
	gfx.deleteAllImages()

	-- Clear all script errors
	errors.clearExecutionErrors()

	-- Reload all ECS schemas
	ecsSchema.reloadAll()

	-- Reload all scripts
	scriptLoader.loadAll()

	-- Re-check error message
	errors.update()
end

local function flushResourceChanges()
	while bridge.res.pollChanges() do end
end

event.cycle.add("autoReload", "autoReload", function()

	-- Ignore initial resource updates to prevent loading everything twice on the first run.
	if not initialized then
		initialized = true
		flushResourceChanges()
	end

	local resource
	while true do
		resource = bridge.res.pollChanges()
		if resource == nil then
			break
		elseif resource == "*" then
			reloadAll()
		elseif type(resource) == "string" then
			reload(resource)
		end
	end
end)

