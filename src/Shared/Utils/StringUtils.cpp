#include <Shared/Utils/StringUtils.hpp>

std::string StringUtils::escapeShellString(std::string string)
{
	return "'" + replace(replace(std::move(string), std::string("\0", 1), ""), "'", "'\\''") + "'";
}

std::string StringUtils::replace(std::string string, const std::string & searchString,
                                 const std::string & replaceString)
{
	std::string::size_type found = string.find(searchString);
	while (found != std::string::npos)
	{
		string.replace(found, searchString.size(), replaceString);
		found = string.find(searchString, found + replaceString.size());
	}
	return string;
}

std::string StringUtils::trimWhitespace(std::string string)
{
	static const std::string whitespace = " \t\n\r";
	std::size_t start = string.find_first_not_of(whitespace);

	if (start == std::string::npos)
	{
		return "";
	}

	std::size_t end = string.find_last_not_of(whitespace);

	return string.substr(start, end - start + 1);
}

std::string StringUtils::fromMaxSizeBuffer(const char * buffer, std::size_t maxSize)
{
	std::size_t size = 0;
	while (size < maxSize && buffer[size] != 0)
	{
		++size;
	}

	return std::string(buffer, buffer + size);
}
