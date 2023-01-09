local errorHandler = {}

local input = require "system.game.Input"
local errors = require "system.debug.ErrorHandler"
local gfx = require "system.game.Graphics"
local rect = require "system.utils.Rect"


errorSkipKey = "Escape"
allowOverdraw = false


function errorHandler.checkErrorSkipKey()
	if input.keyPress(errorSkipKey) then
		input.suppressKey(errorSkipKey)
		return true
	else
		return false
	end
end

function errorHandler.allowOverdraw()
	allowOverdraw = true
end


event.error.add("displayErrorOverlay", "displayError", function(errorData)
	local success, err = pcall(function ()
		if type(errorData) == "table" then
			if allowOverdraw then
				allowOverdraw = false
			else
				gfx.clear()
			end
			local screenRect = {0, 0, gfx.getWidth(), gfx.getHeight()}
			gfx.drawBox(screenRect, {0, 0, 0, 150})
			gfx.drawText {
				font = "gfx/font.png",
				size = 16,
				text = table.concat(errorData, "\n\n"),
				x = 0,
				y = 0,
				fillColor = {255, 128, 128},
				gradient = true,
				shadowColor = {0, 0, 0, 128},
			}
			if errors.hasOnlyExecutionErrors() then
				gfx.drawText {
					font = "gfx/font.png",
					text = "Press " .. errorSkipKey .. " to continue",
					size = 16,
					x = screenRect.w,
					y = 0,
					alignX = 1,
					fillColor = {255, 255, 255},
					gradient = true,
					shadowColor = {0, 0, 0, 128},
				}
			end
		end
	end)
	if not success then
		log.error("Failed to display error overlay: %s", tostring(err))
	end
end)

event.error.add("openScriptInEditor", "openEditor", function(errorData)
	if type(errorData) == "table" then
		for i = 1, #errorData do
			local script, line = string.match(errorData[i], "([a-zA-Z0-9_.]+):([0-9]+):")
			if script and line then
				errors.openScriptEditor(script, tonumber(line))
				break
			end
		end
	end
end)

return errorHandler
