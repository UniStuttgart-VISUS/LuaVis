#include <Shared/Content/Package.hpp>
#include <Shared/Content/PackageSource.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <iterator>

namespace res
{
const std::string PackageSource::PACKAGE_HASH = "$packageHash";

PackageSource::PackageSource() :
	logger("PackageSource")
{
}

PackageSource::PackageSource(std::unique_ptr<Package> package) :
	PackageSource()
{
	setPackage(std::move(package));
}

PackageSource::~PackageSource()
{
}

void PackageSource::setPackage(std::unique_ptr<Package> package)
{
	this->package = std::move(package);

	fileListCache.clear();
	directoryListCache.clear();

	fireEvent(ResourceEvent::MultipleResourcesChanged);
}

Package * PackageSource::getPackage() const
{
	return package.get();
}

bool PackageSource::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	if (!package)
	{
		logger.warn("Attempt to use uninitialized package source");
		return false;
	}

	if (resourceName == PACKAGE_HASH)
	{
		dataTarget = package->getHash();
		return !dataTarget.empty();
	}

	if (!package->select(resourceName))
	{
		return false;
	}

	dataTarget = std::move(package->acquireContent());
	package->deselect();
	return true;
}

bool PackageSource::resourceExists(const std::string & resourceName) const
{
	if (!package)
	{
		logger.warn("Attempt to use uninitialized package source");
		return false;
	}

	if (!package->select(resourceName))
	{
		return false;
	}

	package->deselect();
	return true;
}

std::vector<std::string> PackageSource::getResourceList(std::string prefix, fs::ListFlags flags) const
{
	if (!package)
	{
		logger.warn("Attempt to use uninitialized package source");
		return {};
	}

	if (fileListCache.empty() && package->getContentCount() != 0)
	{
		fileListCache.reserve(package->getContentCount());

		HashSet<std::string> directories;

		for (bool hasMoreContent = package->firstContent(); hasMoreContent; hasMoreContent = package->nextContent())
		{
			std::string name = normalizeResourceName(package->getContentId());
			fileListCache.push_back(name);

			auto pos = name.find_first_of('/');
			while (pos != std::string::npos)
			{
				directories.insert(name.substr(0, pos));
				pos = name.find_first_of('/', pos + 1);
			}
		}

		directoryListCache.assign(directories.begin(), directories.end());

		std::sort(fileListCache.begin(), fileListCache.end());
		std::sort(directoryListCache.begin(), directoryListCache.end());

		package->deselect();
	}

	if (!prefix.empty())
	{
		prefix = normalizeResourceName(prefix + "/");
	}

	std::vector<std::string> resourceList;

	if (package)
	{
		using namespace fs;

		bool recursive = flags & ListRecursive;

		auto addEntriesFromCache = [&](const std::vector<std::string> & cache) {
			for (const auto & resourceName : cache)
			{
				if (stringStartsWith(resourceName, prefix) && resourceName.size() > prefix.size()
				    && (recursive || resourceName.find_first_of('/', prefix.size()) == std::string::npos))
				{
					resourceList.push_back((flags & ListFullPath) ? resourceName : resourceName.substr(prefix.size()));
				}
			}
		};

		if (flags & ListDirectories)
		{
			addEntriesFromCache(directoryListCache);
		}

		if (flags & ListFiles)
		{
			addEntriesFromCache(fileListCache);
		}
	}

	return resourceList;
}

}
