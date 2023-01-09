local listMerge = {}

local utils = require "system.utils.Utilities"

local arrayCopy = utils.arrayCopy
local floor = math.floor

local function appendRemainderToTable(tbl1, tbl2, insertIndex, startIndex, endIndex)
	for i = 0, endIndex - startIndex do
		tbl1[insertIndex + i + 1] = tbl2[startIndex + i]
	end
	return tbl1
end

function listMerge.mergeListPair(list1, list2, comparator)
	local result = {}
	local resultIndex = 1
	local index1, index2 = 1, 1
	local size1, size2 = #list1, #list2

	if size1 == 0 then
		return arrayCopy(list2)
	elseif size2 == 0 then
		return arrayCopy(list1)
	else
		while true do
			if comparator(list1[index1], list2[index2]) then
				result[resultIndex] = list1[index1]
				index1 = index1 + 1
				if index1 > size1 then
					return appendRemainderToTable(result, list2, resultIndex, index2, size2)
				end
			else
				result[resultIndex] = list2[index2]
				index2 = index2 + 1
				if index2 > size2 then
					return appendRemainderToTable(result, list1, resultIndex, index1, size1)
				end
			end
			resultIndex = resultIndex + 1
		end
	end
end

local mergeListPair = listMerge.mergeListPair

function listMerge.mergeLists(lists, comparator)
	local listCount = #lists

	while listCount > 1 do
		local halfListCount = floor(listCount * 0.5)
		for i = 1, halfListCount do
			lists[i] = mergeListPair(lists[i * 2 - 1], lists[i * 2], comparator)
		end
		if listCount % 2 == 1 then
			lists[halfListCount + 1] = lists[listCount]
			listCount = halfListCount + 1
		else
			listCount = halfListCount
		end
	end

	return lists[1] or {}
end

return listMerge
