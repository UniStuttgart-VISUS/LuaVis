#ifndef SRC_SHARED_UTILS_BINARYSTORAGE_HPP_
#define SRC_SHARED_UTILS_BINARYSTORAGE_HPP_

#include <SFML/Config.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

class BinaryStorage
{
public:
	BinaryStorage() :
		myObjectByteCount(1),
		myObjectStringCount(0),
		myObjectCount(0)
	{
	}

	void clear()
	{
		myObjectCount = 0;
		myData = std::vector<char>();
		myStrings = std::vector<std::string>();
	}

	void zero()
	{
		std::memset(myData.data(), 0, myData.size());
	}

	void copy(std::size_t sourceIndex, std::size_t destIndex, std::size_t size = 1)
	{
		copyFrom(*this, sourceIndex, destIndex, size);
	}

	void copyFrom(const BinaryStorage & source, std::size_t sourceIndex, std::size_t destIndex, std::size_t size = 1)
	{
		std::size_t objectByteCount = getObjectByteCount();
		std::size_t objectStringCount = getObjectStringCount();

		if (source.getObjectByteCount() != objectByteCount)
		{
			throw Error("Error copying binary object buffer data: Object byte count mismatch");
		}
		if (source.getObjectStringCount() != objectStringCount)
		{
			throw Error("Error copying binary object buffer data: Object string count mismatch");
		}
		if (sourceIndex + size > source.getObjectCount())
		{
			throw Error("Error copying binary object buffer data: Source out of range");
		}
		if (destIndex + size > getObjectCount())
		{
			throw Error("Error copying binary object buffer data: Destination out of range");
		}
		if (this == &source && sourceIndex + size < destIndex && destIndex + size < sourceIndex)
		{
			throw Error("Error copying binary object buffer data: Source and destination range overlap "
			            "["
			            + cNtoS(sourceIndex) + ";" + cNtoS(sourceIndex + size)
			            + "] "
			              "["
			            + cNtoS(destIndex) + ";" + cNtoS(destIndex + size) + "] ");
		}

		std::memcpy(myData.data() + objectByteCount * destIndex, source.myData.data() + objectByteCount * sourceIndex,
		            objectByteCount * size);
		std::copy(source.myStrings.begin() + objectStringCount * sourceIndex,
		          source.myStrings.begin() + objectStringCount * (sourceIndex + size),
		          myStrings.begin() + objectStringCount * destIndex);
	}

	void setObjectByteCount(std::size_t byteCount)
	{
		if (byteCount == 0)
		{
			byteCount = 1;
		}
		myObjectByteCount = byteCount;
		myData.resize(myObjectCount * byteCount);
	}

	std::size_t getObjectByteCount() const
	{
		return myObjectByteCount;
	}

	void setObjectStringCount(std::size_t stringCount)
	{
		myObjectStringCount = stringCount;
		myStrings.resize(getObjectCount() * stringCount);
	}

	std::size_t getObjectStringCount() const
	{
		return myObjectStringCount;
	}

	void setObjectCount(std::size_t objectCount)
	{
		myObjectCount = objectCount;
		myData.resize(objectCount * myObjectByteCount);
		myStrings.resize(objectCount * myObjectStringCount);
	}

	std::size_t getObjectCount() const
	{
		return myObjectCount;
	}

	void shrinkToFit()
	{
		myData.shrink_to_fit();
		myStrings.shrink_to_fit();
	}

	template <typename T>
	void set(std::size_t index, std::size_t offset, T value)
	{
		std::memcpy(myData.data() + offset + myObjectByteCount * index, &value, sizeof(value));
	}

	template <typename T>
	T get(std::size_t index, std::size_t offset) const
	{
		T value;
		std::memcpy(&value, myData.data() + offset + myObjectByteCount * index, sizeof(value));
		return value;
	}

	char * getDataPointer()
	{
		return myData.data();
	}

	const char * getDataPointer() const
	{
		return myData.data();
	}

	std::size_t getTotalMemoryUsage() const
	{
		return myData.size();
	}

	class Layout
	{
	public:
		std::size_t place(std::size_t size, std::size_t alignment)
		{
			if (size == 0)
			{
				return 0;
			}

			std::size_t index = 0;
			while (!reserveRegion(index, size))
			{
				index += alignment;
			}

			return index;
		}

		std::size_t getBitCount() const
		{
			return myBitset.size();
		}

		std::size_t getByteCount() const
		{
			return (getBitCount() + 7) / 8;
		}

	private:
		void fillRegion(std::size_t index, std::size_t size)
		{
			for (std::size_t i = index; i < index + size; ++i)
			{
				myBitset[i] = true;
			}
		}

		bool reserveRegion(std::size_t index, std::size_t size)
		{
			for (std::size_t i = index; i < index + size; ++i)
			{
				if (i >= myBitset.size())
				{
					myBitset.resize(index + size);
					fillRegion(index, size);
					return true;
				}

				if (myBitset[i])
				{
					return false;
				}
			}

			fillRegion(index, size);
			return true;
		}

		std::vector<bool> myBitset;
	};

private:
	std::size_t myObjectByteCount;
	std::size_t myObjectStringCount;
	std::size_t myObjectCount;

	std::vector<char> myData;
	std::vector<std::string> myStrings;
};

template <>
inline void BinaryStorage::set(std::size_t index, std::size_t offset, std::string value)
{
	myStrings[offset + myObjectStringCount * index] = std::move(value);
}

template <>
inline std::string BinaryStorage::get(std::size_t index, std::size_t offset) const
{
	return myStrings[offset + myObjectStringCount * index];
}

template <>
inline void BinaryStorage::set(std::size_t index, std::size_t offset, bool value)
{
	std::size_t offsetBytes = offset / 8 + myObjectByteCount * index;
	std::size_t offsetBits = offset % 8;

	sf::Uint8 byteValue = static_cast<sf::Uint8>(myData[offsetBytes]);
	byteValue = (byteValue & ~(1 << offsetBits)) | (value << offsetBits);
	myData[offsetBytes] = static_cast<char>(byteValue);
}

template <>
inline bool BinaryStorage::get(std::size_t index, std::size_t offset) const
{
	std::size_t offsetBytes = offset / 8 + myObjectByteCount * index;
	std::size_t offsetBits = offset % 8;

	return ((static_cast<sf::Uint8>(myData[offsetBytes])) >> offsetBits) & 1;
}

#endif
