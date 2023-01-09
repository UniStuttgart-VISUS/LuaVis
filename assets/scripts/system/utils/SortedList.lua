local sortedList = {}

local utils = require "system.utils.Utilities"


local lowerBound = utils.lowerBound
local lowerBoundOrdered = utils.lowerBoundOrdered


function sortedList.contains(list, value)
	return list[lowerBound(list, value)] == value
end

function sortedList.insert(list, value)
	local insertionPoint = lowerBound(list, value)
	for i = #list, insertionPoint, -1 do
		list[i + 1] = list[i]
	end
	list[insertionPoint] = value
end

function sortedList.remove(list, value)
	local removalPoint = lowerBound(list, value)
	if list[removalPoint] == value then
		for i = removalPoint, #list do
			list[i] = list[i + 1]
		end
		return true
	else
		return false
	end
end


function sortedList.containsOrdered(list, value, compare)
	return list[lowerBoundOrdered(list, value, compare)] == value
end

function sortedList.insertOrdered(list, value, compare)
	local insertionPoint = lowerBoundOrdered(list, value, compare)
	for i = #list, insertionPoint, -1 do
		list[i + 1] = list[i]
	end
	list[insertionPoint] = value
end

function sortedList.removeOrdered(list, value, compare)
	local removalPoint = lowerBoundOrdered(list, value, compare)
	if list[removalPoint] == value then
		for i = removalPoint, #list do
			list[i] = list[i + 1]
		end
		return true
	else
		return false
	end
end

return sortedList
