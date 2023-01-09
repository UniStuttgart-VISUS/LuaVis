local fileio = {}

--- Flags for determining the result of resource listings
--- @class ListFlags
--- @field FILES integer List regular files
--- @field DIRECTORIES integer List subdirectories
--- @field FULL_PATH integer List each resource's absolute path, prepending the 'path' parameter to the relative path
--- @field SORTED integer Sort results alphabetically
--- @field RECURSIVE integer Recursively list the contents of subdirectories.
fileio.List = bridge.res.getListFlags()

local defaultListFlags = bit.bor(fileio.List.FILES, fileio.List.SORTED)

function fileio.readFileToString(fileName)
	return bridge.res.readFileToString(fileName)
end

function fileio.listFiles(path, ...)
	return bridge.res.listResources(path, select("#", ...) == 0 and defaultListFlags or bit.bor(...))
end

function fileio.exists(path)
	return bridge.res.resourceExists(path)
end

return fileio
