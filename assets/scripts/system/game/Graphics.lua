local gfx = {}

local config = require "system.game.Config"
local color = require "system.utils.Color"

local ffi = require "ffi"
local C = ffi.C

local gfxID = bridge.gfx.getID()


local sin = math.sin
local cos = math.cos

local getRGBA = color.getRGBA
local colorFromTable = color.fromTable


local images = {}
local vertexBuffers = {}
local currentVertexBuffer = 0
local currentTexturePage = nil
local currentVertexOffset = 0

local vertexStructBuffer = nil
local vertexStructBufferTinted = nil

local viewWidth = nil
local viewHeight = nil


local function getCurrentVertexBuffer()
	return vertexBuffers[currentVertexBuffer]
end

local function nextVertexBuffer()
	currentVertexBuffer = currentVertexBuffer + 1
	currentVertexOffset = 0
	if getCurrentVertexBuffer() == nil then
		vertexBuffers[currentVertexBuffer] = C.wosC_gfx_newVertexBuffer(gfxID)
	else
		C.wosC_gfx_clearVertexBuffer(gfxID, getCurrentVertexBuffer())
	end
	-- Immediately "draw" the current vertex buffer.
	-- As this simply queues up the buffer's ID for drawing,
	-- we can call this function before adding any vertices to the buffer.
	C.wosC_gfx_drawVertexBuffer(gfxID, getCurrentVertexBuffer())
end

local function selectTexturePage(page)
	if currentTexturePage ~= page then
		if currentTexturePage == nil or currentVertexOffset ~= 0 then
			nextVertexBuffer()
		end
		currentTexturePage = page
		C.wosC_gfx_setVertexBufferTexture(gfxID, getCurrentVertexBuffer(), page)
	end
end

local function writeVertices()
	C.wosC_gfx_writeVertices(gfxID, getCurrentVertexBuffer(), currentVertexOffset, 6, vertexStructBuffer)
	currentVertexOffset = currentVertexOffset + 6
end

local function writeTintedVertices()
	C.wosC_gfx_writeVertices(gfxID, getCurrentVertexBuffer(), currentVertexOffset, 6, vertexStructBufferTinted)
	currentVertexOffset = currentVertexOffset + 6
end

local function acquireImage(imageName)
	if type(imageName) ~= "string" and type(imageName) ~= "number" then
		error("Invalid image name specified (expected string, got " .. type(imageName) .. ")", 3)
	end
	if images[imageName] == nil then
		local imageID = type(imageName) == "string" and C.wosC_gfx_loadImage(gfxID, imageName) or imageName
		local imageGeometry = C.wosC_gfx_getImageGeometry(gfxID, imageID)

		images[imageName] =
		{
			id = imageID,
			x = imageGeometry.x,
			y = imageGeometry.y,
			w = imageGeometry.w,
			h = imageGeometry.h,
			texture = imageGeometry.texture
		}
	end
	return images[imageName]
end

function gfx.flush()
	if currentVertexOffset ~= 0 then
		nextVertexBuffer()
	end
end

function gfx.clear()
	if vertexStructBuffer == nil then
		vertexStructBuffer = ffi.new("wosC_gfx_vertex_t [?]", 6)
		for i = 0, 5 do
			vertexStructBuffer[i].color = -1
		end
	end
	if vertexStructBufferTinted == nil then
		vertexStructBufferTinted = ffi.new("wosC_gfx_vertex_t [?]", 6)
	end
	currentVertexBuffer = 0
	currentVertexOffset = 0
	currentTexturePage = nil
	bridge.gfx.clear()
end

function gfx.deleteImage(imageName)
	local image = images[imageName]
	if image ~= nil then
		if C.wosC_gfx_isImageLoaded(gfxID, image.id) then
			C.wosC_gfx_unloadImage(gfxID, image.id)
		end
		images[imageName] = nil
	end
end

function gfx.deleteAllImages()
	for imageName, image in pairs(images) do
		if C.wosC_gfx_isImageLoaded(gfxID, image.id) then
			C.wosC_gfx_unloadImage(gfxID, image.id)
		end
	end
	images = {}
