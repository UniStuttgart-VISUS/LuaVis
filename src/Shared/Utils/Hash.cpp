#include <SFML/System/InputStream.hpp>
#include <Shared/Utils/Hash.hpp>
#include <blake2/blake2.h>
#include <vector>

namespace hash
{

uint32_t dataHash32(const char * data, unsigned int length)
{
	uint32_t hash = 0x9e3779b9;
	for (unsigned int i = 0; i < length; ++i)
	{
		hash += *(data + i) * 31 ^ (length - (i + 1));
		hash = ((hash << 6) ^ (hash >> 2)) + hash;
	}
	return hash;
}

uint64_t dataHash64(const char * data, unsigned int length)
{
	uint64_t hash = 0x79b9128c9e3786b3u;
	for (unsigned int i = 0; i < length; ++i)
	{
		hash += *(data + i) * 63 ^ (length - (i + 1));
		hash = ((hash << 6) ^ (hash >> 2)) + hash;
	}
	return hash;
}

Blake256 computeBlake256(sf::InputStream & input)
{
	input.seek(0);

	Blake256 hashResult;
	hashResult.fill(0);

	blake2s_state hashState;
	blake2s_init(&hashState, BLAKE2S_OUTBYTES);

	sf::Int64 inputSize = input.getSize();
	if (inputSize > 0)
	{
		std::vector<char> buffer(std::min<sf::Int64>(input.getSize(), 65536));

		for (sf::Int64 i = 0; i < inputSize / buffer.size(); ++i)
		{
			if (!input.read(buffer.data(), buffer.size()))
			{
				return hashResult;
			}

			blake2s_update(&hashState, buffer.data(), buffer.size());
		}

		sf::Int64 remainder = inputSize % buffer.size();
		if (remainder != 0)
		{
			if (!input.read(buffer.data(), remainder))
			{
				return hashResult;
			}

			blake2s_update(&hashState, buffer.data(), remainder);
		}
	}

	blake2s_final(&hashState, hashResult.data(), hashResult.size());

	return hashResult;
}

}
