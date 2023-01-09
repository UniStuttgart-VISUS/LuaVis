#include <Client/GUI3/Events/StateEvent.hpp>
#include <Client/GUI3/Widgets/Graphics/Gradient.hpp>
#include <Client/GUI3/Widgets/Misc/Window.hpp>
#include <Client/GUI3/Widgets/Panels/FillPanel.hpp>
#include <Client/GUI3/Widgets/Panels/Panel.hpp>
#include <Client/System/WOSClient.hpp>
#include <Client/System/WOSInterface.hpp>
#include <Client/System/WOSResourceManager.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Transform.hpp>
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
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Debug/CrashHandler.hpp>
#include <Shared/Utils/Debug/StackTrace.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/MessageWindow.hpp>
#include <Shared/Utils/MiscMath.hpp>
#include <Shared/Utils/VectorMul.hpp>
#include <Version.hpp>
#include <algorithm>
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

static const std::string userConfigFilename = "config.json";
static const std::string runtimeConfigFilename = "config.cfg";

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

WOSClient::WOSClient() :
	logger("WOSClient")
{
}

WOSClient::~WOSClient()
{
	if (app)
	{
		app->saveConfig();
	}
}

const sf::Texture * WOSClient::getTexture(std::size_t pageIndex) const
{
	return resourceManager->getTexture(pageIndex);
}

gui3::res::ResourceManager & WOSClient::getResourceManager() const
{
	return *resourceManager;
}

sf::Vector2f WOSClient::getWhitePixel() const
{
	return whitePixelPosition;
}

cfg::Config & WOSClient::getConfig() const
{
	static cfg::Config nullConfig;
	return app ? app->getConfig() : nullConfig;
}

int WOSClient::init(const std::vector<std::string> & args)
{
	this->args = args;

	try
	{
		initApplication();
		initPlatform();
		initAssets();
		initWhitePixel();
		initWindow();
		updateConfig();
		app->updateConfig();
		return LoadSuccess;
	}
	catch (std::exception & e)
	{
		logger.error("Error during initialization: {}", e.what());
		return LoadErrorGeneric;
	}
}

void WOSClient::initApplication()
{
	app = makeUnique<WOSApplication>(args);

	bind(app->addConfigCallback([this]() {
		updateConfig();
	}));
}

void WOSClient::initAssets()
{
	app->loadAssets();

	if (resourceManager == nullptr)
	{
		resourceManager = makeUnique<WOSResourceManager>();

		static cfg::Int textureAtlasSize("wos.game.graphics.textureAtlasSize");
		resourceManager->setTexturePackerPageSize(getConfig().get(textureAtlasSize));

		resourceManager->setTextureAllocator([](const std::string & name, const sf::Image & image) {
			WOSResourceManager::TextureAllocation allocation;

			// TODO change this hard-coded prefix
			if (image.getSize().x >= 1024 || image.getSize().y >= 1024 || stringStartsWith(name, "virtual/")
			    || stringStartsWith(name, "$framebuffer:"))
			{
				allocation.useSingleTexture = true;
			}

			return allocation;
		});
	}

	resourceManager->setSource(app->getResources());
}

void WOSClient::initPlatform()
{
}

void WOSClient::updateConfig()
{
	static cfg::Float framerate("wos.game.framerate");
	static cfg::Bool textureFiltering("wos.game.graphics.filterTextures");

	setFramerateLimit(getConfig().get(framerate));
	resourceManager->setTextureFilteringEnabled(getConfig().get(textureFiltering));

	game->getResourceLoader().setAutoReloadCoalescence(app->getCoalescenceSettings());
}

void WOSClient::initWhitePixel()
{
	whitePixel = getResourceManager().acquireImage(WOSResourceManager::IMAGE_NAME_WHITE_PIXEL);

	if (whitePixel == nullptr)
	{
		throw std::runtime_error("Failed to allocate white pixel");
	}

	if (whitePixel->getTexturePage() != 0)
	{
		throw std::runtime_error("Failed to place white pixel on primary texture");
	}

	whitePixelPosition =
	    sf::Vector2f(whitePixel->getTextureRect().left + 0.5f, whitePixel->getTextureRect().top + 0.5f);
}

void WOSClient::initWindow()
{
	static cfg::Vector2f gameSizeCfg("wos.game.size");
	static cfg::String iconPathCfg("wos.game.icon");

	sf::Vector2f gameSize = getConfig().get(gameSizeCfg);

	gui3::Interface * interface = open();

	std::string iconPath = getConfig().get(iconPathCfg);
	if (!iconPath.empty())
	{
		std::vector<char> iconData;
		if (app->getResources()->loadResource(iconPath, iconData))
		{
			sf::Image icon;
			if (icon.loadFromMemory(iconData.data(), iconData.size()))
			{
				interface->setIcon(icon);
			}
		}
	}

	auto window = gui3::make<gui3::Window>();
	window->setProportional(true);
	window->setAspectRatio(gameSize.x / gameSize.y);
	window->setBorderless(true);
	window->setMaximized(true);
	interface->getRootContainer().add(window);

	auto fillPanel = gui3::make<gui3::FillPanel>();
	window->add(fillPanel);

	auto gradient = gui3::make<gui3::Gradient>(sf::Color::Black);
	gradient->setZPosition(-1);
	fillPanel->add(gradient);

	game = gui3::make<wos::LocalGame>(*app->getResources());
	game->setCommandLineArguments(this->args);

	fillPanel->add(game);

	game->reset();
	game->stealFocus();

	auto resizeFunc = [=](gui3::StateEvent event) {
		fillPanel->setCustomTransform(sf::Transform().scale(window->getSize() / gameSize));
	};

	resizeFunc(gui3::StateEvent::Resized);

	bind(fillPanel->addStateCallback(resizeFunc, gui3::StateEvent::ParentBoundsChanged, -1));

	bind(interface->getRootContainer().addTickCallback([=]() {
		resourceManager->pollChangeEvents();
		app->tick();

		if (app->isExitRequested())
		{
			logger.info("Termination requested, shutting down...");

			invokeLater([this]() {
				exit();
			});
		}
	}));
}

gui3::Interface * WOSClient::makeInterface()
{
	return new WOSInterface(this);
}

void WOSClient::cleanUpBeforeExit()
{
	game->finalize();

	for (const auto & removeAction : removeActions)
	{
		removeAction();
	}

	if (resourceManager)
	{
		resourceManager->cleanUpBeforeExit();
	}

	app = nullptr;
}
