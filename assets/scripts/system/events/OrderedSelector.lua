local orderedSelector = {}

local abstractSelector = require "system.events.AbstractSelector"
local events = require "system.events.Events"


local function compareEntries(a, b)
	if a.index ~= b.index then
		return a.index < b.index
	elseif a.priority ~= b.priority then
		return a.priority > b.priority
	else
		return a.name < b.name
	end
end

function orderedSelector.new(eventType, orderKeys, options)

	local keyTable = {}
	local sortedEntries = {}
	local modCount = -1
	local eventTypeName = eventType.getName()

	for i = 1, #orderKeys do
		if keyTable[orderKeys[i]] == nil then
			keyTable[orderKeys[i]] = i
		else
			error("Duplicate key '" .. orderKeys[i] .. "' in key list for event selector '" .. eventTypeName .. "'")
		end
	end

	local function sortEntries()
		sortedEntries = {}
		for name, entry in pairs(events.getRegisteredFunctions(eventTypeName)) do

			local order, priority
			if type(entry.key) == "table" then
				order = entry.key.order
				priority = entry.key.priority
			else
				order = entry.key
			end

			-- Cache sort index value
			local index = keyTable[order]
			if index == nil then
				error("Ordering key '" .. tostring(order) .. "' (used by '" .. name .. "', defined in script '" ..
					entry.script .. "') does not exist in event '" .. eventTypeName .. "'")
			end

			entry.index = index
			entry.priority = priority or 0
			entry.name = name

			sortedEntries[#sortedEntries + 1] = entry
		end

		table.sort(sortedEntries, compareEntries)
	end

	local function getSortedEntries()
		local newModCount = events.getModificationCount(eventTypeName)
		if modCount ~= newModCount then
			modCount = newModCount
			sortEntries()
		end
		return sortedEntries
	end

	return abstractSelector.new {
		eventType = eventType,
		entriesFunc = getSortedEntries,
		compareFunc = compareEntries,
		options = options,
	}
end

return orderedSelector
