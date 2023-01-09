#include <Shared/Content/DirectorySource.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>
#include <Shared/Utils/LazyFileInputStream.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <cppfs/FileHandle.h>
#include <iostream>

namespace res
{

DirectorySource::DirectorySource() :
	logger("DirectorySource")
{
	directoryObserver.setRecursive(true);
}

DirectorySource::DirectorySource(std::string directory) :
	DirectorySource()
{
	setDirectory(directory);
}

DirectorySource::~DirectorySource()
{
	clearDirectoryObserver();
}

void DirectorySource::setDirectory(std::string directory)
{
	if (this->directory != directory)
	{
		if (!isDirectory(directory))
		{
			logger.warn("Non-existent directory '{}' was specified as resource path", directory);
		}

		this->directory = directory;
		this->directoryCache.clear();

		updateDirectoryCache();
		updateDirectoryObserver();

		// Fire full update event
		fireEvent(ResourceEvent::MultipleResourcesChanged);
	}
}

const std::string & DirectorySource::getDirectory() const
{
	return directory;
}

void DirectorySource::setPollingEnabled(bool pollingEnabled)
{
	if (this->pollingEnabled != pollingEnabled)
	{
		this->pollingEnabled = pollingEnabled;
		updateDirectoryCache();
		updateDirectoryObserver();
	}
}

bool DirectorySource::isPollingEnabled() const
{
	return pollingEnabled;
}

void DirectorySource::setEventCoalescence(fs::DirectoryObserver::CoalescenceSettings coalescence)
{
	directoryObserver.setEventCoalescence(std::move(coalescence));
}

const fs::DirectoryObserver::CoalescenceSettings & DirectorySource::getEventCoalescence() const
{
	return directoryObserver.getEventCoalescence();
}

bool DirectorySource::loadResource(const std::string & resourceNameRef, std::vector<char> & dataTarget)
{
	std::string resourceName = resourceNameRef;

	if (!checkValid(resourceName) || !isRegularFile(addPathPrefix(resourceName)))
	{
		return false;
	}

	DataStream stream;
	if (!stream.openInFile(addPathPrefix(resourceName)))
	{
		return false;
	}
	return stream.exportToVector(dataTarget);
}

bool DirectorySource::loadResourceLimited(const std::string & resourceNameRef, std::vector<char> & dataTarget)
{
	std::string resourceName = resourceNameRef;

	if (!checkValid(resourceName) || !isRegularFile(addPathPrefix(resourceName)))
	{
		return false;
	}

	DataStream stream;
	if (!stream.openInFile(addPathPrefix(resourceName)))
	{
		return false;
	}
	return stream.exportToVector(dataTarget, dataTarget.size());
}

std::unique_ptr<Stream> DirectorySource::openStream(const std::string & resourceNameRef)
{
	std::string resourceName = resourceNameRef;

	if (!checkValid(resourceName) || !isRegularFile(addPathPrefix(resourceName)))
	{
		return nullptr;
	}

	auto stream = makeUnique<LazyFileInputStream>();
	stream->openLazy(addPathPrefix(resourceName));
	return std::move(stream);
}

bool DirectorySource::resourceExists(const std::string & resourceNameRef) const
{
	std::string resourceName = resourceNameRef;

	if (!checkValid(resourceName))
	{
		return false;
	}

	return isRegularFile(addPathPrefix(resourceName));
}

std::string DirectorySource::resolveToFileName(const std::string & resourceName) const
{
	if (!checkValid(resourceName))
	{
		return "";
	}

	return addPathPrefix(resourceName);
}

void DirectorySource::pollChanges()
{
	if (!isPollingEnabled())
	{
		return;
	}

	directoryObserver.process();

	fs::DirectoryObserver::Event event;
	while (directoryObserver.pollEvent(event))
	{
		handleEvent(event);
	}
}

std::string DirectorySource::addPathPrefix(std::string filename) const
{
	return joinPaths(directory, filename);
}

std::string DirectorySource::removePathPrefix(std::string filename) const
{
	// Normalize name for comparison with path prefix.
	filename = normalizeResourceName(filename);

	// Get directory prefix.
	std::string basePath = normalizeResourceName(directory);

	// Remove asset directory prefix.
	if (stringStartsWith(filename, basePath))
	{
		filename.erase(0, basePath.size());
	}
	else if (basePath != ".")
	{
		logger.warn("Failed to remove prefix '{}' from asset path '{}'", directory, filename);
	}

	// Normalize again to remove possible leading slash.
	return normalizeResourceName(filename);
}

std::vector<std::string> DirectorySource::getResourceList(std::string prefix, fs::ListFlags flags) const
{
	using namespace fs;

	std::vector<std::string> resources;

	if (!checkValid(prefix))
	{
		return resources;
	}

	if (flags & fs::ListFiles)
	{
		listFiles(addPathPrefix(prefix), resources, flags & ListRecursive, flags & ListSorted);
	}

	if (flags & ListDirectories)
	{
		listDirectories(addPathPrefix(prefix), resources, flags & ListRecursive, flags & ListSorted);
	}

	std::for_each(resources.begin(), resources.end(), [=](std::string & name) {
		name = res::normalizeResourceName((flags & ListFullPath) ? joinPaths(prefix, name) : std::move(name));
	});

	return std::move(resources);
}

bool DirectorySource::isDisallowedPath(std::string path) const
{
	path = normalizeResourceName(std::move(path));
	return path == ".." || stringStartsWith(path, "../") || stringEndsWith(path, "/..")
	       || path.find("/../") != std::string::npos || path.find('\0') != std::string::npos;
}

res::ResourceEvent::Type DirectorySource::convertEventType(fs::DirectoryObserver::Event::Type eventType)
{
	switch (eventType)
	{
	case fs::DirectoryObserver::Event::Added:
	case fs::DirectoryObserver::Event::MovedTo:
		return ResourceEvent::ResourceAdded;

	case fs::DirectoryObserver::Event::Removed:
	case fs::DirectoryObserver::Event::MovedFrom:
		return ResourceEvent::ResourceRemoved;

	case fs::DirectoryObserver::Event::Modified:
	case fs::DirectoryObserver::Event::ModifiedCoalesced:
		return ResourceEvent::ResourceChanged;

	default:
		return ResourceEvent::None;
	}
}

bool DirectorySource::checkValid(const std::string & path) const
{
	if (directory.empty())
	{
		logger.warn("Attempt to use uninitialized directory source");
		return false;
	}

	if (isDisallowedPath(path))
	{
		logger.warn("Resource path '{}' is invalid", path);
		return false;
	}

	return true;
}

bool DirectorySource::isValid() const
{
	return isDirectory(directory);
}

void DirectorySource::handleEvent(const fs::DirectoryObserver::Event & event)
{
	bool isDir = event.handle ? event.handle->isDirectory() : isDirectory(event.filename);
	ResourceEvent::Type eventType = convertEventType(event.type);

	// Remove prefix from file name
	std::string filename = removePathPrefix(event.filename);

	// Update directory cache
	if (eventType == ResourceEvent::ResourceAdded && isDir)
	{
		// Additive event on what is now a directory: add directory cache entry
		directoryCache.insert(filename);
	}
	else if (eventType == ResourceEvent::ResourceRemoved)
	{
		// Removal event on what *used* to be a directory: remove from cache and mark event as directory-related
		isDir = directoryCache.erase_prefix(filename);
	}

	if (eventType != ResourceEvent::None)
	{
		// Fire event using fully normalized path name, without directory prefix.
		fireEvent(ResourceEvent(isDir ? ResourceEvent::MultipleResourcesChanged : eventType, filename));
	}
}

void DirectorySource::updateDirectoryObserver()
{
	clearDirectoryObserver();

	if (isPollingEnabled())
	{
		directoryObserver.setWatchedDirectory(directory);
		directoryObserver.startWatching();
	}
}

void DirectorySource::clearDirectoryObserver()
{
	directoryObserver.stopWatching();
}

void DirectorySource::updateDirectoryCache(const std::string & prefix)
{
	if (isPollingEnabled())
	{
		if (prefix.empty())
		{
			directoryCache.clear();
		}
		else
		{
			directoryCache.erase_prefix(prefix);
		}

		std::vector<std::string> directoryList;
		if (listDirectories(addPathPrefix(prefix), directoryList, true))
		{
			directoryCache.insert(prefix);

			for (const auto & subDirectory : directoryList)
			{
				directoryCache.insert(joinPaths(prefix, subDirectory));
			}
		}
	}
	else if (prefix.empty())
	{
		directoryCache.clear();
	}
}

}