end

function gfx.getImageSize(imageName)
	local image = acquireImage(imageName)
	if image then
		return image.w, image.h
	else
		return 0, 0
	end
end

function gfx.getImageWidth(imageName)
	local image = acquireImage(imageName)
	if image then
		return image.w
	else
		return 0
	end
end

function gfx.getImageHeight(imageName)
	local image = acquireImage(imageName)
	if image then
		return image.h
	else
		return 0
	end
end

local function setVertexPosition(vertexNumber, x, y)
	vertexStructBuffer[vertexNumber].x = x
	vertexStructBuffer[vertexNumber].y = y
end

local function setVertexTexCoord(vertexNumber, x, y)
	vertexStructBuffer[vertexNumber].tx = x
	vertexStructBuffer[vertexNumber].ty = y
end

local function setTintedVertexPosition(vertexNumber, x, y)
	vertexStructBufferTinted[vertexNumber].x = x
	vertexStructBufferTinted[vertexNumber].y = y
end

local function setTintedVertexTexCoord(vertexNumber, x, y)
	vertexStructBufferTinted[vertexNumber].tx = x
	vertexStructBufferTinted[vertexNumber].ty = y
end

local function setTintedVertexColor(vertexNumber, vertexColor)
	vertexStructBufferTinted[vertexNumber].color = colorFromTable(vertexColor)
end

local function setTintedVertexColors(vertexColor)
	vertexColor = colorFromTable(vertexColor)
	for i = 0, 5 do
		vertexStructBufferTinted[i].color = vertexColor
	end
end

function gfx.drawBox(rect, color)
	setTintedVertexPosition(0, rect[1], rect[2])
	setTintedVertexPosition(1, rect[1] + rect[3], rect[2])
	setTintedVertexPosition(2, rect[1], rect[2] + rect[4])
	setTintedVertexPosition(3, rect[1] + rect[3], rect[2])
	setTintedVertexPosition(4, rect[1] + rect[3], rect[2] + rect[4])
	setTintedVertexPosition(5, rect[1], rect[2] + rect[4])

	selectTexturePage(-1)
	setTintedVertexColors(color)
	writeTintedVertices()
end

function gfx.drawRect(rect, color)
	setTintedVertexPosition(0, rect.x, rect.y)
	setTintedVertexPosition(1, rect.x + rect.w, rect.y)
	setTintedVertexPosition(2, rect.x, rect.y + rect.h)
	setTintedVertexPosition(3, rect.x + rect.w, rect.y)
	setTintedVertexPosition(4, rect.x + rect.w, rect.y + rect.h)
	setTintedVertexPosition(5, rect.x, rect.y + rect.h)

	selectTexturePage(-1)
	setTintedVertexColors(color)
	writeTintedVertices()
end

function gfx.drawTriangle(p1, p2, p3, color)
	selectTexturePage(-1)
	setTintedVertexPosition(0, p1.x, p1.y)
	setTintedVertexPosition(1, p2.x, p2.y)
	setTintedVertexPosition(2, p3.x, p3.y)
	for i = 0, 2 do
		setTintedVertexColor(i, color)
	end
	C.wosC_gfx_writeVertices(gfxID, getCurrentVertexBuffer(), currentVertexOffset, 3, vertexStructBufferTinted)
	currentVertexOffset = currentVertexOffset + 3
end

function gfx.drawTriangleGradient(p1, p2, p3, c1, c2, c3)
	selectTexturePage(-1)
	setTintedVertexPosition(0, p1.x, p1.y)
	setTintedVertexPosition(1, p2.x, p2.y)
	setTintedVertexPosition(2, p3.x, p3.y)
	setTintedVertexColor(0, c1)
	setTintedVertexColor(1, c2)
	setTintedVertexColor(2, c3)
	C.wosC_gfx_writeVertices(gfxID, getCurrentVertexBuffer(), currentVertexOffset, 3, vertexStructBufferTinted)
	currentVertexOffset = currentVertexOffset + 3
