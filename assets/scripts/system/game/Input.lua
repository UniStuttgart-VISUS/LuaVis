local input = {}

local orderedSelector = require "system.events.OrderedSelector"
local timer = require "system.utils.Timer"
local utils = require "system.utils.Utilities"
local window = require "system.game.Window"


local inputMethodChangedSelector = orderedSelector.new(event.inputMethodChanged, {
	"clearCache",
	"autoBind",
})

local function initInputTable()
	return {
		keys = {},
		keyPresses = {},
		keyPressSet = {},
		controllers = {},
		mouse = {
			focus = false,
			x = 0,
			y = 0
		},
	}
end

local fallbackController = { buttons = {}, axes = {} }

local keyNameList
local keyNameMap = {}

-- TODO make these customizable
local controllerAxisThreshold = 50
local mapControllersToKeys = true
local mapMouseButtonsToKeys = true
local mouseScrollYAmount = 0

-- TODO persist known controllers table across sessions to retain correct key bindings
knownControllers = {}
connectedControllers = {}

local virtualInputs = {}

inputCurrentFrame = initInputTable()
inputPreviousFrame = initInputTable()


-- Checks if the key with the specified name is currently pressed.
function input.keyDown(keyname)
	return inputCurrentFrame.keys[keyname:lower()] or false
end

-- Checks if the key with the specified name was just pressed.
function input.keyPress(keyname)
	return inputCurrentFrame.keyPressSet[keyname:lower()] or false
end

-- Checks if the key with the specified name was just released.
function input.keyRelease(keyname)
	keyname = keyname:lower()
	return not inputCurrentFrame.keys[keyname] and inputPreviousFrame.keys[keyname] or false
end

-- Suppresses the specified keypress, preventing it from being recognized as pressed within the same frame.
function input.suppressKey(keyname)
	keyname = keyname:lower()
	local keyPressList = inputCurrentFrame.keyPresses
	if keyPressList and keyPressList[1] then
		if #keyPressList > 1 then
			utils.removeIf(inputCurrentFrame.keyPresses, function (key)
				return key == keyname
			end)
		elseif keyPressList[1] == keyname then
			keyPressList[1] = nil
		end
	end
	inputCurrentFrame.keys[keyname] = nil
	inputCurrentFrame.keyPressSet[keyname] = nil
	virtualInputs[keyname] = nil

	if inputCurrentFrame.suppressed then
		inputCurrentFrame.suppressed[keyname] = true
	else
		inputCurrentFrame.suppressed = { [keyname] = true }
	end
end

function input.isKeySuppressed(keyname)
	return inputCurrentFrame.suppressed and inputCurrentFrame.suppressed[keyname]
end

-- Returns a list containing all keys that were pressed down in the current frame.
function input.getPressedKeys()
	-- Use native support for just-pressed keys, if possible
	if inputCurrentFrame.keyPresses then
		return utils.arrayCopy(inputCurrentFrame.keyPresses)
	end

	-- Fall back to comparing keys to previous frame
	return utils.removeIf(input.getHeldKeys(), function (key)
		return inputPreviousFrame.keys[key]
	end)
end

-- Returns a list containing all keys that are being held down in the current frame.
function input.getHeldKeys()
	return utils.getKeyList(inputCurrentFrame.keys)
end

local mouseButtons = {"left", "right", "middle", "extra1", "extra2"}
local mouseButtonKeyNames = {}

for i = 1, #mouseButtons do
	mouseButtonKeyNames[i] = string.format("mouse_%d", i)
end

local function mouseDownImpl(button, inputTable)
	if button == nil then
		for i = 1, #mouseButtons do
			if mouseDownImpl(i, inputTable) then
				return true
			end
		end
		return false
	elseif type(button) == "number" then
		button = mouseButtons[button]
	elseif type(button) == "string" then
		button = button:lower()
	end
	return inputTable.mouse[button] == true
end

-- Checks if the specified mouse button is pressed.
-- Passing nil tests any mouse button.
-- Available options: "left" (1), "right" (2), "middle" (3), "extra1" (4), "extra2" (5)
function input.mouseDown(button)
	return mouseDownImpl(button, inputCurrentFrame)
end

function input.mousePress(button)
	return not mouseDownImpl(button, inputPreviousFrame) and mouseDownImpl(button, inputCurrentFrame)
end

function input.mouseRelease(button)
	return mouseDownImpl(button, inputPreviousFrame) and not mouseDownImpl(button, inputCurrentFrame)
end

-- Returns the horizontal mouse position.
function input.mouseX()
	return inputCurrentFrame.mouse.x
end

-- Returns the vertical mouse position.
function input.mouseY()
	return inputCurrentFrame.mouse.y
end

-- Returns the horizontal scrolling distance for this frame.
function input.scrollX()
	return inputCurrentFrame.mouse.scrollX
end

-- Returns the vertical scrolling distance for this frame.
function input.scrollY()
	return inputCurrentFrame.mouse.scrollY
end

-- Returns true if the mouse is over the game window, false if the mouse is off the game window.
function input.mouseFocus()
	return inputCurrentFrame.mouse.focus
end

-- Returns a list of unicode codepoints entered in the current frame.
function input.text()
	return inputCurrentFrame.text
end

-- Returns a list of raw key press/release events for this frame.
function input.rawEvents()
	return inputCurrentFrame.keyEvents or {}
end

-- Looks up the name of a raw key by its ID.
function input.lookUpRawKey(keyID)
	return keyNameList[keyID]
end

-- Returns true if any key was pressed in this frame.
function input.anyKeyPressed()
	return inputCurrentFrame.anyKeyPressed
end

function input.getControllerInfo(index)
	return knownControllers[index] or {}
