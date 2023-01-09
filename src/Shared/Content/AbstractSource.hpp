#ifndef SRC_SHARED_CONTENT_ABSTRACTSOURCE_HPP_
#define SRC_SHARED_CONTENT_ABSTRACTSOURCE_HPP_

#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <Shared/Utils/Filesystem/Filesystem.hpp>
#include <memory>
#include <string>
#include <vector>

namespace sf
{
class InputStream;
}

namespace res
{
using Stream = sf::InputStream;

class AbstractSource
{
public:
	AbstractSource();
	virtual ~AbstractSource();

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) = 0;
	virtual bool loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget);
	virtual std::unique_ptr<Stream> openStream(const std::string & resourceName);
	virtual bool resourceExists(const std::string & resourceName) const = 0;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const = 0;
	virtual std::string resolveToFileName(const std::string & resourceName) const;

	virtual void pollChanges();

	CallbackHandle<ResourceEvent> addCallback(CallbackFunction<ResourceEvent> function, ResourceEvent::Type typeFilter,
	                                          int order = 0);

	void fireEvent(ResourceEvent event);

private:
	CallbackManager<ResourceEvent> callbackManager;
};

}

#endif
