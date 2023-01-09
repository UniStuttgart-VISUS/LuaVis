local timer = {}

local C = require("ffi").C

local function getClock()
	return C.wosC_sys_getClockMicroseconds()
end

function timer.new()
	local startTime
	local timerObject = setmetatable({}, {
		__index = {
			getTime = function ()
				return getClock() - startTime
			end,
			reset = function ()
				startTime = getClock()
			end
		},
		__newindex = function (tbl, key, value)
			error("Attempt to write to timer")
		end
	})
	startTime = getClock()
	return timerObject
end

function timer.newPausable()
	local startTime, addedTime
	local timerObject = setmetatable({}, {
		__index = {
			getTime = function ()
				return (startTime and getClock() - startTime or 0) + addedTime
			end,
			reset = function ()
				if startTime ~= nil then
					startTime = getClock()
				end
				addedTime = 0
			end,
			pause = function ()
				if startTime ~= nil then
					addedTime = addedTime + getClock() - startTime
					startTime = nil
				end
			end,
			unpause = function ()
				if startTime == nil then
					startTime = getClock()
				end
			end,
			isPaused = function ()
				return startTime ~= nil
			end,
		},
		__newindex = function (tbl, key, value)
			error("Attempt to write to timer")
		end
	})
	startTime = getClock()
	addedTime = 0
	return timerObject
end

function timer.getGlobalTime()
	return getClock()
end

function timer.seconds(secs)
	return secs * 1000000
end

function timer.milliseconds(millisecs)
	return millisecs * 1000
end

function timer.microseconds(microsecs)
	return microsecs
end

return timer