end

function gfx.drawSprite(imageName, rect, textureRect)
	local image = acquireImage(imageName)
	selectTexturePage(image.texture)

	setVertexPosition(0, rect[1], rect[2])
	setVertexPosition(1, rect[1] + rect[3], rect[2])
	setVertexPosition(2, rect[1], rect[2] + rect[4])
	setVertexPosition(3, rect[1] + rect[3], rect[2])
	setVertexPosition(4, rect[1] + rect[3], rect[2] + rect[4])
	setVertexPosition(5, rect[1], rect[2] + rect[4])

	setVertexTexCoord(0, image.x + textureRect[1], image.y + textureRect[2])
	setVertexTexCoord(1, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setVertexTexCoord(2, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])
	setVertexTexCoord(3, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setVertexTexCoord(4, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2] + textureRect[4])
	setVertexTexCoord(5, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])

	writeVertices()
end

function gfx.drawTintedSprite(imageName, rect, textureRect, color)
	local image = acquireImage(imageName)
	selectTexturePage(image.texture)

	setTintedVertexPosition(0, rect[1], rect[2])
	setTintedVertexPosition(1, rect[1] + rect[3], rect[2])
	setTintedVertexPosition(2, rect[1], rect[2] + rect[4])
	setTintedVertexPosition(3, rect[1] + rect[3], rect[2])
	setTintedVertexPosition(4, rect[1] + rect[3], rect[2] + rect[4])
	setTintedVertexPosition(5, rect[1], rect[2] + rect[4])

	setTintedVertexTexCoord(0, image.x + textureRect[1], image.y + textureRect[2])
	setTintedVertexTexCoord(1, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setTintedVertexTexCoord(2, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])
	setTintedVertexTexCoord(3, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setTintedVertexTexCoord(4, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2] + textureRect[4])
	setTintedVertexTexCoord(5, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])

	setTintedVertexColors(color)
	writeTintedVertices()
end

local function rotatePoint(x, y, angle, originX, originY)
	local sinAngle, cosAngle = sin(angle), cos(angle)
	x, y = x - originX, y - originY
	return x * cosAngle - y * sinAngle + originX, x * sinAngle + y * cosAngle + originY
end

function gfx.drawRotatedSprite(imageName, rect, textureRect, angle, origin)
	local image = acquireImage(imageName)
	selectTexturePage(image.texture)

	local ox = rect[1] + (origin and origin[1] or rect[3] * 0.5)
	local oy = rect[2] + (origin and origin[2] or rect[4] * 0.5)

	setVertexPosition(0, rotatePoint(rect[1], rect[2], angle, ox, oy))
	setVertexPosition(1, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setVertexPosition(2, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))
	setVertexPosition(3, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setVertexPosition(4, rotatePoint(rect[1] + rect[3], rect[2] + rect[4], angle, ox, oy))
	setVertexPosition(5, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))

	setVertexTexCoord(0, image.x + textureRect[1], image.y + textureRect[2])
	setVertexTexCoord(1, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setVertexTexCoord(2, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])
	setVertexTexCoord(3, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setVertexTexCoord(4, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2] + textureRect[4])
	setVertexTexCoord(5, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])

	writeVertices()
end

function gfx.drawRotatedBox(rect, color, angle, origin)
	local ox = rect[1] + (origin and origin[1] or rect[3] * 0.5)
	local oy = rect[2] + (origin and origin[2] or rect[4] * 0.5)

	setTintedVertexPosition(0, rotatePoint(rect[1], rect[2], angle, ox, oy))
	setTintedVertexPosition(1, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setTintedVertexPosition(2, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))
	setTintedVertexPosition(3, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setTintedVertexPosition(4, rotatePoint(rect[1] + rect[3], rect[2] + rect[4], angle, ox, oy))
	setTintedVertexPosition(5, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))

	selectTexturePage(-1)
	setTintedVertexColors(color)
	writeTintedVertices()
