#include <SFML/System/InputStream.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Content/SourcePrefixer.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <iterator>

namespace res
{

SourcePrefixer::SourcePrefixer(std::shared_ptr<AbstractSource> source, std::string prefix)
{
	setPrefix(prefix);
	setSource(source);
}

SourcePrefixer::~SourcePrefixer()
{
}

void SourcePrefixer::setSource(std::shared_ptr<AbstractSource> source)
{
	this->source = source;

	if (source)
	{
		callbackHandle = source->addCallback(
		    [this](ResourceEvent event) {
			    fireEvent(ResourceEvent(event.type, prefix + event.resourceName));
		    },
		    ResourceEvent::Any);
	}
	else
	{
		callbackHandle.remove();
	}
}

std::shared_ptr<AbstractSource> SourcePrefixer::getSource() const
{
	return source;
}

void SourcePrefixer::setPrefix(std::string prefix)
{
	this->prefix = prefix.empty() ? "" : normalizeResourceName(prefix + "/");
}

const std::string & SourcePrefixer::getPrefix() const
{
	return prefix;
}

bool SourcePrefixer::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	std::string normalizedResourceName = normalizeResourceName(resourceName);

	if (source && checkPrefix(normalizedResourceName))
	{
		return source->loadResource(removePrefix(normalizedResourceName), dataTarget);
	}
	else
	{
		return false;
	}
}

bool SourcePrefixer::loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget)
{
	std::string normalizedResourceName = normalizeResourceName(resourceName);

	if (source && checkPrefix(normalizedResourceName))
	{
		return source->loadResourceLimited(removePrefix(normalizedResourceName), dataTarget);
	}
	else
	{
		return false;
	}
}

std::unique_ptr<Stream> SourcePrefixer::openStream(const std::string & resourceName)
{
	std::string normalizedResourceName = normalizeResourceName(resourceName);

	if (source && checkPrefix(normalizedResourceName))
	{
		return source->openStream(removePrefix(normalizedResourceName));
	}
	else
	{
		return nullptr;
	}
}

std::vector<std::string> SourcePrefixer::getResourceList(std::string path, fs::ListFlags flags) const
{
	using namespace fs;

	if (source)
	{
		path = normalizeResourceName(path + "/");

		if (checkPrefix(path))
		{
			auto resources = source->getResourceList(removePrefix(path), fs::ListFlags(flags & ~ListFullPath));

			if (flags & ListFullPath)
			{
				for (auto & entry : resources)
				{
					entry = path + entry;
				}
			}

			return resources;
		}
		else if (checkPartialPrefix(path))
		{
			std::vector<std::string> resources;

			std::string subPrefix = (flags & ListFullPath) ? prefix : prefix.substr(path.size());

			if (flags & ListDirectories)
			{
				auto pos = subPrefix.find_first_of('/', (flags & ListFullPath) ? path.size() : 0);
				while (pos != std::string::npos)
				{
					resources.push_back(subPrefix.substr(0, pos));

					if (!(flags & ListRecursive))
					{
						break;
					}

					pos = prefix.find_first_of('/', pos + 1);
				}
			}

			if (flags & ListRecursive)
			{
				for (const auto & entry : source->getResourceList("", flags))
				{
					resources.push_back(subPrefix + entry);
				}
			}

			return resources;
		}
	}

	return {};
}

bool SourcePrefixer::resourceExists(const std::string & resourceName) const
{
	std::string normalizedResourceName = normalizeResourceName(resourceName);

	if (source && checkPrefix(normalizedResourceName))
	{
		return source->resourceExists(removePrefix(normalizedResourceName));
	}
	else
	{
		return false;
	}
}

std::string SourcePrefixer::resolveToFileName(const std::string & resourceName) const
{
	std::string normalizedResourceName = normalizeResourceName(resourceName);

	if (source && checkPrefix(normalizedResourceName))
	{
		return source->resolveToFileName(removePrefix(normalizedResourceName));
	}
	else
	{
		return "";
	}
}

void SourcePrefixer::pollChanges()
{
	if (source)
	{
		source->pollChanges();
	}
}

bool SourcePrefixer::checkPrefix(const std::string & path) const
{
	return prefix.empty() || stringStartsWith(path, prefix);
}

bool SourcePrefixer::checkPartialPrefix(const std::string & path) const
{
	return stringStartsWith(prefix, path);
}

std::string SourcePrefixer::removePrefix(std::string path) const
{
	path.erase(path.begin(), path.begin() + prefix.size());
	return path;
}

}
