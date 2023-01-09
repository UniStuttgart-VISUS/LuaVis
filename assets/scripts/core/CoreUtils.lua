local function mapTableImpl(tbl, func, visited)
	if type(tbl) == "table" then
		if not visited[tbl] then
			visited[tbl] = true
			for k, v in pairs(tbl) do
				tbl[k] = mapTableImpl(v, func, visited)
			end
		end
		return func(tbl)
	else
		return tbl
	end
end

local function mapTable(tbl, func)
	return mapTableImpl(tbl, func, {})
end

local function deepEqualsImpl(v1, v2, visited1, visited2)
	if v1 == v2 then
		return true
	end

	if type(v1) ~= "table" or type(v2) ~= "table" then
		return false
	end

	visited1 = visited1 or {}
	visited2 = visited2 or {}

	if visited1[v1] or visited2[v2] then
		error("Deep equality comparison is not supported for cyclic tables")
	end

	visited1[v1] = true
	visited2[v2] = true

	for k, v in pairs(v1) do
		local other = v2[k]
		if other == nil or not deepEqualsImpl(v, other, visited1, visited2) then
			return false
		end
	end

	for k, v in pairs(v2) do
		if v1[k] == nil then
			return false
		end
	end

	visited1[v1] = nil
	visited2[v2] = nil

	return true
end

local function deepEquals(v1, v2)
	return deepEqualsImpl(v1, v2)
end

local function deepCopyImpl(value, visited)
	if type(value) ~= "table" then
		return value
	end

	if visited[value] then
		return visited[value]
	end

	local copy = {}
	visited[value] = copy

	for k, v in pairs(value) do
		copy[k] = deepCopyImpl(v, visited)
	end

	return copy
end

local function deepCopy(value)
	return deepCopyImpl(value, {})
end

local function readOnlyTableImpl(tbl)
	return setmetatable({}, {
		__index = tbl,
		__newindex = function(tbl2, key, value)
			error("Attempt to modify read-only table")
		end
	});
end

local function readOnlyTable(tbl)
	return mapTable(deepCopy(tbl), readOnlyTableImpl)
end

return {
	mapTable = mapTable,
	deepEquals = deepEquals,
	deepCopy = deepCopy,
	readOnlyTable = readOnlyTable
}
