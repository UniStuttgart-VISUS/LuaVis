/*
 * BitStream.hpp
 *
 *  Created on: Nov 29, 2015
 *      Author: marukyu
 */

#ifndef SRC_SHARED_UTILS_BITSTREAM_BITSTREAM_HPP_
#define SRC_SHARED_UTILS_BITSTREAM_BITSTREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bits
{

class InputBitStream
{
public:
	template <unsigned int bits, typename ValueType, typename std::enable_if<std::is_integral<ValueType>>::type = 0>
	void write(ValueType value)
	{
		// Truncate value to bit count.
		if (bits < 64)
		{
			value &= (1ULL << bits) - 1;
		}

		writeUnsafe<bits>(value);
	}

	template <typename ValueType, typename std::enable_if<std::is_integral<ValueType>>::type = 0>
	void writePadded(ValueType value)
	{
		// Check if partially written byte needs to be flushed.
		if (bitOffset != 0)
		{
			// Flush partially written byte.
			writeByte(this->partialByte);
			bitOffset = 0;
		}

		// Write full bytes.
		for (unsigned int i = 0; i < sizeof(ValueType); ++i)
		{
			// Write lowest value byte.
			writeByte(static_cast<uint8_t>(value));

			// Downshift value.
			value >>= 8;
		}
	}

	void writePadded(const char * data, std::size_t size)
	{
		// Check if partially written byte needs to be flushed.
		if (bitOffset != 0)
		{
			// Flush partially written byte.
			writeByte(this->partialByte);
			bitOffset = 0;
		}

		// Write full bytes.
		writeBytes(data, size);
	}

private:
	template <unsigned int bits, typename ValueType, typename std::enable_if<std::is_integral<ValueType>>::type = 0>
	void writeUnsafe(ValueType value)
	{
		if (bits > 56)
		{
			// Split value to allow for correct shifting calculations.
			writeImpl<56, ValueType, uint64_t>(static_cast<uint64_t>(value) & ((1ULL << 56) - 1));
			writeImpl<bits - 56, ValueType, uint32_t>(static_cast<uint64_t>(value) >> 56);
		}
		else if (bits > 24)
		{
			// Use 64-bit internal value for shifting calculations.
			writeImpl<bits, ValueType, uint64_t>(value);
		}
		else
		{
			// Use 32-bit internal value for shifting calculations.
			writeImpl<bits, ValueType, uint32_t>(value);
		}
	}

	template <unsigned int bits, typename ValueType, typename InternalValueType>
	void writeImpl(ValueType value)
	{
		// Shift value by current offset.
		InternalValueType valueShifted = static_cast<InternalValueType>(value) << this->bitOffset;

		// Calculate number of byte boundaries to be passed.
		unsigned int byteCount = (bits + this->bitOffset) / 8;

		// Check if any full bytes need to be written out.
		if (byteCount == 0)
		{
			// Value fits within current partial byte (with gap): simply modify the partial byte.
			this->partialByte |= static_cast<uint8_t>(valueShifted);

			// Adjust bit offset.
			this->bitOffset += bits;
		}
		else
		{
			// Combine partial byte with lowest byte of current value and write it out.
			writeByte(this->partialByte | static_cast<uint8_t>(valueShifted));

			// Downshift value.
			valueShifted >>= 8;

			// Write remaining full bytes.
			for (unsigned int i = 0; i < byteCount - 1; ++i)
			{
				// Write lowest value byte.
				writeByte(static_cast<uint8_t>(valueShifted));

				// Downshift value.
				valueShifted >>= 8;
			}

			// Set partial byte.
			this->partialByte = static_cast<uint8_t>(valueShifted);

			// Adjust bit offset if needed.
			if (bits % 8 != 0)
			{
				this->bitOffset += bits;
				this->bitOffset %= 8;
			}
		}
	}

	void writeByte(uint8_t byte)
	{
		writeBytes((const char *) &byte, 1);
	}

	void writeBytes(const char * data, std::size_t size)
	{
		// TODO: Implement.
	}

	unsigned int bitOffset;
	unsigned char partialByte;
};

}

#endif
