#include <Shared/Content/Resource.hpp>
#include <algorithm>
#include <iterator>

namespace res
{

Resource::Resource(std::string name) :
	myName(std::move(name))
{
}

const std::string & Resource::getName() const
{
	return myName;
}

std::size_t Resource::getMemoryUsage() const
{
	return 0;
}

std::string normalizeResourceName(std::string resourceName)
{
	// Keep empty or special resource names untouched.
	if (resourceName.empty() || resourceName[0] == '$')
	{
		return std::move(resourceName);
	}

	// Convert slashes to unix-style.
	std::replace(resourceName.begin(), resourceName.end(), '\\', '/');

	// Merge slashes.
	resourceName.erase(std::unique(resourceName.begin(), resourceName.end(),
	                               [](char a, char b) {
		                               return a == '/' && b == '/';
	                               }),
	                   resourceName.end());

	// Remove leading slash.
	if (!resourceName.empty() && resourceName[0] == '/')
	{
		resourceName.erase(resourceName.begin());
	}

	return std::move(resourceName);
}

}
