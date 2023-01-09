-- Vector objects with several mathematical operations.
-- Original code under MIT license by DarkWiiPlayer, modified by Marukyu for LuaVis.

local vector2 = {}

local vectorType

local ffi = require "ffi"

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

function vector2.isVector2(obj)
	return ffi.istype(vectorType, obj)
end

local isVector2 = vector2.isVector2

local function metaop(name)
	local func = vector2[name]
	return function(a, b)
		if isVector2(a) then
			return func(a, b)
		else
			return func(b, a)
		end
	end
end

function vector2.new(x, y)
	if isNumber(x) then
		return vectorType(x, y or x)
	elseif isVector2(x) then
		return vectorType(x.x, x.y)
	else
		return vectorType(0, 0)
	end
end

function vector2.dot(v1, v2)
	if isVector2(v2) then
		return vectorType(v1.x * v2.x, v1.y * v2.y)
	else
		error("Attempt to take dot product between " .. type(v1) .. " and " .. type(v2), 2)
	end
end

function vector2.add(v1, v2)
	if isVector2(v1) and isVector2(v2) then
		return vectorType(v1.x + v2.x, v1.y + v2.y)
	else
		error("Attempt to add " .. type(v1) .. " and " .. type(v2), 2)
	end
end

function vector2.subtract(v1, v2)
	if isVector2(v1) and isVector2(v2) then
		return vectorType(v1.x - v2.x, v1.y - v2.y)
	else
		error("Attempt to subtract " .. type(v1) .. " and " .. type(v2), 2)
	end
end

function vector2.multiply(v1, v2)
	if isVector2(v1) then
		if isNumber(v2) then
			return vectorType(v1.x * v2, v1.y * v2)
		elseif isVector2(v2) then
			return vectorType(v1.x * v2.x, v1.y * v2.y)
		end
	elseif isNumber(v1) and isVector2(v2) then
		return vectorType(v1 * v2.x, v1 * v2.y)
	else
		error("Attempt to multiply " .. type(v1) .. " and " .. type(v2), 2)
	end
end

function vector2.divide(v1, v2)
	if isVector2(v1) then
		if isNumber(v2) then
			return vectorType(v1.x / v2, v1.y / v2)
		elseif isVector2(v2) then
			return vectorType(v1.x / v2.x, v1.y / v2.y)
		end
	elseif isNumber(v1) and isVector2(v2) then
		return vectorType(v1 / v2.x, v1 / v2.y)
	else
		error("Attempt to divide " .. type(v1) .. " and " .. type(v2), 2)
	end
end

function vector2.rotate(v, angle)
	if isVector2(v) and isNumber(angle) then
		local s, c = sin(angle), cos(angle)
		return vectorType(v.x * c - v.y * s, v.x * s + v.y * c)
	else
		error("Attempt to rotate " .. type(v) .. " by " .. type(angle), 2)
	end
end

function vector2.flip(v)
	return vectorType(-v.x, -v.y)
end

function vector2.angle(v)
	return atan2(v.y, v.x)
end

function vector2.length(v)
	return sqrt(v.x * v.x + v.y * v.y)
end

function vector2.squareLength(v)
	return v.x * v.x + v.y * v.y
end

function vector2.lerp(v1, v2, value)
	return v1 * (1 - value) + v2 * value
end

function vector2.equals(v1, v2)
	if isVector2(v1) and isVector2(v2) then
		return v1.x == v2.x and v1.y == v2.y
	else
		error("Attempt to compare " .. type(v1) .. " with " .. type(v2), 2)
	end
end

function vector2.nearEquals(v1, v2)
	if isVector2(v1) and isVector2(v2) then
		return abs(v1.x - v2.x) < EPSILON and abs(v1.y - v2.y) < EPSILON
	else
		error("Attempt to compare " .. type(v1) .. " with " .. type(v2), 2)
	end
end

function vector2.normalize(v, length)
	local fact = (length or 1) / sqrt(v.x * v.x + v.y * v.y)
	return vectorType(v.x * fact, v.y * fact)
end

vectorType = ffi.metatype([[
	struct {
		double x, y;
	}
]], {
	__index = vector2,

	__add = metaop "add",
	__sub = metaop "subtract",
	__mul = metaop "multiply",
	__div = metaop "divide",
	__len = metaop "length",
	__unm = metaop "flip",
	__eq  = metaop "equals",

	__tostring = function(v)
		return "(" .. v.x .. ", " .. v.y .. ")"
	end,
})

local vecNew = vector2.new

setmetatable(vector2, {
	__call = function (t, x, y)
		return vecNew(x, y)
	end
})

return vector2
