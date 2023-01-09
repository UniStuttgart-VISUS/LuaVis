local imageCache = {}

local fileio = require "system.game.FileIO"

cache = {}

function imageCache.get(path)
	if not cache[path] then
		cache[path] = fileio.listFiles(path)
		table.sort(cache[path])
	end
	return cache[path]
end

return imageCache
