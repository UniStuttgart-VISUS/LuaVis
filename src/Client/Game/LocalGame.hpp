#ifndef SRC_CLIENT_GAME_LOCALGAME_HPP_
#define SRC_CLIENT_GAME_LOCALGAME_HPP_

#include <Client/GUI3/Widget.hpp>
#include <Client/Game/InputManager.hpp>
#include <Client/GameRenderer/GraphicsManager.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <Shared/Game/AbstractGame.hpp>
#include <Shared/Game/AsyncPackageCompiler.hpp>
#include <Shared/Game/PerformanceCounter.hpp>
#include <Shared/Game/ResourceLoader.hpp>
#include <Shared/Game/ScriptManager.hpp>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <string>

namespace cfg
{
class Config;
}
namespace res
{
class SourceAggregator;
}

namespace wos
{

class LocalGame : public AbstractGame, public gui3::Widget
{
public:
	LocalGame(res::SourceAggregator & resources);
	virtual ~LocalGame();

	virtual void reset();
	virtual void resetLater() override;

	virtual void cleanUp();

	virtual cfg::Config & getConfig() const override;
	virtual res::AbstractSource & getResources() const override;

	virtual CallbackHandle<> addTickCallback(CallbackFunction<> callback, int order = 0) override;

	void setCommandLineArguments(std::vector<std::string> arguments);
	virtual const std::vector<std::string> & getCommandLineArguments() const override;

	virtual void showError(std::string errorText) override;
	virtual void exit() override;

	ScriptManager & getScriptManager();
	const ScriptManager & getScriptManager() const;

	ResourceLoader & getResourceLoader();
	const ResourceLoader & getResourceLoader() const;

	void finalize();

	virtual void deallocateUnusedResources() override;

private:
	virtual void onRender(sf::RenderTarget & target, sf::RenderStates states) const override;

	void initializeMemoryUsageProviders();

	void tick();

	std::vector<std::string> commandLineArguments;

	wosc::ArrayContext arrayContext;

	ResourceLoader resourceLoader;
	AsyncPackageCompiler packageCompiler;

	GraphicsManager graphics;
	InputManager input;
	ScriptManager scripts;

	mutable PerformanceCounter performance;
	mutable sf::Clock frameClock;

	bool finalized = false;

	Logger logger;
};

}

#endif
