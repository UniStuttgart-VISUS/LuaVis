#include <SFML/System/Err.hpp>
#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/Config/ConfigAggregator.hpp>
#include <Shared/Config/JSONConfig.hpp>
#include <Shared/Content/DirectorySource.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Content/PackageSource.hpp>
#include <Shared/Content/SourceAggregator.hpp>
#include <Shared/Content/SourcePrefixer.hpp>
#include <Shared/System/WOSApplication.hpp>
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Debug/CrashHandler.hpp>
#include <Shared/Utils/Debug/StackTrace.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/Filesystem/LocalStorage.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/MessageWindow.hpp>
#include <Shared/Utils/MiscMath.hpp>
#include <Shared/Utils/VectorMul.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Version.hpp>
#include <algorithm>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace res
{
class DirectorySource;
}

namespace sf
{
class Texture;
}

static const std::string builtInConfigFilename = "config.json";
static const std::string legacyUserConfigFilename = "config.cfg";

struct MountPoint
{
	std::string source;
	std::string target;
};

// TODO extract this class and mount point related code
class MountPointKey
{
public:
	MountPointKey(std::string key) :
		source(key + ".source"),
		target(key + ".target")
	{
	}

	using DataType = MountPoint;

	DataType onGet(const cfg::Config & config) const
	{
		DataType mountPoint;
		mountPoint.source = config.get(source);
		mountPoint.target = config.get(target);
		return mountPoint;
	}

	void onSet(cfg::Config & config, DataType mountPoint) const
	{
		config.set(source, mountPoint.source);
		config.set(target, mountPoint.target);
	}

private:
	cfg::String source;
	cfg::String target;
};

static const cfg::String assetsPath("wos.game.assets.path");
static const cfg::List<cfg::String> assetsPackages("wos.game.assets.packages");
static const cfg::Bool externalAssetsRequired("wos.game.assets.external.required");
static const cfg::List<MountPointKey> externalAssetsMountPoints("wos.game.assets.external.mountPoints");

static const cfg::Int logConsoleVerbosity("wos.game.debug.logging.console.verbosity");
static const cfg::Int logFileVerbosity("wos.game.debug.logging.file.verbosity");
static const cfg::Float logFileFlushInterval("wos.game.debug.logging.file.flushInterval");
static const cfg::String logFileName("wos.game.debug.logging.file.name");

static const cfg::String versionString("wos.game.version");

WOSApplication::WOSApplication(const std::vector<std::string> & args) :
	args(args),
	logger("WOSApplication")
{
	initCrashHandler();
	initSFMLErrorHandler();
	initConfig();
	initLogSettings();

	// Set version in crash handler
	CrashHandler::setMetadata(CrashHandler::MetaKey::Version, getConfig().get(versionString));
}

WOSApplication::~WOSApplication()
{
	saveConfig();
}

void WOSApplication::tick()
{
	if (builtInConfigReloader.poll())
	{
		logger.info("Reloading application configuration...");
		initConfig();
		updateConfig();
		configCallback.fireCallback(1);
	}

	sf::Time flushInterval = sf::seconds(getConfig().get(logFileFlushInterval));
	if (flushInterval >= sf::Time::Zero && logFlushClock.getElapsedTime() > flushInterval)
	{
		logFlushClock.restart();
		Logger::flush();
	}
}

cfg::Config & WOSApplication::getConfig() const
{
	return *config;
}

void WOSApplication::saveConfig()
{
	if (userConfig == nullptr)
	{
		logger.warn("User configuration was not initialized. Settings will not be saved!");
		return;
	}

	fs::LocalStorage::getInstance(fs::LocalStorage::Path::Configuration).createDirectory("", true);

	DataStream stream;
	stream.openOutFile(getUserConfigFilename());
	std::string configContents = userConfig->saveToString(cfg::JSONConfig::Style::Pretty);
	stream.addData(configContents.data(), configContents.size());
}

void WOSApplication::updateConfig()
{
	static cfg::Bool autoReloadAssets("wos.game.assets.autoReload.enabled");

	if (assetsDirectorySource)
	{
		assetsDirectorySource->setPollingEnabled(getConfig().get(autoReloadAssets));
		assetsDirectorySource->setEventCoalescence(getCoalescenceSettings());
	}

	initLogSettings();
}

