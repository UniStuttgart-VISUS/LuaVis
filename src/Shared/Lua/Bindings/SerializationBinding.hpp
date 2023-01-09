#ifndef SRC_SHARED_LUA_BINDINGS_SERIALIZATIONBINDING_HPP_
#define SRC_SHARED_LUA_BINDINGS_SERIALIZATIONBINDING_HPP_

#include <SFML/Config.hpp>
#include <Shared/Lua/Bindings/SerializationBinding.h>
#include <cstddef>
#include <vector>

namespace wosc
{

class Serializer
{
public:
	class Buffer
	{
	public:
		void setData(std::vector<char> data, std::size_t paddingBits);

		const std::vector<char> & getData() const;
		std::size_t getPaddingBits() const;

		void writeInt(sf::Int64 value);
		void writeUint(sf::Uint64 value);
		void writeBool(bool value);
		void writeData(const char * data, std::size_t size);

		sf::Int64 readInt();
		sf::Uint64 readUint();
		bool readBool();
		const char * readData(std::size_t size);

		bool isValid() const;
		bool isAtEnd() const;

		std::size_t tell();
		void seek(std::size_t bits);

		void setSizeInBits(std::size_t sizeInBits);
		std::size_t getSizeInBits() const;

		void setActive(bool active);
		bool isActive() const;

	private:
		void writeBytes(const void * bytes, std::size_t size);
		void readBytes(void * target, std::size_t size);

		std::vector<char> data;
		std::vector<char> tempBuf;
		std::size_t paddingBits = 0;
		std::size_t seekPointer = 0;
		bool active = true;
	};

	Serializer();
	~Serializer();

	static Serializer & getByID(wosC_serial_t serialID);

	Serializer(const Serializer &) = delete;
	Serializer(Serializer &&) = delete;
	Serializer & operator=(const Serializer &) = delete;
	Serializer & operator=(Serializer &&) = delete;

	wosC_serial_buffer_t newBuffer();
	void deleteBuffer(wosC_serial_buffer_t bufferID);
	bool bufferExists(wosC_serial_buffer_t bufferID) const;
	Buffer & getBuffer(wosC_serial_buffer_t bufferID);
	const Buffer & getBuffer(wosC_serial_buffer_t bufferID) const;

	wosC_serial_t getID() const;

private:
	void checkBufferAccess(wosC_serial_buffer_t bufferID) const;

	wosC_serial_t id;

	std::vector<Buffer> buffers;
};

}

#endif
