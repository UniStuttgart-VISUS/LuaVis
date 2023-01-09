local window = require "system.game.Window"

event.configLoad.add("gameWindow", "config", function (conf)
	window.setFullscreen(
		conf["video.fullScreen"] == true,
		conf["video.resolution.width"] or 1280,
		conf["video.resolution.height"] or 720)
end)

event.configSave.add("gameWindow", "config", function (conf)
	local width, height = window.getSize()
	conf["video.fullScreen"] = window.isFullscreen()
	conf["video.resolution.width"] = width
	conf["video.resolution.height"] = height
end)

