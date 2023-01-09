local stringUtils = {}

local utils = require "system.utils.Utilities"

function stringUtils.equalsIgnoreCase(str1, str2)
	if str1 == nil or str2 == nil then
		return str1 == str2
	else
		-- TODO optimize this by performing proper case-insensitive comparison instead of forcing strings lowercase
		return string.lower(str1) == string.lower(str2)
	end
end

function stringUtils.split(str, delimiter)
	local result = {}
	local start = 1
	local delimiterStart, delimiterEnd = string.find(str, delimiter, start)
	while delimiterStart do
		table.insert(result, string.sub(str, start, delimiterStart - 1))
		start  = delimiterEnd + 1
		delimiterStart, delimiterEnd = string.find(str, delimiter, start)
	end
	table.insert(result, string.sub(str, start))
	return result
end

function stringUtils.capitalizeFirst(str)
	return string.upper(string.sub(str, 1, 1)) .. string.sub(str, 2)
end

function stringUtils.startsWith(str, startStr)
	return string.sub(str, 1, #startStr) == startStr
end

function stringUtils.endsWith(str, endStr)
	return #endStr == 0 or string.sub(str, -#endStr) == endStr
end

function stringUtils.toUnicode(text, forceCopy)
	if text == nil then
		return {}
	elseif type(text) == "string" then
		local textTable = {}
		for i = 1, #text do
			textTable[i] = string.byte(text, i)
		end
		return textTable
	elseif type(text) == "table" then
		if forceCopy then
			return utils.deepCopy(text)
		else
			return text
		end
	else
		error("Expected string or table, got " .. type(text), 2)
	end
end

function stringUtils.fromUnicode(text)
	if type(text) == "string" then
		return text
	elseif type(text) == "table" then
		local charTable = {}
		for i = 1, #text do
			charTable[i] = string.char(text[i])
		end
		return table.concat(charTable)
	else
		error("Expected string or table, got " .. type(text), 2)
	end
end

function stringUtils.parseQueryString(queryString)
	-- TODO add escape character
	local querySplit = stringUtils.split(queryString, "%?")
	local args = {}
	if querySplit[2] then
		for _, keyValue in ipairs(stringUtils.split(querySplit[2], "%&")) do
			local keyValueSplit = stringUtils.split(keyValue, "%=")
			if keyValueSplit[2] then
				-- Query argument contains equals sign: add named parameter
				args[keyValueSplit[1]] = keyValueSplit[2]
			else
				-- Query argument contains no equals sign: add positional parameter
				args[#args + 1] = keyValueSplit[1]
			end
		end
	end
	return querySplit[1], args
end

function stringUtils.buildQueryString(base, args)
	-- TODO check for and escape ?, & and =
	local parts = {base}
	for i = 1, #args do
		parts[#parts + 1] = #parts == 1 and "?" or "&"
		parts[#parts + 1] = args[i]
	end
	for k, v in pairs(args) do
		parts[#parts + 1] = #parts == 1 and "?" or "&"
		parts[#parts + 1] = k
		parts[#parts + 1] = "="
		parts[#parts + 1] = v
	end
	return table.concat(parts)
end

return stringUtils
