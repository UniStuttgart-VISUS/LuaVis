local framebuffer = {}

local array = require "system.utils.Array"
local bitmap = require "system.game.Bitmap"
local proxy = require "system.utils.Proxy"

local ffi = require "ffi"
local C = ffi.C

local gfxBridge = bridge.gfx

local gfxID = gfxBridge.getID()

local refCType = ffi.typeof("wosC_array_ref_t")
local arrayContextID = bridge.array.getContext()

function framebuffer.new(width, height)
	local id

	-- Lazy-initialize the framebuffer
	local function initialize()
		if id == nil then
			id = gfxBridge.createFramebuffer(width, height)
		end
	end

	local functions = {
		load = function (imageName)
			initialize()
			local arrayID = gfxBridge.getImagePixels(imageName)
			gfxBridge.updateFramebuffer(id, 0, 0, width, height, arrayID, 0)
			-- Create reference for automatic deallocation on garbage collection
			local arrayRef = ffi.new(refCType)
			arrayRef.context = arrayContextID
			arrayRef.array = arrayID
			C.wosC_array_delete(arrayRef)
		end,
		update = function (image, x, y, width, height)
			initialize()

			if type(image) ~= "table" then
				error("Invalid image for framebuffer update (expected table, got " .. type(image) .. ")", 2)
			end

			local offset
			if type(image.getArray) == "function" then
				width, height = image.getWidth(), image.getHeight()
				offset = bitmap.HEADER_BYTES
				image = image.getArray()
				if not array.isArray(image) then
					error("Invalid bitmap for framebuffer update", 2)
				end
			elseif array.isArray(image) then
				if (image.size * array.getByteSizeByType(image.type)) < width * height then
					error(string.format("Array size %s is too small for framebuffer update of %sx%s pixels",
					image.size, width, height), 2)
				end
			else
				error("Invalid table for framebuffer update (expected bitmap or array)", 2)
			end

			return gfxBridge.updateFramebuffer(id, x or 0, y or 0, width, height, image.id, offset or 0)
		end,
		clear = function ()
			initialize()
			local blankArray = array.new(array.Type.INT32, width * height)
			return gfxBridge.updateFramebuffer(id, 0, 0, width, height, blankArray.id, 0)
		end,
		isValid = function ()
			initialize()
			return id ~= nil and id >= 0
		end,
	}

	return proxy.setMetatable({}, {
		__index = function (_, key)
			if key == "id" then
				initialize()
				return id
			elseif functions[key] then
				return functions[key]
			end
		end,
		__newindex = function ()
			error("Attempt to write to framebuffer")
		end,
		__gc = function ()
			if id and C.wosC_gfx_isImageLoaded(gfxID, id) then
				C.wosC_gfx_unloadImage(gfxID, id)
			end
		end,
	})
end

return framebuffer
