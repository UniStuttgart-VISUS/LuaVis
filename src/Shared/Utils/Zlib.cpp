#include <Shared/Utils/Zlib.hpp>
#include <algorithm>
#include <cstddef>
#include <zlib.h>

namespace zu
{

static const std::size_t ZU_HEADER_SIZE = 4;

template <typename Container>
bool compressTransfer(const Container & input, Container & output, int level)
{
	// resize array to have sufficient space for compression.
	uLongf bufferSize = input.size() + input.size() / 10 + 12 + ZU_HEADER_SIZE;
	output.resize(bufferSize);

	// write uncomressed size in network order at the beginning.
	output[0] = input.size() & 0xFF;
	output[1] = (input.size() >> 8) & 0xFF;
	output[2] = (input.size() >> 16) & 0xFF;
	output[3] = (input.size() >> 24) & 0xFF;

	// attempt compression.
	int ret = ::compress2(((Bytef *) &output[0]) + ZU_HEADER_SIZE, &bufferSize, (const Bytef *) input.data(),
	                      (uLong) input.size(), level);

	if (ret != Z_OK)
	{
		return false;
	}

	// resize output to actual size.
	output.resize(bufferSize + ZU_HEADER_SIZE);

	return true;
}

template <typename Container>
bool decompressTransfer(const Container & input, Container & output)
{
	if (input.size() < ZU_HEADER_SIZE)
	{
		return false;
	}

	// input size, without header.
	uLongf inputSize = input.size() - ZU_HEADER_SIZE;
	uLongf bufferSize = static_cast<uLongf>(static_cast<unsigned char>(input[0]))
	                    + (static_cast<unsigned char>(input[1]) << 8) + (static_cast<unsigned char>(input[2]) << 16)
	                    + (static_cast<unsigned char>(input[3]) << 24);

	// remember actual valid data size.
	uLongf outputSize = bufferSize;

	// allocate buffer.
	output.resize(bufferSize);

	int ret =
	    ::uncompress((Bytef *) &output[0], &bufferSize, ((const Bytef *) input.data()) + ZU_HEADER_SIZE, inputSize);

	// error or data size mismatch? abort.
	if (ret != Z_OK || bufferSize != outputSize)
	{
		return false;
	}

	return true;
}

bool compress(std::vector<char> & data, int level)
{
	std::vector<char> destBuf;
	bool ret = compressTransfer(data, destBuf, level);

	if (ret)
	{
		data = std::move(destBuf);
	}

	return ret;
}

bool decompress(std::vector<char> & data)
{
	std::vector<char> destBuf;
	bool ret = decompressTransfer(data, destBuf);

	if (ret)
	{
		data = std::move(destBuf);
	}

	return ret;
}

bool compress(std::string & data, int level)
{
	std::string destBuf;
	bool ret = compressTransfer(data, destBuf, level);

	if (ret)
	{
		data = std::move(destBuf);
	}

	return ret;
}

bool decompress(std::string & data)
{
	std::string destBuf;
	bool ret = decompressTransfer(data, destBuf);

	if (ret)
	{
		data = std::move(destBuf);
	}

	return ret;
}

}
