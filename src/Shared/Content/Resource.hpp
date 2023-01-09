#ifndef SRC_SHARED_CONTENT_RESOURCE_HPP_
#define SRC_SHARED_CONTENT_RESOURCE_HPP_

#include <memory>
#include <string>

namespace res
{

class Resource
{
public:
	Resource(std::string name);
	virtual ~Resource() = default;

	const std::string & getName() const;
	virtual std::size_t getMemoryUsage() const;

private:
	std::string myName;
};

template <typename ResourceType>
using Ptr = std::shared_ptr<ResourceType>;

std::string normalizeResourceName(std::string resourceName);

}

#endif
