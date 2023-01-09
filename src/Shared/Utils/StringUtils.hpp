#ifndef SRC_SHARED_UTILS_STRINGUTILS_HPP_
#define SRC_SHARED_UTILS_STRINGUTILS_HPP_

#include <string>

namespace StringUtils
{

std::string escapeShellString(std::string string);
std::string replace(std::string string, const std::string & searchString, const std::string & replaceString);
std::string trimWhitespace(std::string string);
std::string fromMaxSizeBuffer(const char * buffer, std::size_t maxSize);

};

#endif
