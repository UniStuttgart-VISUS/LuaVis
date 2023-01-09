local random = {}

local ffi = require "ffi"
local C = ffi.C

local abs = math.abs
local min = math.min
local max = math.max
local floor = math.floor
local bitAnd = bit.band
local bitOr = bit.bor
local bitXor = bit.bxor
local bitRShift = bit.rshift
local bitLShift = bit.lshift
local bitToBit = bit.tobit

local noiseMax = 0x7FFFFFF
local noiseInv = 1 / (noiseMax + 1)
local thresholdGranularity = 2 ^ 24
local thresholdInvGranularity = 1 / thresholdGranularity

-- Seed global random number generator
math.randomseed(require("os").time())

-- Reduces the specified random signed 32-bit integer to a smaller range of integers
function random.limit(value, limit)
	return value % limit
end

local reduceToLimit = random.limit

local function noise3Impl(x, y, z)
	return C.wosC_noise_gen3(x, y, z)
end

local function noise2Impl(x, y)
	-- TODO add binding for 2D noise
	return noise3Impl(x, y, 0x34F687)
end

-- 2D pseudo-random noise function
function random.noise2(x, y, limit)
	return reduceToLimit(noise2Impl(x, y), limit)
end

-- 3D pseudo-random noise function
function random.noise3(x, y, z, limit)
	return reduceToLimit(noise3Impl(x, y, z), limit)
end

local noise2 = random.noise2
local noise3 = random.noise3

-- 2D pseudo-random thresholding function
function random.threshold2(x, y, threshold)
	return floor((noise2(x, y, thresholdGranularity) * thresholdInvGranularity) + 1 - threshold)
end

-- 3D pseudo-random thresholding function
function random.threshold3(x, y, z, threshold)
	return floor((noise3(x, y, z, thresholdGranularity) * thresholdInvGranularity) + 1 - threshold)
end

-- Advances a random number generator's state
function random.rng3Advance(r1, r2, r3)
	return noise2Impl(r2, r3), noise2Impl(r3, r1), noise2Impl(r1, r2)
end

-- Returns a random number based on a generator's state
function random.rng3Get(r1, r2, r3, limit)
	return reduceToLimit(bitXor(r1, r2, r3), limit)
end

return random
