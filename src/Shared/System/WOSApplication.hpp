#ifndef WOS_APPLICATION_HPP
#define WOS_APPLICATION_HPP

#include <Client/Game/LocalGame.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <Shared/Utils/Filesystem/FileObserver.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace res
{
class PackageSource;
class SourceAggregator;
class DirectorySource;
}

namespace cfg
{
class ConfigAggregator;
class Config;
class JSONConfig;
}

class WOSApplication
{
public:
	WOSApplication(const std::vector<std::string> & args);
	virtual ~WOSApplication();

	void tick();

	cfg::Config & getConfig() const;
	void saveConfig();
	void updateConfig();

	CallbackHandle<> addConfigCallback(std::function<void()> callback);

	void loadAssets();

	std::shared_ptr<res::SourceAggregator> getResources() const;
	std::shared_ptr<res::SourceAggregator> getBaseResources() const;

	fs::DirectoryObserver::CoalescenceSettings getCoalescenceSettings() const;

	bool isExitRequested() const;

private:
	void initCrashHandler();
	void initSFMLErrorHandler();
	void initConfig();
	void initLogSettings();

	void initDirectoryAssets(int order);
	void initExternalAssets(int order);
	void initPackageAssets(int order);

	std::string getUserConfigFilename() const;

	// Resources
	std::shared_ptr<res::SourceAggregator> resourceAggregator;
	std::shared_ptr<res::SourceAggregator> baseResources;
	std::shared_ptr<res::PackageSource> packageSource;
	std::shared_ptr<res::DirectorySource> assetsDirectorySource;

	// Configs
	std::unique_ptr<cfg::Config> config;
	std::shared_ptr<cfg::ConfigAggregator> configAggregator;
	std::shared_ptr<cfg::JSONConfig> builtInConfig;
	std::shared_ptr<cfg::JSONConfig> userConfig;
	std::shared_ptr<cfg::JSONConfig> argumentConfig;

	std::string customUserConfigFilename;

	// Debug
	fs::FileObserver builtInConfigReloader;
	CallbackManager<> configCallback;
	sf::Clock logFlushClock;

	// Command line arguments
	std::vector<std::string> args;

	Logger logger;
};

#endif
