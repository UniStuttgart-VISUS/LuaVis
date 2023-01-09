local window = {}

function window.isFullscreen()
	return bridge.window.isFullscreen()
end

function window.setFullscreen(fullscreen, width, height)
	bridge.window.changeMode(width or bridge.window.getWidth(), height or bridge.window.getHeight(), fullscreen)
end

function window.getSize()
	return bridge.window.getWidth(), bridge.window.getHeight()
end

function window.resize(width, height)
	bridge.window.changeMode(width, height, bridge.window.isFullscreen())
end

return window
