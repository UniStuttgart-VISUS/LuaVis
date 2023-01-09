#ifndef SRC_SHARED_CONTENT_DETAIL_PACKAGEFORMAT_HPP_
#define SRC_SHARED_CONTENT_DETAIL_PACKAGEFORMAT_HPP_

#include <SFML/Config.hpp>
#include <limits>
#include <string>

namespace res
{
namespace detail
{
namespace PackageFormat
{

std::string fileNameToHashable(const std::string & filename);
sf::Uint32 nameHash(const std::string & id);
sf::Uint8 nameHint(const std::string & id);

const std::string headerMagic = "WSP0";
const sf::Uint32 tableBegin = 8;
const sf::Uint32 tableSize = 256;
const sf::Uint32 dataBegin = tableBegin + tableSize * sizeof(sf::Uint32);
const sf::Uint32 maxFileSize = std::numeric_limits<sf::Uint32>::max();

}
}
}

#endif
