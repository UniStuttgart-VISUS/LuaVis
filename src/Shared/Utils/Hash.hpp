#ifndef HASH_HPP
#define HASH_HPP

#include <array>
#include <cstdint>

namespace sf
{
class InputStream;
}

namespace hash
{

using Blake256 = std::array<char, 32>;

uint32_t dataHash32(const char * data, unsigned int length);
uint64_t dataHash64(const char * data, unsigned int length);

Blake256 computeBlake256(sf::InputStream & input);

}

#endif
