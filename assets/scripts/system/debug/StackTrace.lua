local stackTrace = {}

-- Adapted from Kepler Project / Lua-Compat-5.3 (https://github.com/keplerproject/lua-compat-5.3)
--
-- The MIT License (MIT)
--
-- Copyright (c) 2015 Kepler Project.
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy of
-- this software and associated documentation files (the "Software"), to deal in
-- the Software without restriction, including without limitation the rights to
-- use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
-- the Software, and to permit persons to whom the Software is furnished to do so,
-- subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.

local debug = require "debug"
local standardLibrary = require "core.StandardLibrary"
local utils = require "system.utils.Utilities"
local stackTraceRules = require "system.debug.StackTraceRules"

local maxStackFramesTop = 128
local maxStackFramesBottom = 128

local maxFormattedStackFramesTop = 12
local maxFormattedStackFramesBottom = 10

local omittedStackFrame = {
	currentline = -1,
	short_src = "omitted",
	namewhat = "function",
	name = "unknown",
}

local function findFunctionInTable(func, tbl, maxDepth)
	if maxDepth == 0 or type(tbl) ~= "table" then
		return nil
	end
	for k, v in pairs(tbl) do
		if type(k) == "string" then
			if rawequal(func, v) then
				return k
			end
			local recur = findFunctionInTable(func, v, maxDepth - 1)
			if recur then
				return k .. "." .. recur
			end
		end
	end
	return nil
end

local function getNumberOfStackFrames()
	local lowerSearchBound, upperSearchBound = 1, 1

	-- Perform exponential search to identify bounds for binary search.
	while debug.getinfo(upperSearchBound, "") do
		lowerSearchBound = upperSearchBound
		upperSearchBound = upperSearchBound * 2
	end

	-- Perform binary search to find exact 
	while lowerSearchBound < upperSearchBound do
		local middle = math.floor((lowerSearchBound + upperSearchBound) * 0.5)
		if debug.getinfo(middle, "") then
			lowerSearchBound = middle + 1
		else
			upperSearchBound = middle
		end
	end
	return upperSearchBound - 1
end

local function getLocalVariables(level)
	local vars = {}
	local names = {}
	local index = 1
	while true do
		local name, value = debug.getlocal(level + 1, index)
		if name == nil then
			break
		end
		names[index] = name
		vars[name] = value
		index = index + 1
	end
	return vars, names
end

local function getUpvalues(func)
	local vars = {}
	local names = {}
	local index = 1
	while true do
		local name, value = debug.getupvalue(func, index)
		if name == nil then
			break
		end
		names[index] = name
		vars[name] = value
		index = index + 1
	end
	return vars, names
end

local function generateStackTrace(level)
	local trace = {}
	level = (level or 0) + 1
	local frameCount = getNumberOfStackFrames() - level - 1
	for i = 1, frameCount do
		if i <= maxStackFramesTop or i > frameCount - maxStackFramesBottom then
			trace[i] = debug.getinfo(level + i, "Slnf")
			if trace[i] then
				trace[i].vars = {}
				trace[i].vars.locals, trace[i].vars.localNames = getLocalVariables(level + i)
				trace[i].vars.upvalues, trace[i].vars.upvalueNames = getUpvalues(trace[i].func)
			end
		else
			trace[i] = omittedStackFrame
		end
	end
	stackTraceRules.apply(trace)
	return trace
end

function stackTrace.generate(level)
	local trace = generateStackTrace((level or 0) + 1)
	return trace
end

function stackTrace.formatFrame(info)
	local line = string.format("\t%s:", info.short_src)
	if info.currentline > 0 then
		line = line .. string.format("%d:", info.currentline)
	end
	local location = "?"
	if info.global then
		location = string.format("function '%s'", info.global)
	elseif info.namewhat ~= "" then
		location = string.format("%s '%s'", info.namewhat, info.name)
	elseif info.what == "main" then
		location = "main chunk"
	elseif info.what ~= "C" then
		location = string.format("function <%s:%d>", info.short_src, info.linedefined)
	end
	line = line .. " in " .. location
	return line
end

local function addTraceLinesToOutput(trace, output, firstLine, lastLine)
	for i = firstLine, lastLine do
		output[#output + 1] = stackTrace.formatFrame(trace[i])
	end
end

function stackTrace.format(trace)
	local output = {"stack traceback:"}
	if #trace <= maxFormattedStackFramesTop + maxFormattedStackFramesBottom + 1 then
		addTraceLinesToOutput(trace, output, 1, #trace)
	else
		addTraceLinesToOutput(trace, output, 1, maxFormattedStackFramesTop)
		output[#output + 1] = "\t..."
		addTraceLinesToOutput(trace, output, #trace - maxFormattedStackFramesBottom + 1, #trace)
	end
	return table.concat(output, "\n")
end

function stackTrace.traceback(message, level)
	local traceString = stackTrace.format(generateStackTrace((level or 0) + 1))
	if message ~= nil then
		return tostring(message) .. "\n" .. traceString
	else
		return traceString
	end
end

stackTraceRules.add("IdentifyStandardLibraryFunctions", -100, function (trace)
	for i, info in ipairs(trace) do
		local path = findFunctionInTable(info.func, standardLibrary, 2)
		if path then
			info.namewhat = "function"
			info.global = path
		end
	end
end)

stackTraceRules.add("RemoveFlaggedEntries", 100, function (trace)
	-- Remove all flagged stack trace entries
	utils.removeIf(trace, function (info)
		return info.remove
	end)
end)

stackTraceRules.add("StripFunctionReferences", 10000, function (trace)
	-- Remove all function references for security reasons
	for i, info in ipairs(trace) do
		info.func = nil
	end
end)

return stackTrace
