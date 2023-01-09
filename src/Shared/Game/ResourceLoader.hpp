#ifndef SRC_SHARED_GAME_RESOURCELOADER_HPP_
#define SRC_SHARED_GAME_RESOURCELOADER_HPP_

#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>
#include <Sol2/sol.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace wosc
{
class ArrayContext;
}

namespace res
{
class AbstractSource;
class SourceAggregator;
}

namespace wos
{

class ResourceLoader
{
public:
	ResourceLoader(res::SourceAggregator & aggregator);
	virtual ~ResourceLoader();

	ResourceLoader(const ResourceLoader &) = delete;
	ResourceLoader(ResourceLoader &&) = delete;
	ResourceLoader & operator=(const ResourceLoader &) = delete;
	ResourceLoader & operator=(ResourceLoader &&) = delete;

	res::AbstractSource & getCombinedSource() const;

	void removeAllSources();
	void removeOwnedSources();

	void setArrayContext(wosc::ArrayContext * arrayContext);
	wosc::ArrayContext * getArrayContext() const;

	void setExternalAssetPath(std::string externalAssetPath);
	const std::string & getExternalAssetPath() const;

	void addDirectorySource(std::string label, std::string mountPoint, int order, std::string path, bool external,
	                        bool autoReload);
	void addPackageSource(std::string label, std::string mountPoint, int order, std::string path);
	void addZipSource(std::string label, std::string mountPoint, int order, std::string path);
	void addLuaFunctionSource(std::string label, std::string mountPoint, int order, sol::function function);

	void removeSource(const std::string & label);
	bool hasSource(const std::string & label) const;

	void setSourceOrder(const std::string & label, int order);
	int getSourceOrder(const std::string & label) const;

	void setAutoReloadCoalescence(fs::DirectoryObserver::CoalescenceSettings autoReloadCoalescence);
	fs::DirectoryObserver::CoalescenceSettings getAutoReloadCoalescence() const;

private:
	void addSource(std::shared_ptr<res::AbstractSource> source, int order, std::string label);

	res::SourceAggregator & aggregator;
	wosc::ArrayContext * arrayContext = nullptr;
	std::string externalAssetPath;
	std::unordered_map<std::string, std::shared_ptr<res::AbstractSource>> sources;
	fs::DirectoryObserver::CoalescenceSettings autoReloadCoalescence;

	Logger logger;
};

}

#endif
