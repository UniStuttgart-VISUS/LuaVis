#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/Events/Key.hpp>
#include <Client/GUI3/Events/KeyEvent.hpp>
#include <Client/GUI3/Events/StateEvent.hpp>
#include <Client/Game/LocalGame.hpp>
#include <Client/Lua/Bridges/GraphicsBridge.hpp>
#include <Client/Lua/Bridges/InputBridge.hpp>
#include <Client/Lua/Bridges/WindowBridge.hpp>
#include <Client/System/WOSResourceManager.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/Content/SourceAggregator.hpp>
#include <Shared/Lua/Bridges/ArrayBridge.hpp>
#include <Shared/Lua/Bridges/ConfigBridge.hpp>
#include <Shared/Lua/Bridges/CoreBridge.hpp>
#include <Shared/Lua/Bridges/DebugBridge.hpp>
#include <Shared/Lua/Bridges/PerformanceBridge.hpp>
#include <Shared/Lua/Bridges/ResourceBridge.hpp>
#include <Shared/Lua/Bridges/ScriptBridge.hpp>
#include <Shared/Lua/Bridges/UtilityBridge.hpp>

#include <Version.hpp>

#include <exception>
#include <memory>

static cfg::Bool debugResetEnabled("wos.game.debug.reset.enabled");
static cfg::String debugResetKey("wos.game.debug.reset.key");

namespace wos
{

LocalGame::LocalGame(res::SourceAggregator & resources) :
	resourceLoader(resources),
	graphics(*this),
	input(*this),
	scripts(*this),
	logger("LocalGame")
{
	setComplexOverride(true);

	graphics.setArrayContext(&arrayContext);

	resourceLoader.setArrayContext(&arrayContext);

	addKeyboardCallback(
	    [=](gui3::KeyEvent event) {
		    if (config().get(debugResetEnabled) && event.key == gui3::Key::fromString(config().get(debugResetKey)))
		    {
			    reset();
		    }
	    },
	    gui3::KeyEvent::Press);

	addTickCallback([=]() {
		tick();
	});

	addStateCallback(
	    [=](gui3::StateEvent event) {
		    finalize();
	    },
	    gui3::StateEvent::WindowClosed);

	// clang-format off
	std::vector<std::shared_ptr<lua::AbstractBridge>> bridges {
		std::make_shared<lua::CoreBridge>(*this),
		std::make_shared<lua::ScriptBridge>(scripts),
		std::make_shared<lua::ConfigBridge>(*this),
		std::make_shared<lua::InputBridge>(input),
		std::make_shared<lua::GraphicsBridge>(graphics),
		std::make_shared<lua::WindowBridge>(*this),
		std::make_shared<lua::ResourceBridge>(resourceLoader, packageCompiler, getThreadPool()),
		std::make_shared<lua::ArrayBridge>(arrayContext),
		std::make_shared<lua::UtilityBridge>(),
		std::make_shared<lua::PerformanceBridge>(performance),
		std::make_shared<lua::DebugBridge>(*this, scripts)
	};
	// clang-format on


	scripts.setBridges(std::move(bridges));
}

LocalGame::~LocalGame()
{
	finalize();
}

void LocalGame::reset()
{
	cleanUp();
	initializeMemoryUsageProviders();

	scripts.init();
}

void LocalGame::resetLater()
{
	getParentApplication()->invokeLater(
	    [=]()
	    {
		    this->reset();
	    });
}

void LocalGame::cleanUp()
{
	try
	{
		resourceLoader.removeOwnedSources();
		packageCompiler.clear();
		if (scripts.hasEventFunction())
		{
			scripts.callEventFunction(ScriptManager::Event::EXIT);
		}
	}
	catch (std::exception & e)
	{
		logger.error("Error during clean-up: {}", e.what());
	}

	graphics.reset();

	arrayContext.clear();
}

cfg::Config & LocalGame::getConfig() const
{
	return config();
}

res::AbstractSource & LocalGame::getResources() const
{
	return resourceLoader.getCombinedSource();
}

CallbackHandle<> LocalGame::addTickCallback(CallbackFunction<> callback, int order)
{
	return gui3::Widget::addTickCallback(callback, order);
}

void LocalGame::setCommandLineArguments(std::vector<std::string> arguments)
{
	commandLineArguments = std::move(arguments);
}

const std::vector<std::string> & LocalGame::getCommandLineArguments() const
{
	return commandLineArguments;
}

void LocalGame::showError(std::string errorText)
{
	// TODO show error visually
	logger.error("{}", errorText);
}

void LocalGame::exit()
{
	getParentApplication()->invokeLater([=]() {
		getParentApplication()->exit();
	});
}

ScriptManager & LocalGame::getScriptManager()
{
	return scripts;
}

const ScriptManager & LocalGame::getScriptManager() const
{
	return scripts;
}

ResourceLoader & LocalGame::getResourceLoader()
{
	return resourceLoader;
}

const ResourceLoader & LocalGame::getResourceLoader() const
{
	return resourceLoader;
}

void LocalGame::finalize()
{
	if (!finalized)
	{
		cleanUp();
		finalized = true;
	}
}

void LocalGame::deallocateUnusedResources()
{
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&getParentApplication()->getResourceManager()))
	{
		resourceManager->collectDataGarbage();
	}
}

void LocalGame::onRender(sf::RenderTarget & target, sf::RenderStates states) const
{
	sf::Clock renderClock;

	target.draw(graphics, states);

	performance.setRenderTime(renderClock.getElapsedTime());
}

void LocalGame::initializeMemoryUsageProviders()
{
	// TODO possibly move this to another class
	auto registerSimpleMemoryUsageProvider = [this](std::string name, auto provider) {
		performance.registerMemoryUsageProvider(
		    name, wos::PerformanceCounter::MemoryUsageProvider([this, provider](std::size_t entryCount) {
			    // TODO support entryCount != 0 for per-item breakdown
			    std::vector<PerformanceCounter::MemoryUsageEntry> result;
			    PerformanceCounter::MemoryUsageEntry entry;
			    entry.memoryUsage = provider();
			    result.push_back(entry);
			    return result;
		    }));
	};

	registerSimpleMemoryUsageProvider("Arrays", [this]() {
		return arrayContext.getTotalMemoryUsage();
	});

	// TODO move memory usage functions to base resource manager class
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&getParentApplication()->getResourceManager()))
	{
		registerSimpleMemoryUsageProvider("Textures", [resourceManager]() {
			return resourceManager->getImageMemoryUsage();
		});

		registerSimpleMemoryUsageProvider("Data", [resourceManager]() {
			return resourceManager->getDataMemoryUsage();
		});
	}
}

void LocalGame::tick()
{
	auto * parentApp = getParentApplication();
	int fpsLimit = parentApp ? parentApp->getFramerateLimit() : 60;
	performance.setTargetTime(fpsLimit == 0 ? sf::Time::Zero : sf::microseconds(1000000 / fpsLimit));
	performance.setSleepTime(parentApp ? parentApp->getLastSleepTime() : sf::Time::Zero);
	performance.setTotalFrameTime(parentApp ? parentApp->getLastFrameTime() : sf::Time::Zero);
	performance.tick();

	if (!scripts.getError().empty())
	{
		return;
	}

	try
	{
		sf::Clock tickClock;
		scripts.callEventFunction(ScriptManager::Event::TICK);
		performance.setTickTime(tickClock.getElapsedTime());
	}
	catch (std::exception & e)
	{
		scripts.setError(e.what());
	}
}

}
