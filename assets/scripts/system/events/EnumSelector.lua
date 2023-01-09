local enumSelector = {}

local abstractSelector = require "system.events.AbstractSelector"
local events = require "system.events.Events"


local function compareEntries(a, b)
	if a.priority ~= b.priority then
		return a.priority > b.priority
	else
		return a.name < b.name
	end
end

function enumSelector.new(eventType, enumTable, options)

	local sortedEntries = {}
	local modCount = -1
	local eventTypeName = eventType.getName()
	local allEntriesPlaceholder = {}

	local function sortEntries(enumValue)
		local sortedEnumEntries = {}
		for name, entry in pairs(events.getRegisteredFunctions(eventTypeName)) do

			local key, priority
			if type(entry.key) == "table" then
				key = entry.key.key
				priority = entry.key.priority
			else
				key = entry.key
			end

			local match
			if key == nil or enumValue == allEntriesPlaceholder then
				-- "nil" matches all enum entries
				match = true
			else
				-- In case of no enum table being supplied, use identity mapping
				local index = enumTable and enumTable[key] or key
				if index == nil then
					error("Enum key '" .. tostring(key) .. "' (used by '" .. name .. "', defined in script '" ..
						entry.script .. "') does not exist for event '" .. eventTypeName .. "'")
				end
				match = (index == enumValue)
			end

			if match then
				entry.name = name
				entry.priority = priority or 0
				sortedEnumEntries[#sortedEnumEntries + 1] = entry
			end
		end

		table.sort(sortedEnumEntries, compareEntries)

		sortedEntries[enumValue] = sortedEnumEntries
	end

	local function getSortedEnumEntries(enumValue)
		if enumValue == nil then
			enumValue = allEntriesPlaceholder
		end
		local newModCount = events.getModificationCount(eventTypeName)
		if modCount ~= newModCount then
			modCount = newModCount
			sortedEntries = {}
		end
		local sortedEnumEntries = sortedEntries[enumValue]
		if sortedEnumEntries == nil then
			sortEntries(enumValue)
			sortedEnumEntries = sortedEntries[enumValue]
		end
		return sortedEnumEntries
	end

	return abstractSelector.new {
		eventType = eventType,
		entriesFunc = getSortedEnumEntries,
		compareFunc = compareEntries,
		options = options,
	}
end

return enumSelector
