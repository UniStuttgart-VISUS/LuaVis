#ifndef SRC_SHARED_UTILS_OWNINGINPUTSTREAM_HPP_
#define SRC_SHARED_UTILS_OWNINGINPUTSTREAM_HPP_

#include <SFML/System/MemoryInputStream.hpp>
#include <vector>

class OwningMemoryStream : public sf::MemoryInputStream
{
public:
	OwningMemoryStream(std::vector<char> data);
	virtual ~OwningMemoryStream() = default;

private:
	std::vector<char> data;
};

#endif