CallbackHandle<> WOSApplication::addConfigCallback(std::function<void()> callback)
{
	return configCallback.addCallback(callback, 1, 0);
}

void WOSApplication::loadAssets()
{
	baseResources = std::make_shared<res::SourceAggregator>();

	resourceAggregator = std::make_shared<res::SourceAggregator>();
	resourceAggregator->addSource(baseResources, 0);

	initDirectoryAssets(0);
	initExternalAssets(50);
	initPackageAssets(100);

	updateConfig();
}

std::shared_ptr<res::SourceAggregator> WOSApplication::getResources() const
{
	return resourceAggregator;
}

std::shared_ptr<res::SourceAggregator> WOSApplication::getBaseResources() const
{
	return baseResources;
}

fs::DirectoryObserver::CoalescenceSettings WOSApplication::getCoalescenceSettings() const
{
	static cfg::Bool autoReloadCoalescence("wos.game.assets.autoReload.delay.enabled");
	static cfg::Bool autoReloadCoalescenceForceEnable("wos.game.assets.autoReload.delay.force");
	static cfg::Float autoReloadCoalescenceDelay("wos.game.assets.autoReload.delay.interval");
	static cfg::Float autoReloadCoalescenceLimit("wos.game.assets.autoReload.delay.maximum");

	fs::DirectoryObserver::CoalescenceSettings coalescence;
	coalescence.enabled = getConfig().get(autoReloadCoalescence);
	coalescence.force = getConfig().get(autoReloadCoalescenceForceEnable);
	coalescence.delay = sf::seconds(getConfig().get(autoReloadCoalescenceDelay));
	coalescence.limit = sf::seconds(getConfig().get(autoReloadCoalescenceLimit));

	return coalescence;
}

bool WOSApplication::isExitRequested() const
{
	return CrashHandler::isExitRequested();
}

void WOSApplication::initCrashHandler()
{
	CrashHandler::init("ApplicationCrash.log");
}

class SFMLErrorHandler : public std::streambuf
{
public:
	SFMLErrorHandler() :
		logger("SFML")
	{
	}
	virtual ~SFMLErrorHandler() = default;

	virtual int_type overflow(int_type c) override
	{
		if (c == '\n')
		{
			if (errorCount++ < 500)
			{
				// Stop logging after 500 errors
				logger.error("{}", errorText);
			}
			errorText.clear();
		}
		else
		{
			errorText += c;
		}
		return 0;
	}

	Logger logger;
	std::atomic_int errorCount = ATOMIC_VAR_INIT(0);

	static thread_local std::string errorText;
};

thread_local std::string SFMLErrorHandler::errorText {};

void WOSApplication::initSFMLErrorHandler()
{
	sf::err().rdbuf(new SFMLErrorHandler());
}

void WOSApplication::initDirectoryAssets(int order)
{
	if (assetsDirectorySource == nullptr && !config->get(assetsPath).empty())
	{
		assetsDirectorySource = std::make_shared<res::DirectorySource>(config->get(assetsPath));

		if (assetsDirectorySource->isValid())
		{
			baseResources->addSource(assetsDirectorySource, order);
		}
	}
}

void WOSApplication::initExternalAssets(int order)
{
}

void WOSApplication::initPackageAssets(int order)
{
	std::vector<std::string> packagePaths = config->get(assetsPackages);

	for (auto & packagePath : packagePaths)
	{
		auto package = makeUnique<res::Package>();
		if (package->openFile(packagePath))
		{
			packageSource = std::make_shared<res::PackageSource>(std::move(package));
			baseResources->addSource(packageSource, order++);
		}
	}
}

