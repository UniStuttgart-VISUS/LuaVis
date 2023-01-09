local proxy = {}

local newproxy = newproxy
local getmetatable = getmetatable
local setmetatable = setmetatable

function proxy.setMetatable(tbl, metatable)
	local instance = newproxy(true)
	getmetatable(instance).__gc = metatable.__gc
	metatable[instance] = true
	return setmetatable(tbl, metatable)
end

return proxy
