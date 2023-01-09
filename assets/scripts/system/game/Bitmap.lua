local bitmap = {}

local array = require "system.utils.Array"

local floor = math.floor
local min = math.min
local max = math.max

local HEADER_SIZE = 12
local HEADER_MAGIC = 0x89008901

local function writeUint32(array, offset, num)
	for i = 0, 3 do
		array[offset + 3 - i] = floor(num / (2 ^ (i * 8))) % 256
	end
end

local function writeBitmapHeader(array, width, height)
	writeUint32(array, 0, HEADER_MAGIC)
	writeUint32(array, 4, width)
	writeUint32(array, 8, height)
end

local function clipRect(rect, width, height)
	rect[3], rect[4] = min(rect[3], width), min(rect[4], height)
	return {max(min(rect[1], width - rect[3]), 0), max(min(rect[2], height - rect[4]), 0), rect[3], rect[4]}
end

local function getArrayIndex(x, y, width)
	return (x + y * width) * 4 + HEADER_SIZE
end

local function createBitmapWrapper(data, width, height)
	writeBitmapHeader(data, width, height)

	local function getIndex(x, y)
		return getArrayIndex(x, y, width)
	end

	return setmetatable({}, {
		__index = {
			getPixel = function (x, y)
				local index = getIndex(x, y)
				return {data[index], data[index + 1], data[index + 2], data[index + 3]}
			end,
			setPixel = function (x, y, color)
				local index = getIndex(x, y)
				for i = 0, 3 do
					data[index + i] = color[i + 1]
				end
			end,
			getWidth = function ()
				return width
			end,
			getHeight = function ()
				return height
			end,
			getSize = function ()
				return width, height
			end,
			copyRect = function (source, sourceRect, targetPos)
				-- TODO correctly clamp negative source/target positions
				local sourceData, sourceWidth, sourceHeight = source.getArray(), source.getSize()

				sourceRect = clipRect(sourceRect, sourceWidth, sourceHeight)

				local targetRect = {targetPos[1], targetPos[2], sourceRect[3], sourceRect[4]}
				targetRect = clipRect(targetRect, width, height)

				local sourceIndex = getArrayIndex(sourceRect[1], sourceRect[2], sourceWidth)
				local targetIndex = getIndex(targetRect[1], targetRect[2])

				local sourceStride = getArrayIndex(sourceRect[1], sourceRect[2] + 1, sourceWidth) - sourceIndex
				local targetStride = getIndex(targetRect[1], targetRect[2] + 1) - targetIndex

				local rectWidth, rectHeight = targetRect[3], targetRect[4]

				for y = 0, rectHeight - 1 do
					array.copy(sourceData, data, sourceIndex, targetIndex, rectWidth * 4)
					sourceIndex = sourceIndex + sourceStride
					targetIndex = targetIndex + targetStride
				end
			end,
			getArray = function ()
				return data
			end,
		},
		__newindex = function (tbl, key, value)
			error("Attempt to write to bitmap wrapper", 2)
		end,
	})
end

function bitmap.load(filename)
	local arrayID, width, height = bridge.gfx.getImagePixels(filename)
	if type(arrayID) == "number" and arrayID >= 0 then
		local rgbaArray = array.getArrayByID(array.Type.UINT8, arrayID)
		local bitmapArray = array.new(array.Type.UINT8, rgbaArray.size + HEADER_SIZE)
		array.copy(rgbaArray, bitmapArray, 0, HEADER_SIZE, rgbaArray.size)
		return createBitmapWrapper(bitmapArray, width, height)
	end
end

function bitmap.new(width, height)
	local bitmapArray = array.new(array.Type.UINT8, width * height * 4 + HEADER_SIZE)
	return createBitmapWrapper(bitmapArray, width, height)
end

function bitmap.crop(bm, rect)
	rect = clipRect(rect, bm.getSize())
	local cropped = bitmap.new(rect[3], rect[4])
	cropped.copyRect(bm, rect, {0, 0})
	return cropped
end

return bitmap
