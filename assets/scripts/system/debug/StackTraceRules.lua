local stackTraceRules = {}

local utils = require "system.utils.Utilities"

rules = {}

local sortedRules

function stackTraceRules.add(name, order, func)
	rules[name] = {order = order or 0, func = func}
	sortedRules = nil
end

function stackTraceRules.apply(trace)
	if not sortedRules then
		sortedRules = utils.flatten(rules)
		table.sort(sortedRules, function (r1, r2)
			if r1[2].order ~= r2[2].order then
				return r1[2].order < r2[2].order
			else
				return r1[1] < r2[1]
			end
		end)
	end
	for _, rule in ipairs(sortedRules) do
		rule[2].func(trace)
	end
end

return stackTraceRules
