local mergeSelector = {}

local abstractSelector = require "system.events.AbstractSelector"
local listMerge = require "system.utils.ListMerge"

function mergeSelector.new(prototypeSelector, wrapperFunction, options)

	local eventType = prototypeSelector.eventType
	local compareFunc = prototypeSelector.compareEntries

	local function getMergedEntries(parameter)
		return listMerge.mergeLists(parameter.entries, compareFunc)
	end

	return abstractSelector.new {
		eventType = eventType,
		entriesFunc = getMergedEntries,
		compareFunc = compareFunc,
		wrapperFunc = wrapperFunction,
		options = options,
	}
end

-- Initializes a table for firing an event on multiple selectors.
function mergeSelector.initMergeTable()
	return {entries = {}, userData = {}}
end

--- Adds a set of entries to the merge table.
-- @param mergeTable
--        The merge table to append the entries to.
-- @param selector
--        The selector to get the entries from (e.g. a specific entitySelector instance).
-- @param selectionParameter
--        The selection parameter to supply to the selector (e.g. entity ID).
-- @param userData
--        The custom data to supply for each entry added to the merge table from this call.
--        This can be accessed in the wrapper function by indexing the userData table using the entry table as a key.
function mergeSelector.addMergeTableEntries(mergeTable, selector, selectionParameter, userData)
	local selectorEntries = selector.getEntries(selectionParameter)
	if #selectorEntries ~= 0 then
		local entries = mergeTable.entries
		entries[#entries + 1] = selectorEntries
		for i = 1, #selectorEntries do
			mergeTable.userData[selectorEntries[i]] = userData
		end
	end
end

return mergeSelector
