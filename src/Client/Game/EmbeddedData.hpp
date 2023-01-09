#ifndef EMBEDDED_DATA_HPP
#define EMBEDDED_DATA_HPP

#include <string>
#include <vector>

namespace embedded
{
extern std::string fallbackFont;

std::vector<char> hexDecode(const std::string & encodedString);
}

#endif
