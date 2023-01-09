local functionNameRegistry = {}

registry = nil

local function getRegistry()
	if registry == nil then
		registry = setmetatable({}, {
			__mode = "k"
		})
	end
	return registry
end

function functionNameRegistry.add(func, name)
	getRegistry()[func] = name
end

function functionNameRegistry.get(func)
	return getRegistry()[func]
end

return functionNameRegistry
