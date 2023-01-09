local bitUtils = {}

local bitAnd = bit.band
local bitOr = bit.bor
local bitXor = bit.bxor
local bitNot = bit.bnot
local bitLShift = bit.lshift
local bitRShift = bit.rshift
local bitARShift = bit.arshift


local function bitGenRange(offset, size)
	return bitRShift(bitLShift(-1, 32 - size), 32 - offset - size)
end

function bitUtils.setRange(bitset, offset, size, value)
	local mask = bitGenRange(offset, size)
	return bitOr(bitAnd(bitset, bitNot(mask)), bitAnd(bitLShift(value, offset), mask))
end

function bitUtils.getRange(bitset, offset, size)
	return bitRShift(bitAnd(bitset, bitGenRange(offset, size)), offset)
end

local bitSetRange = bitUtils.setRange
local bitGetRange = bitUtils.getRange

local function toTwosComplement(value, size)
	return bitRShift(bitLShift(value, 32 - size), 32 - size)
end

local function fromTwosComplement(value, size)
	return bitOr(bitARShift(bitLShift(value, 32 - size), 32 - size), value)
end

function bitUtils.setSignedRange(bitset, offset, size, value)
	return bitSetRange(bitset, offset, size, toTwosComplement(value, size))
end

function bitUtils.getSignedRange(bitset, offset, size)
	return fromTwosComplement(bitGetRange(bitset, offset, size), size)
end

function bitUtils.set(bitset, offset, boolValue)
	return bitSetRange(bitset, offset, 1, boolValue and 1 or 0)
end

function bitUtils.test(bitset, offset)
	return bitAnd(bitset, bitLShift(1, offset)) ~= 0
end

return bitUtils
