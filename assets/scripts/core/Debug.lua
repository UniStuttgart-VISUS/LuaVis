local inspect = require("core.Inspect").inspect
local logger = require("core.Logger").createLoggerForScript("Debug")

local function writeToDebugLog(str)
	logger.info("%s", str)
end

local function dumpValue(...)
	if select("#", ...) ~= 0 then
		local pieces = {}
		for i = 1, select("#", ...) do
			local piece = select(i, ...)
			pieces[#pieces + 1] = inspect(piece)
		end
		writeToDebugLog(table.concat(pieces, " "))
	end
end

return setmetatable({},
{
	__call = function (tbl, ...)
		dumpValue(...)
	end,
	__index = {
		trace = function (...)
			dumpValue(...)
			writeToDebugLog(debug.traceback())
		end,
		locals = function (...)
			dumpValue(...)
			local index = 1
			while true do
				local name, value = debug.getlocal(2, index)
				if name == nil then
					break
				end
				writeToDebugLog(name .. " = " .. inspect(value))
				index = index + 1
			end
		end,
		upvalues = function (...)
			dumpValue(...)
			local index = 1
			local func = debug.getinfo(2, "f").func
			while true do
				local name, value = debug.getupvalue(func, index)
				if name == nil then
					break
				end
				writeToDebugLog(name .. " = " .. inspect(value))
				index = index + 1
			end
		end,
	},
	__newindex = function ()
		error("Attempt to write to read-only debug table", 2)
	end
})
