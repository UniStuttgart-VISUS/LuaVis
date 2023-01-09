local config = {}

local systemConfig = require "system.game.Config"
local orderedSelector = require "system.events.OrderedSelector"


local configLoadSelector
local configSaveSelector

local keyPrefix = "luavis."


local function fireConfigLoadEvent()
	if configLoadSelector == nil then
		configLoadSelector = orderedSelector.new(event.configLoad, {
			"config",
		})
	end
	configLoadSelector.fire(setmetatable({}, {
		__index = function (tbl, key)
			return systemConfig.get(keyPrefix .. key)
		end,
		__newindex = function ()
			error("Attempt to modify config table during load", 2)
		end,
	}))
end

local function fireConfigSaveEvent()
	if configSaveSelector == nil then
		configSaveSelector = orderedSelector.new(event.configSave, {
			"config",
		})
	end
	local conf = {}
	configSaveSelector.fire(conf)
	return conf
end

function config.load()
	fireConfigLoadEvent()
end

function config.save()
	local conf = fireConfigSaveEvent()
	for key, value in pairs(conf) do
		if type(key) == "string" then
			systemConfig.set(keyPrefix .. key, value)
		end
	end
end

event.startup.add("loadConfig", "config", function ()
	config.load()
end)

event.exit.add("saveConfig", "config", function ()
	config.save()
end)

return config
