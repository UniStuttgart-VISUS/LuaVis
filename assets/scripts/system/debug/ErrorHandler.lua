local errors = {}

local gfx = require "system.game.Graphics"
local orderedSelector = require "system.events.OrderedSelector"

local scriptLoader = require "core.ScriptLoader"


local errorSelector


executionErrors = {}
isErrorInfoCollected = false

function errors.hasError()
	return #executionErrors ~= 0
		or next(scriptLoader.getLoadErrors()) ~= nil
end

function errors.hasOnlyExecutionErrors()
	return #executionErrors ~= 0
		and next(scriptLoader.getLoadErrors()) == nil
end

function errors.update()
	isErrorInfoCollected = false
end

function errors.addExecutionError(scriptName, eventType, funcName, errorText)
	executionErrors[#executionErrors + 1] = {
		scriptName = scriptName,
		funcName = funcName,
		eventType = eventType,
		errorText = errorText
	}
end

function errors.clearExecutionErrors()
	executionErrors = {}
end

function errors.openScriptEditor(script, line)
	--local s, e = pcall(function() bridge.debug.openScriptEditor(script, line) end)
	--if not s then
	--	dbg(e)
	--end
end


local function fireError(errorLines)
	if errorSelector == nil then
		errorSelector = orderedSelector.new(event.error, {
			"handleError",
			"displayError",
			"openEditor",
		})
	end
	errorSelector.fire(errorLines)
end


event.cycle.add("collectErrors", "errors", function ()

	if errors.hasError() then
		if not isErrorInfoCollected then
			isErrorInfoCollected = true
			local errorLines = {}

			local loadErrors = scriptLoader.getLoadErrors()
			for script, err in pairs(loadErrors) do
				errorLines[#errorLines + 1] = "Error loading script '" .. script .. "':\n" .. err
			end

			local executionErrors = executionErrors
			for i = 1, #executionErrors do
				local err = executionErrors[i]
				errorLines[#errorLines + 1] = "Error in '" .. err.eventType .. "." .. err.funcName .. "':\n" .. err.errorText
			end

			fireError(errorLines)
		end
	elseif isErrorInfoCollected then
		isErrorInfoCollected = false
		fireError(nil)
	end
end)

event.error.add("displaySystemError", "displayError", function(errorData)
	bridge.core.showError(table.concat(errorData, "\n\n"))
end)

return errors
