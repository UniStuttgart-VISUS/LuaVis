#include <SFML/Config.hpp>
#include <Shared/Lua/Bindings/SerializationBinding.h>
#include <Shared/Lua/Bindings/SerializationBinding.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace
{
std::vector<wosc::Serializer *> boundSerializers;
}

extern "C"
{

	wosC_serial_buffer_t wosC_serial_newBuffer(wosC_serial_t serialID)
	{
		return wosc::Serializer::getByID(serialID).newBuffer();
	}

	void wosC_serial_deleteBuffer(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		wosc::Serializer::getByID(serialID).deleteBuffer(bufferID);
	}

	bool wosC_serial_bufferExists(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).bufferExists(bufferID);
	}

	int32_t wosC_serial_getMaxBufferSize(wosC_serial_t serialID)
	{
		// TODO make max serial buffer size customizable
		return 65536 * 8;
	}

	void wosC_serial_setBufferSize(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t size)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).setSizeInBits(size);
	}

	int32_t wosC_serial_getBufferSize(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).getSizeInBits();
	}

	void wosC_serial_setBufferPosition(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t location)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).seek(location);
	}

	int32_t wosC_serial_getBufferPosition(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).tell();
	}

	bool wosC_serial_isBufferValid(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).isValid();
	}

	bool wosC_serial_isBufferEndReached(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).isAtEnd();
	}

	void wosC_serial_writeInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, double number)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).writeInt(number);
	}

	void wosC_serial_writeUnsignedInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, double number)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).writeUint(number);
	}

	void wosC_serial_writeBoolean(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, bool value)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).writeBool(value);
	}

	void wosC_serial_writeString(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, const char * string,
	                             int32_t length)
	{
		wosc::Serializer::getByID(serialID).getBuffer(bufferID).writeData(string, length);
	}

	double wosC_serial_readInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).readInt();
	}

	double wosC_serial_readUnsignedInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).readUint();
	}

	bool wosC_serial_readBoolean(wosC_serial_t serialID, wosC_serial_buffer_t bufferID)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).readBool();
	}

	const char * wosC_serial_readString(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t length)
	{
		return wosc::Serializer::getByID(serialID).getBuffer(bufferID).readData(length);
	}
}

namespace wosc
{

Serializer::Serializer()
{
	for (wosC_serial_t i = 0; i < (wosC_serial_t) boundSerializers.size(); ++i)
	{
		if (boundSerializers[i] == nullptr)
		{
			boundSerializers[i] = this;
			id = i;
			return;
		}
	}
	boundSerializers.push_back(this);
	id = boundSerializers.size() - 1;
}

Serializer::~Serializer()
{
	boundSerializers[id] = nullptr;
	while (!boundSerializers.empty() && boundSerializers.back() == nullptr)
	{
		boundSerializers.pop_back();
	}
}

Serializer & Serializer::getByID(wosC_serial_t serialID)
{
	if (serialID >= (wosC_serial_t) boundSerializers.size() || boundSerializers[serialID] == nullptr)
	{
		throw Error("Attempt to access invalid serialization context " + cNtoS(serialID));
	}

	return *boundSerializers[serialID];
}

wosC_serial_buffer_t Serializer::newBuffer()
{
	for (wosC_serial_buffer_t i = 0; i < (wosC_serial_buffer_t) buffers.size(); ++i)
	{
		if (!bufferExists(i))
		{
			return i;
		}
	}
	buffers.emplace_back();
	return buffers.size() - 1;
}

void Serializer::deleteBuffer(wosC_serial_buffer_t bufferID)
{
	checkBufferAccess(bufferID);

	buffers[bufferID].setActive(false);
	while (!buffers.empty() && buffers.back().isActive())
	{
		buffers.pop_back();
	}
}

bool Serializer::bufferExists(wosC_serial_buffer_t bufferID) const
{
	return bufferID < (wosC_serial_buffer_t) buffers.size() && buffers[bufferID].isActive();
}

Serializer::Buffer & Serializer::getBuffer(wosC_serial_buffer_t bufferID)
{
	checkBufferAccess(bufferID);

	return buffers[bufferID];
}

const Serializer::Buffer & Serializer::getBuffer(wosC_serial_buffer_t bufferID) const
{
	checkBufferAccess(bufferID);

	return buffers[bufferID];
}

wosC_serial_t Serializer::getID() const
{
	return id;
}


void Serializer::Buffer::setData(std::vector<char> data, std::size_t paddingBits)
{
	this->data = std::move(data);
	this->paddingBits = paddingBits;
	seekPointer = 0;
}

const std::vector<char> & Serializer::Buffer::getData() const
{
	return data;
}

std::size_t Serializer::Buffer::getPaddingBits() const
{
	return paddingBits;
}

void Serializer::Buffer::writeInt(sf::Int64 value)
{
	writeBytes(&value, sizeof(value));
}

void Serializer::Buffer::writeUint(sf::Uint64 value)
{
	writeBytes(&value, sizeof(value));
}

void Serializer::Buffer::writeBool(bool value)
{
	writeBytes(&value, 1);
}

void Serializer::Buffer::writeData(const char * data, std::size_t size)
{
	writeBytes(data, size);
}

sf::Int64 Serializer::Buffer::readInt()
{
	sf::Int64 value = 0;
	readBytes(&value, sizeof(value));
	return value;
}

sf::Uint64 Serializer::Buffer::readUint()
{
	sf::Uint64 value = 0;
	readBytes(&value, sizeof(value));
	return value;
}

bool Serializer::Buffer::readBool()
{
	bool value = false;
	readBytes(&value, 1);
	return value;
}

const char * Serializer::Buffer::readData(std::size_t size)
{
	tempBuf.resize(size);
	readBytes(tempBuf.data(), tempBuf.size());
	return tempBuf.data();
}

bool Serializer::Buffer::isValid() const
{
	return seekPointer <= data.size();
}

bool Serializer::Buffer::isAtEnd() const
{
	return seekPointer == data.size();
}

std::size_t Serializer::Buffer::tell()
{
	return seekPointer * 8;
}

void Serializer::Buffer::seek(std::size_t bits)
{
	if (bits % 8 != 0)
	{
		throw Error("Non-byte seeks not yet implemented");
	}

	seekPointer = bits / 8;
}

void Serializer::Buffer::setSizeInBits(std::size_t sizeInBits)
{
	if (sizeInBits % 8 != 0)
	{
		throw Error("Non-byte sizes not yet implemented");
	}

	data.resize(sizeInBits / 8);
}

std::size_t Serializer::Buffer::getSizeInBits() const
{
	return data.size() * 8;
}

void Serializer::Buffer::setActive(bool active)
{
	this->active = active;
}

bool Serializer::Buffer::isActive() const
{
	return active;
}

void Serializer::Buffer::writeBytes(const void * bytes, std::size_t size)
{
	if (data.size() < seekPointer + size)
	{
		data.resize(seekPointer + size);
	}
	std::memcpy(data.data() + seekPointer, bytes, size);
	seekPointer += size;
}

void Serializer::Buffer::readBytes(void * target, std::size_t size)
{
	if (data.size() >= seekPointer + size)
	{
		std::memcpy(target, data.data() + seekPointer, size);
	}
}

void Serializer::checkBufferAccess(wosC_serial_buffer_t buffer) const
{
	if (!bufferExists(buffer))
	{
		throw Error("Attempt to access invalid buffer with ID " + cNtoS(buffer));
	}
}

}
