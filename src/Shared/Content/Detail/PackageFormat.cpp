#include <Shared/Content/Detail/PackageFormat.hpp>
#include <Shared/Utils/Hash.hpp>
#include <Shared/Utils/Utilities.hpp>

namespace res
{
namespace detail
{
namespace PackageFormat
{

std::string fileNameToHashable(const std::string & filename)
{
	return removeFileExtension(filename);
}

sf::Uint32 nameHash(const std::string & id)
{
	std::string hashableId = fileNameToHashable(id);
	return hash::dataHash32(hashableId.data(), hashableId.size());
}
sf::Uint8 nameHint(const std::string & id)
{
	return nameHash(id) % 256;
}

}
}
}
