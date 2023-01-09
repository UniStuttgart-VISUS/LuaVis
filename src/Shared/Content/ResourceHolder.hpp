#ifndef SRC_SHARED_CONTENT_RESOURCEHOLDER_HPP_
#define SRC_SHARED_CONTENT_RESOURCEHOLDER_HPP_

#include <Shared/Content/Resource.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <string>
#include <tsl/htrie_map.h>

namespace res
{

template <typename ResourceType>
class ResourceHolder
{
private:
	using Map = tsl::htrie_map<char, Ptr<ResourceType>>;

public:
	using Iterator = typename Map::const_iterator;

	Ptr<ResourceType> get(const std::string & resourceName) const
	{
		// Check if resource is cached.
		auto it = resources.find(resourceName);
		if (it != resources.end())
		{
			// Return cached entry.
			return *it;
		}

		return nullptr;
	}

	void add(const std::string & resourceName, Ptr<ResourceType> resource)
	{
		memoryUsage += resource->getMemoryUsage();
		resources[resourceName] = resource;
	}

	void runGC()
	{
		for (auto it = resources.begin(); it != resources.end();)
		{
			if (it->unique())
			{
				memoryUsage -= (*it)->getMemoryUsage();
				it = resources.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void invalidateResource(const std::string & filename)
	{
		auto it = resources.find(filename);
		if (it != resources.end())
		{
			memoryUsage -= (*it)->getMemoryUsage();
			resources.erase(it);
		}
	}

	void invalidateResources(const std::string & prefix = "")
	{
		if (prefix.empty())
		{
			resources.clear();
			memoryUsage = 0;
			memoryUsageNeedUpdate = false;
		}
		else
		{
			resources.erase(prefix);
			resources.erase_prefix(prefix.back() == '/' ? prefix : prefix + "/");
			memoryUsageNeedUpdate = true;
		}
	}

	Iterator begin() const
	{
		return resources.cbegin();
	}

	Iterator end() const
	{
		return resources.cend();
	}

	std::size_t getTotalMemoryUsage() const
	{
		std::size_t usage = 0;
		for (const auto & resource : resources)
		{
			usage += resource->getMemoryUsage();
		}
		return usage;
		/*
		if (memoryUsageNeedUpdate)
		{
		    memoryUsage = 0;
		    for (const auto & resource : resources)
		    {
		        memoryUsage += resource->getMemoryUsage();
		    }
		    memoryUsageNeedUpdate = false;
		}
		return memoryUsage;
		*/
	}

private:
	Map resources;

	mutable std::size_t memoryUsage = 0;
	mutable bool memoryUsageNeedUpdate = false;
};

}

#endif
