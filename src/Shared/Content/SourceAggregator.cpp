#include <SFML/System/InputStream.hpp>
#include <Shared/Content/SourceAggregator.hpp>
#include <iterator>

namespace res
{

SourceAggregator::SourceAggregator()
{
}

SourceAggregator::~SourceAggregator()
{
}

void SourceAggregator::addSource(std::shared_ptr<AbstractSource> source, int order, std::string label)
{
	if (source != nullptr)
	{
		std::size_t index = findSource(*source);

		if (index >= sourceEntries.size())
		{
			sourceEntries.emplace_back(source, order, label,
			                           source->addCallback(
			                               [=](ResourceEvent event) {
				                               fireEvent(event);
			                               },
			                               ResourceEvent::Any));
			sort();
		}
	}
}

void SourceAggregator::removeSource(AbstractSource & source)
{
	std::size_t index = findSource(source);

	if (index < sourceEntries.size())
	{
		sourceEntries.erase(sourceEntries.begin() + index);
	}
}

bool SourceAggregator::hasSource(AbstractSource & source) const
{
	return findSource(source) < sourceEntries.size();
}

void SourceAggregator::clearSources()
{
	sourceEntries.clear();
}

void SourceAggregator::setSourceOrder(AbstractSource & source, int order)
{
	std::size_t index = findSource(source);

	if (index != sourceEntries.size())
	{
		sourceEntries[index].order = order;
		sort();
	}
}

int SourceAggregator::getSourceOrder(AbstractSource & source) const
{
	std::size_t index = findSource(source);

	if (index != sourceEntries.size())
	{
		return sourceEntries[index].order;
	}

	return 0;
}

void SourceAggregator::setSourceLabel(AbstractSource & source, std::string label)
{
	std::size_t index = findSource(source);

	if (index != sourceEntries.size())
	{
		sourceEntries[index].label = std::move(label);
	}
}

std::string SourceAggregator::getSourceLabel(AbstractSource & source) const
{
	std::size_t index = findSource(source);

	if (index != sourceEntries.size())
	{
		return sourceEntries[index].label;
	}

	return "";
}

std::shared_ptr<AbstractSource> SourceAggregator::lookUpSourceByLabel(const std::string & label) const
{
	for (std::size_t i = 0; i < sourceEntries.size(); ++i)
	{
		if (sourceEntries[i].label == label)
		{
			return sourceEntries[i].source;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<AbstractSource>> SourceAggregator::getSources() const
{
	std::vector<std::shared_ptr<AbstractSource>> sources;
	for (const auto & source : sourceEntries)
	{
		sources.push_back(source.source);
	}
	return sources;
}

bool SourceAggregator::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	for (const auto & source : sourceEntries)
	{
		if (source.source->loadResource(resourceName, dataTarget))
		{
			return true;
		}
	}

	return false;
}

bool SourceAggregator::loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget)
{
	for (const auto & source : sourceEntries)
	{
		if (source.source->loadResourceLimited(resourceName, dataTarget))
		{
			return true;
		}
	}

	return false;
}

std::unique_ptr<Stream> SourceAggregator::openStream(const std::string & resourceName)
{
	for (const auto & source : sourceEntries)
	{
		if (auto stream = source.source->openStream(resourceName))
		{
			return stream;
		}
	}

	return nullptr;
}

bool SourceAggregator::resourceExists(const std::string & resourceName) const
{
	for (const auto & source : sourceEntries)
	{
		if (source.source->resourceExists(resourceName))
		{
			return true;
		}
	}

	return false;
}

std::vector<std::string> SourceAggregator::getResourceList(std::string prefix, fs::ListFlags flags) const
{
	std::vector<std::string> result;

	for (const auto & source : sourceEntries)
	{
		std::vector<std::string> resources = source.source->getResourceList(prefix, flags);
		result.insert(result.end(), resources.begin(), resources.end());
	}

	std::sort(result.begin(), result.end());
	result.erase(std::unique(result.begin(), result.end()), result.end());

	return result;
}

std::string SourceAggregator::resolveToFileName(const std::string & resourceName) const
{
	for (const auto & source : sourceEntries)
	{
		std::string fileName = source.source->resolveToFileName(resourceName);
		if (!fileName.empty())
		{
			return fileName;
		}
	}

	return "";
}

void SourceAggregator::pollChanges()
{
	for (const auto & source : sourceEntries)
	{
		source.source->pollChanges();
	}
}

std::size_t SourceAggregator::findSource(AbstractSource & source) const
{
	for (std::size_t i = 0; i < sourceEntries.size(); ++i)
	{
		if (sourceEntries[i].source.get() == &source)
		{
			return i;
		}
	}
	return sourceEntries.size();
}

void SourceAggregator::sort()
{
	std::stable_sort(sourceEntries.begin(), sourceEntries.end(), [](const SourceEntry & a, const SourceEntry & b) {
		return a.order < b.order;
	});
}

}
