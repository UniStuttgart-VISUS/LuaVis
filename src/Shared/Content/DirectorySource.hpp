#ifndef SRC_SHARED_CONTENT_DIRECTORYSOURCE_HPP_
#define SRC_SHARED_CONTENT_DIRECTORYSOURCE_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>
#include <Shared/Utils/Filesystem/Filesystem.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <string>
#include <tsl/htrie_set.h>
#include <vector>

namespace res
{

class DirectorySource : public AbstractSource
{
public:
	DirectorySource();
	DirectorySource(std::string directory);
	virtual ~DirectorySource();

	void setDirectory(std::string directory);
	const std::string & getDirectory() const;

	bool isValid() const;

	void setPollingEnabled(bool pollingEnabled);
	bool isPollingEnabled() const;

	void setEventCoalescence(fs::DirectoryObserver::CoalescenceSettings coalescence);
	const fs::DirectoryObserver::CoalescenceSettings & getEventCoalescence() const;

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual std::unique_ptr<Stream> openStream(const std::string & resourceName) override;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::string resolveToFileName(const std::string & resourceName) const override;
	virtual void pollChanges() override;

private:
	static res::ResourceEvent::Type convertEventType(fs::DirectoryObserver::Event::Type eventType);

	bool checkValid(const std::string & path) const;
	bool isDisallowedPath(std::string path) const;

	void handleEvent(const fs::DirectoryObserver::Event & event);

	std::string addPathPrefix(std::string filename) const;
	std::string removePathPrefix(std::string filename) const;

	void updateDirectoryObserver();
	void clearDirectoryObserver();

	void updateDirectoryCache(const std::string & path = "");

	std::string directory;
	bool pollingEnabled = false;
	fs::DirectoryObserver directoryObserver;
	tsl::htrie_set<char> directoryCache;

	Logger logger;
};

}

#endif
