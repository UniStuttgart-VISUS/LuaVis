local config = {}

local DataType = {
	MISSING = 0,
	NULL = 1,
	STRING = 2,
	BOOL = 3,
	INT = 4,
	FLOAT = 5,
	ARRAY = 6,
	MAP = 7,
}

local getInt = bridge.config.getInt
local getFloat = bridge.config.getFloat
local getString = bridge.config.getString
local getBool = bridge.config.getBool

local setInt = bridge.config.setInt
local setFloat = bridge.config.setFloat
local setString = bridge.config.setString
local setBool = bridge.config.setBool

local unset = bridge.config.unset

local next = next
local floor = math.floor
local type = type
local format = string.format

local function getDataType(key)
	return getInt(key .. "[type]") or DataType.MISSING
end

local function isArray(tbl)
	-- Tables with numeric keys and empty tables are considered arrays
	return tbl[1] ~= nil or next(tbl) == nil
end

function config.getString(key)
	return getString(key)
end

function config.getNumber(key)
	return getFloat(key)
end

function config.getBool(key)
	return getBool(key)
end

local getters, setters

local function get(key)
	local getter = getters[getDataType(key)]
	return getter and getter(key)
end

local function set(key, value)
	local setter = setters[type(value)]
	if setter then
		setter(key, value)
	else
		log.warn("Attempt to write value of type '%s' to config key '%s'", type(value), key)
	end
end

getters = {
	[DataType.MISSING] = function () return nil end,
	[DataType.NULL] = function () return nil end,
	[DataType.STRING] = getString,
	[DataType.BOOL] = getBool,
	[DataType.INT] = getFloat,
	[DataType.FLOAT] = getFloat,

	[DataType.ARRAY] = function (key)
		local result = {}
		-- Iterate over numeric indices and fetch values
		local length = getInt(key .. "[length]")
		if type(length) == "number" then
			for i = 1, length do
				result[i] = get(format("%s[%d]", key, i - 1))
			end
		end
		return result
	end,

	[DataType.MAP] = function (key)
		local result = {}
		-- Iterate over key list and fetch values
		local keyCount = getInt(key .. "[keys][length]")
		if type(keyCount) == "number" then
			for i = 1, keyCount do
				local entryKey = getString(format("%s[keys][%d]", key, i - 1))
				if entryKey then
					result[entryKey] = get(format("%s.%s", key, entryKey))
				end
			end
		end
		return result
	end,
}

setters = {
	string = setString,
	number = function (key, value)
		-- Store numbers without a fractional part as integers if they fit within 32 bits
		if value == floor(value) and value < 2^31 and value > -2^31 then
			return setInt(key, value)
		else
			return setFloat(key, value)
		end
	end,
	boolean = setBool,
	table = function (key, value)
		if isArray(value) then
			-- Treat as array: resize first, then set entries
			setInt(key .. "[length]", #value)
			for i = 1, #value do
				set(format("%s[%d]", key, i - 1), value[i])
			end
		else
			-- Treat as map: clear tree first, then set entries
			unset(key)
			for k, v in pairs(value) do
				set(format("%s.%s", key, k), v)
			end
		end
	end,
	["nil"] = unset,
}

function config.get(key)
	return get(key)
end

function config.set(key, value)
	return set(key, value)
end

return config