void WOSApplication::initConfig()
{
	if (config == nullptr)
	{
		Logger configLogger("Config");
		config = makeUnique<cfg::Config>();
		config->setDense(true);
		config->setMissingKeyFunction([=](std::string key) {
			return cfg::Value();
		});
	}

	if (configAggregator == nullptr)
	{
		configAggregator = std::make_shared<cfg::ConfigAggregator>();
	}

	if (builtInConfig == nullptr)
	{
		builtInConfigReloader.setTargetFile(builtInConfigFilename);
		builtInConfig = std::make_shared<cfg::JSONConfig>();
		configAggregator->addConfig(builtInConfig, -1);
	}

	if (userConfig == nullptr)
	{
		static const std::string argUserConfigPrefix = "--userconfig=";

		for (const auto & arg : args)
		{
			if (stringStartsWith(arg, argUserConfigPrefix))
			{
				customUserConfigFilename = arg.substr(argUserConfigPrefix.size());
			}
		}

		userConfig = std::make_shared<cfg::JSONConfig>();
		configAggregator->addConfig(userConfig, -2);
		configAggregator->setWritableConfig(userConfig.get());
	}

	if (argumentConfig == nullptr)
	{
		static const std::string argConfigPrefix = "-c";

		argumentConfig = std::make_shared<cfg::JSONConfig>();
		argumentConfig->loadFromString("{}");
		for (const auto & arg : args)
		{
			if (stringStartsWith(arg, argConfigPrefix))
			{
				auto equalsPosition = arg.find_first_of('=', argConfigPrefix.size());
				if (equalsPosition != std::string::npos)
				{
					auto key = arg.substr(argConfigPrefix.size(), equalsPosition - argConfigPrefix.size());
					auto value = arg.substr(equalsPosition + 1);
					argumentConfig->writeValue(key, value);
				}
			}
		}
		configAggregator->addConfig(argumentConfig, -3);
	}

	if (config->getConfigSource() == nullptr)
	{
		config->setConfigSource(configAggregator);
	}

	config->clearCache();

	try
	{
		DataStream configStream;
		if (!configStream.openInFile(builtInConfigFilename))
		{
			throw Error("Failed to open " + builtInConfigFilename);
		}
		builtInConfig->loadFromMemory((const char *) configStream.getData(), configStream.getDataSize());
	}
	catch (std::exception & ex)
	{
		logger.error("Error loading config file '{}': {}", builtInConfigFilename, ex.what());
	}

	try
	{
		std::vector<std::string> configFilenames = {getUserConfigFilename(), legacyUserConfigFilename};

		DataStream configStream;
		for (auto & configFilename : configFilenames)
		{
			if (configStream.openInFile(configFilename))
			{
				userConfig->loadFromMemory((const char *) configStream.getData(), configStream.getDataSize());
				break;
			}
		}
	}
	catch (std::exception & ex)
	{
		// Ignore error. Config doesn't have to exist.
		logger.info("Config file '{}' does not exist; it will be created upon closing", legacyUserConfigFilename);
	}
}

void WOSApplication::initLogSettings()
{
	auto convertVerbosityToLogLevel = [](int verbosity) {
		// Logging level range is inverted for config (0 = silent, 6 = maximum verbosity)
		return Logger::Level(int(Logger::Level::Off)
		                     - clamp<int>(int(Logger::Level::Trace), verbosity, int(Logger::Level::Off)));
	};

	if (Logger::getLogFileName() != config->get(logFileName))
	{
		Logger::setLogFileName(config->get(logFileName));
		logger.debug("Logging to file '{}'", config->get(logFileName));
	}

	auto consoleLevel = convertVerbosityToLogLevel(config->get(logConsoleVerbosity));
	if (Logger::getConsoleLoggingLevel() != consoleLevel)
	{
		Logger::setConsoleLoggingLevel(consoleLevel);
		logger.debug("Console logging level has been set to {}", Logger::getLevelName(consoleLevel));
	}

	auto fileLevel = convertVerbosityToLogLevel(config->get(logFileVerbosity));
	if (Logger::getFileLoggingLevel() != fileLevel)
	{
		Logger::setFileLoggingLevel(fileLevel);
		logger.debug("File logging level has been set to {}", Logger::getLevelName(fileLevel));
	}
}

std::string WOSApplication::getUserConfigFilename() const
{
	if (!customUserConfigFilename.empty())
	{
		return customUserConfigFilename;
	}
	else
	{
		return fs::LocalStorage::getInstance(fs::LocalStorage::Path::Configuration).resolve("userconfig.json");
	}
}
