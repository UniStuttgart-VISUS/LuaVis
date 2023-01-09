local entitySelector = {}

local abstractSelector = require "system.events.AbstractSelector"
local ecs = require "system.game.Entities"
local events = require "system.events.Events"


local globalECSModCount = 0

local function compareEntries(a, b)
	if a.index ~= b.index then
		return a.index < b.index
	elseif a.priority ~= b.priority then
		return a.priority > b.priority
	else
		return a.name < b.name
	end
end

function entitySelector.new(eventType, orderKeys, options)

	local keyTable = {}
	local sortedEntries = {}
	local eventModCount, ecsModCount = -1, -1
	local eventTypeName = eventType.getName()

	for i = 1, #orderKeys do
		if keyTable[orderKeys[i]] == nil then
			keyTable[orderKeys[i]] = i
		else
			error("Duplicate key '" .. orderKeys[i] .. "' in key list for event selector '" .. eventTypeName .. "'")
		end
	end

	local function entityHasComponents(entity, components)
		if type(components) == "table" then
			for i = 1, #components do
				if type(components[i]) == "string" and not entity:hasComponent(components[i]) then
					return false
				end
			end
		elseif type(components) == "string" then
			return entity:hasComponent(components)
		end
		return true
	end

	local function sortEntries(entityType)
		local entity = ecs.getEntityPrototype(entityType)
		local sortedEntityEntries = {}
		for name, entry in pairs(events.getRegisteredFunctions(eventTypeName)) do

			local order, priority, filter
			if type(entry.key) == "table" then
				order = entry.key.order
				priority = entry.key.priority
				filter = entry.key.filter
			else
				order = entry.key
			end

			if entityHasComponents(entity, filter) then
				local index = keyTable[order]
				if index == nil then
					error("Ordering key '" .. tostring(order) .. "' (used by '" .. name .. "', defined in" ..
						" script '" .. entry.script .. "') does not exist in event '" .. eventTypeName .. "'")
				end

				entry.index = index
				entry.priority = priority or 0
				entry.name = name

				sortedEntityEntries[#sortedEntityEntries + 1] = entry
			end
		end

		table.sort(sortedEntityEntries, compareEntries)

		sortedEntries[entityType] = sortedEntityEntries
	end

	local function getSortedEntityEntries(entityType)
		local newEventModCount = events.getModificationCount(eventTypeName)
		local newECSModCount = globalECSModCount
		if eventModCount ~= newEventModCount or ecsModCount ~= newECSModCount then
			eventModCount = newEventModCount
			ecsModCount = newECSModCount
			sortedEntries = {}
		end
		local sortedEntityEntries = sortedEntries[entityType]
		if sortedEntityEntries == nil then
			sortEntries(entityType)
			sortedEntityEntries = sortedEntries[entityType]
		end
		return sortedEntityEntries
	end

	return abstractSelector.new {
		eventType = eventType,
		entriesFunc = getSortedEntityEntries,
		compareFunc = compareEntries,
		options = options,
	}
end

event.ecsSchemaReloaded.add("entitySelector", "clearCache", function ()
	globalECSModCount = globalECSModCount + 1
end)

return entitySelector