end

function input.isLegacyControllerEnabled()
	return mapControllersToKeys
end

local function getControllerAxes(controllerData)
	local result = {}
	if controllerData.axes then
		for axis, value in ipairs(controllerData.axes) do
			if value < -controllerAxisThreshold then
				result[axis * 2 - 1] = true
			elseif value > controllerAxisThreshold then
				result[axis * 2] = true
			end
		end
	end
	return result
end

local function handleVirtualRelease(inputTable, button)
	if virtualInputs[button] then
		inputTable.keys[button] = nil
		inputTable.keyEvents[#inputTable.keyEvents + 1] = -keyNameMap[button]
		inputTable.keyEvents[#inputTable.keyEvents + 1] = timer.getGlobalTime()
		virtualInputs[button] = nil
	end
end

local function handleVirtualPress(inputTable, button)
	handleVirtualRelease(inputTable, button)
	inputTable.keys[button] = true
	inputTable.keyPresses[#inputTable.keyPresses + 1] = button
	inputTable.keyEvents[#inputTable.keyEvents + 1] = keyNameMap[button]
	inputTable.keyEvents[#inputTable.keyEvents + 1] = timer.getGlobalTime()
	virtualInputs[button] = true
end

local function preprocessInputs(inputTable, previous)
	-- Fix up nil values for initial frames
	if inputTable == nil or inputTable.keys == nil or inputTable.mouse == nil then
		inputTable = initInputTable()
	end

	-- Handle key events
	inputTable.keyPresses = inputTable.keyPresses or {}
	if inputTable.keyEvents then
		for i = 1, #inputTable.keyEvents, 2 do
			local key = inputTable.keyEvents[i]
			if key > 0 then
				inputTable.keyPresses[#inputTable.keyPresses + 1] = keyNameList[key]
				inputTable.anyKeyPressed = true
			end
		end
	end

	-- Handle held keys
	local mappedKeys = {}
	for key, value in pairs(inputTable.keys) do
		mappedKeys[keyNameList[key]] = value
	end

	-- Add held virtual keys
	for key in pairs(virtualInputs) do
		mappedKeys[key] = true
	end

	inputTable.keys = mappedKeys

	-- Map mouse buttons to keys
	if mapMouseButtonsToKeys then
		for i, button in ipairs(mouseButtons) do
			local isHeld, wasHeld = inputTable.mouse[button], previous.mouse[button]
			if isHeld and not wasHeld then
				handleVirtualPress(inputTable, mouseButtonKeyNames[i])
			elseif wasHeld and not isHeld then
				handleVirtualRelease(inputTable, mouseButtonKeyNames[i])
			end
		end

		-- Map scroll wheel to keys
		mouseScrollYAmount = mouseScrollYAmount + (inputTable.mouse.scrollY or 0)
		if math.abs(mouseScrollYAmount) >= 1 then
			local mouseKey = mouseScrollYAmount > 0 and "mouse_wheel_up" or "mouse_wheel_down"
			handleVirtualPress(inputTable, mouseKey)
			handleVirtualRelease(inputTable, mouseKey)
			mouseScrollYAmount = 0
		end
	end

	-- Define keypress set
	inputTable.keyPressSet = {}
	for _, k in ipairs(inputTable.keyPresses) do
		inputTable.keyPressSet[k] = true
	end

	return inputTable, previous
end

local getInput = bridge.input and bridge.input.getInput or initInputTable
local setupChanged = bridge.input and bridge.input.controllerSetupChanged or function () end
local getConnectedControllers = bridge.input and bridge.input.getConnectedControllers or function () return {} end
local getKeyboardLookupTable = bridge.input and bridge.input.getKeyboardLookupTable or function () return {} end

local function findOrCreateController(data)
	for id, known in ipairs(knownControllers) do
		if utils.deepEquals(data, known) then
			return id
		end
	end
	knownControllers[#knownControllers + 1] = data
	return #knownControllers
end

local function updateControllerSetup()
	if setupChanged() then
		connectedControllers = {}
		for id, data in pairs(getConnectedControllers()) do
			-- Map native controller ID to known (unique) controller ID
			connectedControllers[id] = findOrCreateController(data)
		end
	end
end

local function initLookupTables()
	if not keyNameList then
		keyNameList = {}
		keyNameMap = {}
		for id, key in ipairs(getKeyboardLookupTable()) do
			key = key:lower()
			keyNameList[id] = key
			keyNameMap[key] = id
		end

		local getNativeKeyName = bridge.input and bridge.input.getNativeKeyName
		if getNativeKeyName then
			-- Auto-discover key names by ID
			setmetatable(keyNameList, {
				__index = function (tbl, keyID)
					local name = keyID
					if type(keyID) == "number" then
						name = utf8.lower(tostring(getNativeKeyName(keyID) or ""))
						if name == "" or rawget(keyNameMap, name) then
							name = string.format("key_%s", keyID)
						end
					elseif type(keyID) ~= "string" then
						name = tostring(keyID)
					end
					tbl[keyID] = name
					keyNameMap[name] = keyID
					return name
				end,
			})

			-- Auto-generate key IDs by name
			setmetatable(keyNameMap, {
				__index = function (tbl, keyName)
					local id = #keyNameList + 1
					keyNameList[id] = keyName
					tbl[keyName] = id
					return id
				end,
			})
		end
	end
end

-- Called every tick before everything else: reads the game's inputs
event.cycle.add("updateInput", "input", function ()
	initLookupTables()
	updateControllerSetup()
	inputCurrentFrame, inputPreviousFrame = preprocessInputs(getInput(), inputCurrentFrame or initInputTable())
end)

return input
