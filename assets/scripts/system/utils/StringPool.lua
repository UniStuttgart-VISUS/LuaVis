local stringPool = {}

local type = type

function stringPool.new(maxEntryCount)

	local nameToIdMap = {}
	local idToNameMap = {}

	local function addEntry(name)
		local id = #idToNameMap + 1
		if id >= maxEntryCount then
			error("Maximum string pool entry count (" .. maxEntryCount .. ") reached", 3)
		end
		idToNameMap[id] = name
		nameToIdMap[name] = id
		return id
	end

	local function getEntryByName(name)
		if name == nil or name == "" then
			return 0
		elseif type(name) ~= "string" then
			error("String pool entry lookup type must be a string, but was " .. type(name), 3)
		end
		return nameToIdMap[name]
	end

	return setmetatable({}, {
		__index = {
			lookUpByName = function (name)
				local id = getEntryByName(name)
				if id == nil then
					id = addEntry(name)
				end
				return id
			end,
			lookUpByID = function (id)
				return idToNameMap[id]
			end,
			reset = function ()
				nameToIdMap = {}
				idToNameMap = {}
			end,
		},
		__newindex = function (tbl, key, value)
			error("Attempt to modify string pool", 2)
		end,
	})
end

return stringPool
