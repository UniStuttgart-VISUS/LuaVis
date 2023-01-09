local color = {}

local utils = require "system.utils.Utilities"
local bitUtils = require "system.utils.BitUtilities"

local bitLShift = bit.lshift
local bitRShift = bit.rshift
local bitSetRange = bitUtils.setRange
local bitOr = bit.bor
local bitAnd = bit.band

local type = type

local abs = math.abs
local min = math.min

local lerp = utils.lerp
local clamp = utils.clamp
local step = utils.step


function color.rgba(r, g, b, a)
	return bitOr(bitAnd(r, 255),
		bitLShift(bitAnd(g, 255), 8),
		bitLShift(bitAnd(b, 255), 16),
		bitLShift(bitAnd(a, 255), 24))
end

local rgba = color.rgba


color.TRANSPARENT = rgba(0, 0, 0, 0)
color.BLACK       = rgba(0, 0, 0, 255)
color.WHITE       = rgba(255, 255, 255, 255)

color.RED         = rgba(255, 0, 0, 255)
color.GREEN       = rgba(0, 255, 0, 255)
color.BLUE        = rgba(0, 0, 255, 255)

color.CYAN        = rgba(0, 255, 255, 255)
color.MAGENTA     = rgba(255, 0, 255, 255)
color.YELLOW      = rgba(255, 255, 0, 255)


function color.rgb(r, g, b)
	return rgba(r, g, b, 255)
end

function color.getR(c)
	return bitAnd(c, 255)
end

function color.getG(c)
	return bitAnd(bitRShift(c, 8), 255)
end

function color.getB(c)
	return bitAnd(bitRShift(c, 16), 255)
end

function color.getA(c)
	return bitAnd(bitRShift(c, 24), 255)
end

function color.setR(c, r)
	return bitSetRange(c, 0, 8, r)
end

function color.setG(c, g)
	return bitSetRange(c, 8, 8, g)
end

function color.setB(c, b)
	return bitSetRange(c, 16, 8, b)
end

function color.setA(c, a)
	return bitSetRange(c, 24, 8, a)
end

function color.fade(c, factor)
	return color.setA(c, clamp(0, color.getA(c) * factor, 255))
end

function color.getRGBA(c)
	return bitAnd(c, 255),
		bitAnd(bitRShift(c, 8), 255),
		bitAnd(bitRShift(c, 16), 255),
		bitAnd(bitRShift(c, 24), 255)
end

local getRGBA = color.getRGBA

function color.getRGBANormalized(c)
	return bitAnd(c, 255) / 255,
		bitAnd(bitRShift(c, 8), 255) / 255,
		bitAnd(bitRShift(c, 16), 255) / 255,
		bitAnd(bitRShift(c, 24), 255) / 255
end

local getRGBANormalized = color.getRGBANormalized

function color.getTable(c)
	return {
		bitAnd(c, 255),
		bitAnd(bitRShift(c, 8), 255),
		bitAnd(bitRShift(c, 16), 255),
		bitAnd(bitRShift(c, 24), 255)
	}
end

local function computeHSVComponent(h, s, v)
	return v * lerp(1, clamp(0, abs((h % 1) * 6 - 3) - 1, 1), s) * 255
end

function color.hsv(h, s, v, a)
	return rgba(
		computeHSVComponent(h + 1, s, v),
		computeHSVComponent(h + 2/3, s, v),
		computeHSVComponent(h + 1/3, s, v),
	(a or 1) * 255)
end

function color.toHSV(c)
	local r, g, b, a = getRGBANormalized(c)

	local t0 = step(b, g)
	local t1 = lerp(b, g, t0)
	local t2 = lerp(g, b, t0)
	local t3 = step(t1, r)
	local t4 = lerp(t1, r, t3)
	local t5 = lerp(r, t1, t3)
	local t6 = t4 - min(t5, t2)

	if t6 == 0 or t4 == 0 then
		return 0, 0, t4, a
	else
		return abs(2/3 - t0 - t3 * 5/3 + 2 * t0 * t3 + (t5 - t2) / (6 * t6)), t6 / t4, t4, a
	end
end

function color.fromTable(table)
	if type(table) == "table" then
		return rgba(table[1], table[2], table[3], table[4] or 255)
	else
		return table
	end
end

return color
