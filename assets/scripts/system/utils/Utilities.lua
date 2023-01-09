local utils = {}

local coreutils = require "core.CoreUtils"
local inspect = require "core.Inspect"

local min = math.min
local max = math.max
local floor = math.floor
local ceil = math.ceil
local type = type

local lowercaseCache = {}

function utils.toLowercaseCache(text)
	if lowercaseCache[text] then
		return lowercaseCache[text]
	else
		lowercaseCache[text] = string.lower(text)
		return lowercaseCache[text]
	end
end

function utils.deepCopy(tbl)
	return coreutils.deepCopy(tbl)
end


function utils.shallowCopy(tbl)
	if type(tbl) == "table" then
		local result = {}
		for k, v in pairs(tbl) do
			result[k] = v
		end
		return result
	else
		return tbl
	end
end

function utils.arrayCopy(tbl)
	local result = {}
	for i = 1, #tbl do
		result[i] = tbl[i]
	end
	return result
end

function utils.readOnlyTable(tbl)
	return coreutils.readOnlyTable(tbl)
end

function utils.deepEquals(tbl1, tbl2)
	return coreutils.deepEquals(tbl1, tbl2)
end

function utils.inspect(object)
	return inspect(object)
end

function utils.appendToTable(tbl1, tbl2)
	for i = 1, #tbl2 do
		tbl1[#tbl1 + 1] = tbl2[i]
	end
	return tbl1
end

function utils.copyConcatArrays(tbl1, tbl2)
	local result = {}
	for i = 1, #tbl1 do
		result[#result + 1] = tbl1[i]
	end
	for i = 1, #tbl2 do
		result[#result + 1] = tbl2[i]
	end
	return result
end

function utils.removeIf(tbl, func)
	local size = #tbl
	local left, right = 1, 1
	while right <= size do
		if func(tbl[right]) then
			tbl[right] = nil
		else
			if left ~= right then
				tbl[left] = tbl[right]
				tbl[right] = nil
			end
			left = left + 1
		end
		right = right + 1
	end
	return tbl
end

function utils.tableRemoveIf(tbl, func)
	local itemsToRemove = {}
	for key, value in pairs(tbl) do
		if func(key, value) then
			itemsToRemove[#itemsToRemove + 1] = key
		end
	end
	for i = 1, #itemsToRemove do
		tbl[itemsToRemove[i]] = nil
	end
	return tbl
end

function utils.lowerBound(tbl, value)
	local first, count = 1, #tbl
	local step, index

	while count > 0 do
		step = floor(count / 2)
		index = first + step
		if tbl[index] < value then
			first = index + 1
			count = count - step - 1
		else
			count = step
		end
	end
	return first
end

function utils.lowerBoundOrdered(tbl, value, compare)
	local first, count = 1, #tbl
	local step, index

	while count > 0 do
		step = floor(count / 2)
		index = first + step
		if compare(tbl[index], value) then
			first = index + 1
			count = count - step - 1
		else
			count = step
		end
	end
	return first
end

function utils.clamp(lowLimit, value, upLimit)
	return max(lowLimit, min(value, upLimit))
end

function utils.cacheTable(func, tbl)
	tbl = tbl or {}
	return setmetatable(tbl,
	{
		__index = function (indexedTable, key)
			local value = func()
			indexedTable[key] = value
			return value
		end
	})
end

function utils.mergeTables(dest, src)
	for k, v in pairs(src) do
		dest[k] = v
	end
	return dest
end

function utils.createGaplessTable(tbl)
	local entries, result = {}, {}
	for i, k in pairs(tbl) do
		if type(i) == "number" then
			entries[#entries + 1] = {i, k}
		else
			result[i] = k
		end
	end
	table.sort(entries, function (a, b)
		return a[1] < b[1]
	end)
	for i = 1, #entries do
		result[i] = entries[i][2]
	end
	return result
end

function utils.flatten(tbl)
	local result = {}
	for k, v in pairs(tbl) do
		result[#result + 1] = {k, v}
	end
	return result
end

function utils.getKeyList(tbl)
	local keys = {}
	local i = 1
	for k, v in pairs(tbl) do
		keys[i] = k
		i = i + 1
	end
	return keys
end

function utils.getValueList(tbl)
	local values = {}
	local i = 1
	for k, v in pairs(tbl) do
		values[i] = v
		i = i + 1
	end
	return values
end

function utils.getKeyValueLists(tbl)
	local keys, values = {}, {}
	local i = 1
	for k, v in pairs(tbl) do
		keys[i] = k
		values[i] = v
		i = i + 1
	end
	return keys, values
end

function utils.setKeyAtPath(targetTable, keyPath, value)
	if #keyPath == 0 then
		return false, 0
	end
	for i = 1, #keyPath do
		local key = keyPath[i]
		if (type(key) ~= "number" and type(key) ~= "string") or type(targetTable) ~= "table" then
			return false, i
		end
		if i == #keyPath then
			targetTable[key] = value
		else
			local newTarget = targetTable[key]
			if newTarget == nil then
				newTarget = {}
				targetTable[key] = newTarget
			end
			targetTable = newTarget
		end
	end
	return true
end

function utils.lerp(v1, v2, factor)
	return (1 - factor) * v1 + factor * v2
end

function utils.step(edge, x)
	return 1 - max(0, min(1, ceil(edge - x)))
end

return utils
