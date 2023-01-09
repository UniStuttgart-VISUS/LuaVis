-- Rectangle objects with several mathematical operations.
-- Original code for vectors under MIT license by DarkWiiPlayer, modified by Marukyu for LuaVis.

local rect = {}

local rectType

local ffi = require "ffi"
local vector2 = require "system.utils.Vector2"

local isVector2 = vector2.isVector2

local abs = math.abs
local sin = math.sin
local cos = math.cos
local sqrt = math.sqrt
local atan2 = math.atan2
local type = type

local EPSILON = 0.000001


local function isNumber(obj)
	return type(obj) == "number"
end

function rect.isRect(obj)
	return ffi.istype(rectType, obj)
end

local isRect = rect.isRect

local function metaop(name)
	local func = rect[name]
	return function(a, b)
		if isRect(a) then
			return func(a, b)
		else
			return func(b, a)
		end
	end
end

function rect.new(x, y, w, h)
	if isNumber(x) then
		return rectType(x, y, w, h)
	elseif isVector2(x) then
		return rectType(x.x, x.y, y.x, y.y)
	elseif isRect(x) then
		return rectType(x.x, x.y, x.w, x.h)
	else
		return rectType(0, 0, 0, 0)
	end
end

function rect.scale(r, factor)
	if isNumber(factor) then
		return rectType(r.x, r.y, r.w * factor, r.h * factor)
	elseif isVector2(factor) then
		return rectType(r.x, r.y, r.w * factor.x, r.h * factor.y)
	else
		error("Attempt to scale " .. type(r) .. " by " .. type(factor), 2)
	end
end

function rect.resize(r, size)
	if isNumber(size) then
		return rectType(r.x, r.y, r.w * size, r.h * size)
	else
		error("Attempt to resize " .. type(r) .. " to " .. type(size), 2)
	end
end

function rect.grow(r, diff)
	if isVector2(diff) then
		return rectType(r.x - diff.x, r.y - diff.y, r.w + diff.x * 2, r.h + diff.y * 2)
	elseif isNumber(diff) then
		return rectType(r.x - diff, r.y - diff, r.w + diff * 2, r.h + diff * 2)
	else
		error("Attempt to grow " .. type(r) .. " by " .. type(diff), 2)
	end
end

function rect.add(r, offset)
	if isVector2(offset) then
		return rectType(r.x + offset.x, r.y + offset.y, r.w, r.h)
	else
		error("Attempt to add " .. type(r) .. " and " .. type(offset), 2)
	end
end

function rect.subtract(r, offset)
	if isVector2(offset) then
		return rectType(r.x - offset.x, r.y - offset.y, r.w, r.h)
	else
		error("Attempt to subtract " .. type(r) .. " and " .. type(offset), 2)
	end
end

function rect.position(r, v)
	if isVector2(v) then
		return vector2(r.x + r.w * v.x, r.y + r.h * v.y)
	else
		return vector2(r.x, r.y)
	end
end

function rect.topLeft(r)
	return vector2(r.x, r.y)
end

function rect.topRight(r)
	return vector2(r.x + r.w, r.y)
end

function rect.bottomLeft(r)
	return vector2(r.x, r.y + r.h)
end

function rect.bottomRight(r)
	return vector2(r.x + r.w, r.y + r.h)
end

function rect.center(r)
	return vector2(r.x + r.w * 0.5, r.y + r.h * 0.5)
end

function rect.centerWithin(outer, inner)
	if isVector2(inner) then
		return rect(outer:center() - inner * 0.5, inner)
	elseif isRect(inner) then
		return rect.centerWithin(outer, vector2(inner.w, inner.h))
	else
		error("Attempt to center " .. type(inner) .. " within " .. type(outer), 2)
	end
end

function rect.size(r)
	return vector2(r.w, r.h)
end

function rect.equals(r1, r2)
	if isRect(r2) then
		return r1.x == r2.x and r1.y == r2.y
		   and r1.w == r2.w and r1.h == r2.h
	else
		error("Attempt to compare " .. type(r1) .. " with " .. type(r2), 2)
	end
end

function rect.nearEquals(r1, r2)
	if isRect(r2) then
		return abs(r1.x - r2.x) < EPSILON and abs(r1.y - r2.y) < EPSILON
		   and abs(r1.w - r2.w) < EPSILON and abs(r1.h - r2.h) < EPSILON
	else
		error("Attempt to compare " .. type(r1) .. " with " .. type(r2), 2)
	end
end

rectType = ffi.metatype([[
	struct {
		double x, y, w, h;
	}
]], {
	__index = rect,

	__add = metaop "add",
	__sub = metaop "subtract",
	__eq  = metaop "equals",
	__mul = metaop "scale",

	__tostring = function(r)
		return "(" .. r.x .. ", " .. r.y .. "; " .. r.w .. "x" .. r.h .. ")"
	end,
})

local rectNew = rect.new

setmetatable(rect, {
	__call = function (t, x, y, w, h)
		return rectNew(x, y, w, h)
	end
})

return rect
