local array = {}

local ffi = require "ffi"
local sentinel = require "core.Sentinel"

local C = ffi.C
local type = type
local floor = math.floor
local min = math.min
local copy = ffi.copy

local arrayContextID = bridge.array.getContext()

local arrayCTypes = {}
local arrayByteSizes = {}
local arrayTypeNames = {}
local arrayMaxSize = 1024 * 1024 * 8

local refCType = ffi.typeof("wosC_array_ref_t")


array.Type =
{
	INT8 = 1,
	INT16 = 2,
	INT32 = 3,
	UINT8 = 4,
	UINT16 = 5,
	UINT32 = 6,
	FLOAT = 7,
	DOUBLE = 8,
}

array.MAX_SIZE = arrayMaxSize

-- Generate array type lookup tables
arrayCTypes = {
	ffi.typeof("int8_t *"), ffi.typeof("int16_t *"), ffi.typeof("int32_t *"),
	ffi.typeof("uint8_t *"), ffi.typeof("uint16_t *"), ffi.typeof("uint32_t *"),
	ffi.typeof("float *"), ffi.typeof("double *"),
}
arrayByteSizes = { 1, 2, 4, 1, 2, 4, 4, 8 }
arrayTypeNames = { "int8", "int16", "int32", "uint8", "uint16", "uint32", "float", "double" }

local function createArrayWrapper(arrayID, arrayType)
	-- Check if array lifetime is already managed by another wrapper (this prevents use-after-free problems)
	if C.wosC_array_isOwned(arrayContextID, arrayID) then
		error("Failed to wrap array (ownership has already been taken)", 2)
	end

	-- Take ownership of the array, preventing its re-acquisition via the 'getArrayByID' function
	C.wosC_array_setOwnership(arrayContextID, arrayID, true)

	-- Create reference for automatic deallocation on garbage collection
	local arrayRef = ffi.new(refCType)
	arrayRef.context = arrayContextID
	arrayRef.array = arrayID
	ffi.gc(arrayRef, C.wosC_array_delete)

	-- Acquire array info object
	local arrayInfo = C.wosC_array_getArrayInfo(arrayContextID, arrayID)

	-- Get effective array data pointer and size (in full array items, rather than bytes)
	local arrayData = ffi.cast(arrayCTypes[arrayType], arrayInfo.data)
	local size = floor(arrayInfo.size / arrayByteSizes[arrayType])

	return setmetatable({}, {
		__index = function(tbl, key)
			if type(key) == "number" and key >= 0 and key < size then
				return arrayData[key]
			end
			if key == "size" then
				return size
			end
			if key == "type" then
				return arrayType
			end
			if key == "id" then
				return arrayID
			end
			if key == sentinel then
				return arrayInfo, arrayRef
			end
		end,
		__newindex = function(tbl, key, value)
			if type(key) == "number" and key >= 0 and key < size then
				arrayData[key] = value
			end
		end,
		_copy = array.copy,
		_serialize = function ()
			return {type = "array", arrayType = arrayType, data = ffi.string(arrayInfo.data, arrayInfo.size)}
		end,
		_dbg = function ()
			local result = {}
			local maxEntries = 50
			for i = 1, min(maxEntries, size) do
				result[i] = arrayData[i - 1]
			end
			if size > maxEntries then
				result[#result + 1] = string.format("<%s more entries>", size - maxEntries)
			end
			return result
		end,
		__tostring = function ()
			return string.format("array %s[%d] (id %d)", arrayTypeNames[arrayType], size, arrayID)
		end,
	})
end

event.deserializeType.add("array", "array", function (ev)
	ev.output = array.fromString(ev.input.arrayType, ev.input.data)
end)

function array.isArray(arr)
	return type(arr) == "table" and arr[sentinel] ~= nil
end

function array.new(arrayType, size)
	local ctype = arrayCTypes[arrayType]
	if ctype == nil then
		error("Invalid array type specified", 2)
	end

	if type(size) ~= "number" then
		error("Array size must be a number", 2)
	end

	local byteSize = floor(size) * arrayByteSizes[arrayType]

	if byteSize < 0 or byteSize > arrayMaxSize then
		error("Array size must be between 0 and " .. floor(arrayMaxSize / arrayByteSizes[arrayType]), 2)
	end

	return createArrayWrapper(C.wosC_array_new(arrayContextID, byteSize), arrayType)
end

function array.fromString(arrayType, str)
	local elementSize = arrayByteSizes[arrayType]
	if elementSize == nil then
		error("Invalid array type specified", 2)
	end
	if type(str) ~= "string" then
		error("Invalid argument (expected string, got " .. type(str) .. ")", 2)
	end
	local elementCount = math.floor(#str / elementSize)
	local output = array.new(arrayType, elementCount)
	copy(output[sentinel].data, str, elementCount * elementSize)
	return output
end

function array.getArrayByID(arrayType, arrayID)
	local ctype = arrayCTypes[arrayType]
	if ctype == nil then
		error("Invalid array type specified", 2)
	end

	if type(arrayID) ~= "number" then
		error("Array ID must be a number", 2)
	end

	return createArrayWrapper(arrayID, arrayType)
end

function array.getByteSizeByType(arrayType)
	return arrayByteSizes[arrayType]
end

function array.copy(sourceArray, destArray, sourceOffset, destOffset, count)
	destArray = destArray or array.new(sourceArray.type, sourceArray.size)
	sourceOffset = floor(sourceOffset or 0)
	destOffset = floor(destOffset or 0)
	count = count or sourceArray.size

	local sourceArrayInfo = sourceArray[sentinel]
	if sourceArrayInfo == nil then
		error("Array copy failed: could not access source array", 2)
	end

	local destArrayInfo = destArray[sentinel]
	if destArrayInfo == nil then
		error("Array copy failed: could not access destination array", 2)
	end

	if sourceArray.type ~= destArray.type then
		error("Array copy failed: mismatched array types", 2)
	end

	if sourceOffset < 0 or sourceOffset + count > sourceArray.size then
		error("Array copy failed: source index out of range", 2)
	end

	if destOffset < 0 or destOffset + count > destArray.size then
		error("Array copy failed: destination index out of range", 2)
	end

	local itemSize = arrayByteSizes[sourceArray.type]
	copy(destArrayInfo.data + itemSize * destOffset, sourceArrayInfo.data + itemSize * sourceOffset, itemSize * count)
	return destArray
end

return array
