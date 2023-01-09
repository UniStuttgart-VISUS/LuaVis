-- Track all declared variable names
local declaredNames = {}

-- Mark all current non-nil globals as tracked
for k, v in pairs(_G) do
	declaredNames[k] = true
end

-- Disallow reads/writes to undeclared variables.
setmetatable(_G, {
	__newindex = function (tbl, name, value)
		if not declaredNames[name] then
			error("Attempt to write to undeclared variable " .. name, 2)
		else
			rawset(tbl, name, value)
		end
	end,
	__index = function (tbl, name)
		if not declaredNames[name] then
			error("Attempt to read undeclared variable " .. name, 2)
		else
			return nil
		end
	end
})
