local orders = {}

function orders.new(keyList, orderName)

	local keyTable = {}
	local entryTable = {}
	local sortedEntries
	local needSort = true

	local function sortEntries()
		if not needSort then
			return
		end
		
		sortedEntries = {}
		for _, entry in pairs(entryTable) do

			-- Cache sort index value
			local index = keyTable[entry.orderKey]
			if index == nil then
				error("Key '" .. entry.orderKey .. "' (used by '" .. entry.name ..
					"') does not exist in order '" .. orderName .. "'")
			else
				entry.index = index
			end

			sortedEntries[#sortedEntries + 1] = entry
		end

		table.sort(sortedEntries, function(a, b)
			if a.index ~= b.index then
				return a.index < b.index
			else
				return a.name < b.name
			end
		end)

		needSort = false
	end

	local order
	order = {
		add = function(name, orderKey, value)
			if name == nil then
				error("No order entry name specified")
			end

			if value == nil then
				entryTable[name] = nil
				needSort = true
			else
				local entry = entryTable[name]
				if entry == nil then
					entry = {}
					entry.name = name
					entryTable[name] = entry
				end
				entry.value = value
				if entry.orderKey ~= orderKey then
					entry.orderKey = orderKey
					needSort = true
				end
			end
		end,

		remove = function(name)
			order.add(name, nil, nil)
		end,

		forEach = function(func)
			sortEntries()
			for i = 1, #sortedEntries do
				local entry = sortedEntries[i]
				func(entry.value, entry.name)
			end
		end,

		setKeys = function(list)

			list = list or {}

			local newKeyTable = {}

			for i, v in ipairs(list) do
				if newKeyTable[v] == nil then
					newKeyTable[v] = i
				else
					error("Duplicate key " .. v .. " in order")
				end
			end

			keyTable = newKeyTable
			needSort = true

		end,

		hasEntries = function()
			return next(entryTable) ~= nil
		end,

		hasKeys = function()
			return next(keyTable) ~= nil
		end
	}

	order.setKeys(keyList)

	return order
end

return orders
