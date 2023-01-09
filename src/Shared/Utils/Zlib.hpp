#ifndef ZLIB_UTIL_HPP
#define ZLIB_UTIL_HPP

#include <string>
#include <vector>

namespace zu
{

bool compress(std::vector<char> & data, int level = 6);
bool decompress(std::vector<char> & data);

bool compress(std::string & data, int level = 6);
bool decompress(std::string & data);
}

#endif
