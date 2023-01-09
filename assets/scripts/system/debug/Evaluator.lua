local evaluator = {}

-- TODO: disable evaluator in release builds
local enabled = true

function evaluator.isEnabled()
	return enabled
end

local function packPCallResult(...)
	if select(1, ...) then
		local size = select("#", ...) - 1
		local resultTable = {[0] = size}
		for i = 1, size do
			resultTable[i] = select(i + 1, ...)
		end
		return true, resultTable
	else
		return false, select(2, ...)
	end
end

local function pcallPacked(...)
	return packPCallResult(pcall(...))
end

function evaluator.execute(source, environment)
	if not enabled then
		return false, "Dynamic expression evaluation is disabled"
	end

	local func, err = loadstring(source, "@eval")
	if err then
		return false, err
	end
	setfenv(func, environment)
	return pcallPacked(func)
end

return evaluator
