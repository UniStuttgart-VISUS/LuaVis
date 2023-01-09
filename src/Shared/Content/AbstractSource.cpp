#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/OwningInputStream.hpp>
#include <algorithm>

namespace res
{

AbstractSource::AbstractSource()
{
}

AbstractSource::~AbstractSource()
{
}

bool AbstractSource::loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget)
{
	std::size_t maxSize = dataTarget.size();
	if (loadResource(resourceName, dataTarget))
	{
		if (dataTarget.size() > maxSize)
		{
			dataTarget.resize(maxSize);
		}
		return true;
	}
	else
	{
		dataTarget.clear();
		return false;
	}
}

std::unique_ptr<Stream> AbstractSource::openStream(const std::string & resourceName)
{
	// Default naive implementation: read the whole resource and store it in the stream itself
	std::vector<char> data;
	if (loadResource(resourceName, data))
	{
		return makeUnique<OwningMemoryStream>(std::move(data));
	}
	else
	{
		return nullptr;
	}
}

std::string AbstractSource::resolveToFileName(const std::string & resourceName) const
{
	return "";
}

void AbstractSource::pollChanges()
{
}

CallbackHandle<ResourceEvent> AbstractSource::addCallback(CallbackFunction<ResourceEvent> function,
                                                          ResourceEvent::Type typeFilter, int order)
{
	return callbackManager.addCallback(function, typeFilter, order);
}

void AbstractSource::fireEvent(ResourceEvent event)
{
	callbackManager.fireCallback(event.type, event);
}

}
