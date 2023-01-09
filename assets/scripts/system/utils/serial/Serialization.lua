local serial = {}

local utils = require "system.utils.Utilities"
local messagePack = require "system.utils.serial.MessagePack"
local serpent = require "system.utils.serial.Serpent"

local messagePackPrefix = string.char(0)

local serpentOptions = {
	sortkeys = true,
	fatal = true,
	nocode = true,
}

serial.Format = {
	MESSAGE_PACK = 1,
	SERPENT = 2,
}

function serial.serialize(data)
	return messagePack.pack(data)
end

function serial.deserialize(data)
	return messagePack.unpack(data)
end

function serial.serializeTagged(data, format, options)
	if format == serial.Format.MESSAGE_PACK then
		return messagePack.packWithPrefix(data, messagePackPrefix)
	elseif format == serial.Format.SERPENT then
		return serpent.serialize(data, options and utils.mergeTables(options, serpentOptions) or serpentOptions)
	else
		error("Unrecognized serialization type '" .. tostring(format) .. "'")
	end
end

function serial.deserializeTagged(data)
	local success, value = messagePack.unpackWithPrefix(data, messagePackPrefix)
	if success then
		return value
	else
		success, value = serpent.load(data)
		if success then
			return value
		else
			error(value)
		end
	end
end

return serial