end

function gfx.drawRotatedTintedSprite(imageName, rect, textureRect, color, angle, origin)
	local image = acquireImage(imageName)
	selectTexturePage(image.texture)

	local ox = rect[1] + (origin and origin[1] or rect[3] * 0.5)
	local oy = rect[2] + (origin and origin[2] or rect[4] * 0.5)

	setTintedVertexPosition(0, rotatePoint(rect[1], rect[2], angle, ox, oy))
	setTintedVertexPosition(1, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setTintedVertexPosition(2, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))
	setTintedVertexPosition(3, rotatePoint(rect[1] + rect[3], rect[2], angle, ox, oy))
	setTintedVertexPosition(4, rotatePoint(rect[1] + rect[3], rect[2] + rect[4], angle, ox, oy))
	setTintedVertexPosition(5, rotatePoint(rect[1], rect[2] + rect[4], angle, ox, oy))

	setTintedVertexTexCoord(0, image.x + textureRect[1], image.y + textureRect[2])
	setTintedVertexTexCoord(1, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setTintedVertexTexCoord(2, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])
	setTintedVertexTexCoord(3, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2])
	setTintedVertexTexCoord(4, image.x + textureRect[1] + textureRect[3], image.y + textureRect[2] + textureRect[4])
	setTintedVertexTexCoord(5, image.x + textureRect[1], image.y + textureRect[2] + textureRect[4])

	setTintedVertexColors(color)
	writeTintedVertices()
end

local function toIntegerColor(colorValue)
	-- SFML expects reverse color values
	return colorValue and bit.bswap(colorFromTable(colorValue))
end

function gfx.drawText(textInfo)
	if type(textInfo) ~= "table" then
		error("Argument to drawText must be a table, but was " .. type(textInfo), 2)
	end
	if textInfo.text ~= nil and type(textInfo.text) ~= "string" and type(textInfo.text) ~= "table" then
		error("Field 'text' must be a string or table, but was " .. type(textInfo.text), 2)
	end
	-- TODO be less wasteful with vertex buffers
	nextVertexBuffer()
	textInfo.vertexBuffer = getCurrentVertexBuffer()
	textInfo.fillColor = toIntegerColor(textInfo.fillColor)
	textInfo.outlineColor = toIntegerColor(textInfo.outlineColor)
	textInfo.shadowColor = toIntegerColor(textInfo.shadowColor)

	-- TODO kind of a hack
	if textInfo.position then
		textInfo.x = textInfo.position.x or textInfo.position[1] or textInfo.x
		textInfo.y = textInfo.position.y or textInfo.position[2] or textInfo.y
	end

	local bounds
	if textInfo.monospaceWidth then
		local text = textInfo.text
		if type(text) == "string" then
			for i = 1, #text do
				textInfo.text = text:sub(i, i)
				bridge.gfx.drawText(textInfo)
				textInfo.x = textInfo.x + textInfo.monospaceWidth
			end
		elseif type(text) == "table" then
			for i = 1, #text do
				textInfo.text = text[i]
				bridge.gfx.drawText(textInfo)
				textInfo.x = textInfo.x + textInfo.monospaceWidth
			end
		end
		-- TODO return bounds of monospace text
	else
		bounds = bridge.gfx.drawText(textInfo)
	end
	-- TODO be less wasteful with vertex buffers
	nextVertexBuffer()
	currentTexturePage = nil
	return bounds
end

function gfx.takeScreenshot(targetFile)
	nextVertexBuffer()
	bridge.gfx.takeScreenshot(getCurrentVertexBuffer(), targetFile)
	nextVertexBuffer()
end

function gfx.getWidth()
	if not viewWidth then
		viewWidth = config.getNumber("wos.game.size[0]")
	end
	return viewWidth
end

function gfx.getHeight()
	if not viewHeight then
		viewHeight = config.getNumber("wos.game.size[1]")
	end
	return viewHeight
end

function gfx.getSize()
	return gfx.getWidth(), gfx.getHeight()
end

return gfx
