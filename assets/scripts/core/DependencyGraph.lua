local depGraph = {}

local function reverseTable(tbl)
	local left, right = 1, #tbl
	while left < right do
		tbl[left], tbl[right] = tbl[right], tbl[left]
		left = left + 1
		right = right - 1
	end
end

local function traverseImpl(links, node, result, visited)
	-- Visitation map stores 3 states per node:
	-- nil: node has not been seen
	-- true: node is currently being visited (cycle!)
	-- false: node has been visited previously
	if visited[node] == true then
		return node
	elseif visited[node] == false then
		return nil
	else
		if links[node] then
			visited[node] = true
			for k, v in pairs(links[node]) do
				local cycle = traverseImpl(links, k, result, visited)
				if cycle then
					return cycle
				end
			end
		end
		visited[node] = false
		table.insert(result, node)
	end
end

local function traverse(links, node)
	local result = {}
	local cycle = traverseImpl(links, node, result, {})
	if cycle then
		return nil, cycle
	else
		result[#result] = nil
		reverseTable(result)
		return result
	end
end

local function traverseCheck(links, from, to)
	local visited = {}
	local cycle = traverseImpl(links, from, {}, visited)
	return visited[to] ~= nil
end

local function establishLink(links, from, to)
	local linkSubTable = links[from]
	if linkSubTable == nil then
		linkSubTable = {}
		links[from] = linkSubTable
	end
	linkSubTable[to] = true
end

local function breakLink(links, from, to)
	local linkSubTable = links[from]
	if linkSubTable ~= nil then
		linkSubTable[to] = nil
		if next(linkSubTable) == nil then
			links[from] = nil
		end
	end
end

local function getKeyList(tbl)
	local keys = {}
	for k in pairs(tbl) do
		keys[#keys + 1] = k
	end
	return keys
end

local function breakAllLinks(links, backLinks, node)
	if links[node] then
		local linkList = getKeyList(links[node])
		for i = 1, #linkList do
			breakLink(links, node, linkList[i])
			breakLink(backLinks, linkList[i], node)
		end
	end
end

function depGraph.new()
	local links, backLinks = {}, {}
	return setmetatable({}, {
		__index = {
			clear = function ()
				links, backLinks = {}, {}
			end,
			addLink = function (from, to)
				establishLink(links, from, to)
				establishLink(backLinks, to, from)
			end,
			removeLink = function (from, to)
				breakLink(links, from, to)
				breakLink(backLinks, to, from)
			end,
			traverseForward = function (startNode)
				return traverse(links, startNode)
			end,
			traverseBackward = function (startNode)
				return traverse(backLinks, startNode)
			end,
			checkConnection = function (from, to)
				return traverseCheck(links, from, to)
			end,
			unlink = function (node)
				breakAllLinks(links, backLinks, node)
				breakAllLinks(backLinks, links, node)
			end,
			unlinkOutgoing = function (node)
				breakAllLinks(links, backLinks, node)
			end,
			unlinkIncoming = function (node)
				breakAllLinks(backLinks, links, node)
			end,
			getAllLinks = function ()
				local res, res2 = {}, {}
				for k1, v in pairs(links) do
					for k2 in pairs(v) do
						res[#res + 1] = {k1, k2}
					end
				end
				for k1, v in pairs(backLinks) do
					for k2 in pairs(v) do
						res2[#res2 + 1] = {k2, k1}
					end
				end
				local function sortFunc(a, b)
					if a[1] < b[1] then
						return true
					elseif a[1] > b[1] then
						return false
					else
						return a[2] < b[2]
					end
				end
				table.sort(res, sortFunc)
				table.sort(res2, sortFunc)
				return res, res2
			end,
		},
		__newindex = function (tbl, key, value)
			error("Attempt to write to dependency graph")
		end
	})
end

return depGraph
