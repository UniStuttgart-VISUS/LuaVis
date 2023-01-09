#include "Shared/Utils/NetTypes.hpp"
#include "Shared/Utils/DataStream.hpp"

namespace nt
{

namespace priv
{
// returns the unsigned representation of the specified number.
std::uint64_t mapSigned(std::int64_t num)
{
	if (num < 0)
	{
		// negative numbers are mapped to the odd numbers.
		return std::uint64_t(-num) * 2 - 1;
	}
	else
	{
		// positive numbers and zero are mapped to the even numbers.
		return std::uint64_t(num) * 2;
	}
}

// returns the signed number from its unsigned representation.
std::int64_t unmapSigned(std::uint64_t num)
{
	if (num % 2 == 0)
	{
		// even numbers are mapped back to the positive numbers.
		return std::int64_t(num / 2);
	}
	else
	{
		// odd numbers are mapped back to the negative numbers.
		return -std::int64_t((num - 1) / 2) - 1;
	}
}

// gets the byte length of an unsigned VInt.
std::size_t getVLength(std::uint64_t num)
{
	// length of the VInt in bytes.
	std::size_t length = 0;

	// unconditionally perform first shift. even zero needs 1 byte.
	do
	{
		// divide number by 128, or shift right by 7 bits.
		num >>= 7;

		// increase length by 1 byte.
		length++;
	} while (num != 0);

	// returns the VInt's length.
	return length;
}

// returns the next byte of a VInt.
std::uint8_t nextVByte(std::uint64_t & num)
{
	// get lowest 7 bits of number.
	std::uint8_t byte = num & 0x7F;

	// reduce number.
	num >>= 7;

	// add continuation bit.
	if (num != 0)
	{
		byte |= 0x80;
	}

	return byte;
}

// adds data to a number from its VInt representation. returns true if VInt continues.
bool attachVByte(std::uint64_t & num, std::uint8_t byte, std::size_t byteIndex)
{
	// attach lowest 7 bits of the input byte to the number.
	num |= std::uint64_t(byte & 0x7F) << std::uint64_t(byteIndex * 7);

	// return true if there is a continuation bit.
	return byte & 0x80;
}

// maximum possible number of bytes a 64-bit VInt can have.
std::size_t maxVIntLength = 10;

// adds an unsigned VInt to a data stream.
void appendVInt(DataStream & stream, std::uint64_t num)
{
	std::size_t len = getVLength(num);

	for (std::size_t i = 0; i < len; ++i)
	{
		stream << nextVByte(num);
	}
}

// extracts an unsigned VInt from a data stream.
std::uint64_t extractVInt(DataStream & stream)
{
	std::uint64_t num = 0;

	for (std::size_t i = 0; i < maxVIntLength; ++i)
	{
		std::uint8_t byte;

		stream >> byte;

		if (!attachVByte(num, byte, i))
		{
			break;
		}
	}

	return num;
}
}

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::int64_t> & data)
{
	priv::appendVInt(stream, priv::mapSigned(data.value));
	return stream;
}

DataStream & operator<<(DataStream & stream, const VIntWrapper<std::int64_t> & data)
{
	priv::appendVInt(stream, priv::mapSigned(data.value));
	return stream;
}

DataStream & operator>>(DataStream & stream, const VIntWrapper<std::int64_t> & data)
{
	data.value = priv::unmapSigned(priv::extractVInt(stream));
	return stream;
}

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::uint64_t> & data)
{
	priv::appendVInt(stream, data.value);
	return stream;
}

DataStream & operator<<(DataStream & stream, const VIntWrapper<std::uint64_t> & data)
{
	priv::appendVInt(stream, data.value);
	return stream;
}

DataStream & operator>>(DataStream & stream, const VIntWrapper<std::uint64_t> & data)
{
	data.value = priv::extractVInt(stream);
	return stream;
}

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::int32_t> & data)
{
	priv::appendVInt(stream, priv::mapSigned(data.value));
	return stream;
}

DataStream & operator<<(DataStream & stream, const VIntWrapper<std::int32_t> & data)
{
	priv::appendVInt(stream, priv::mapSigned(data.value));
	return stream;
}

DataStream & operator>>(DataStream & stream, const VIntWrapper<std::int32_t> & data)
{
	data.value = priv::unmapSigned(priv::extractVInt(stream));
	return stream;
}

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::uint32_t> & data)
{
	priv::appendVInt(stream, data.value);
	return stream;
}

DataStream & operator<<(DataStream & stream, const VIntWrapper<std::uint32_t> & data)
{
	priv::appendVInt(stream, data.value);
	return stream;
}

DataStream & operator>>(DataStream & stream, const VIntWrapper<std::uint32_t> & data)
{
	data.value = priv::extractVInt(stream);
	return stream;
}

}
