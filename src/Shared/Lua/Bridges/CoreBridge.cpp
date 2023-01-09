#include <Shared/Game/AbstractGame.hpp>
#include <Shared/Lua/Bridges/CoreBridge.hpp>
#include <functional>
#include <string>

namespace lua
{

CoreBridge::CoreBridge(wos::AbstractGame & game) :
	game(game),
	logger("CoreBridge")
{
}

CoreBridge::~CoreBridge()
{
}

void CoreBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("core.getCommandLineArguments", //
	    std::function<sol::table(sol::this_state)>(
	        [=](sol::this_state state) -> sol::table
	        {
		        std::size_t index = 0;
		        auto table = sol::state_view(state).create_table();
		        for (const auto & element : game.getCommandLineArguments())
		        {
			        table[index++] = element;
		        }
		        return table;
	        }));

	loader.bind("core.deallocateUnusedResources", //
	    std::function<void()>(
	        [=]()
	        {
		        game.deallocateUnusedResources();
	        }));

	loader.bind("core.reset", //
	    std::function<void()>(
	        [=]()
	        {
		        game.resetLater();
	        }));

	loader.bind("core.exit", std::function<void()>([=]() {
		            game.exit();
	            }));

	loader.bind("core.showError", std::function<void(std::string)>([=](std::string errorText) {
		            game.showError(errorText);
	            }));

	loader.bind("core.log", std::function<void(int, std::string, std::string)>(
	                            [=](int logLevel, std::string loggerName, std::string logText) {
		                            Logger dynamicLogger(loggerName);
		                            dynamicLogger.log((Logger::Level) logLevel, "{}", logText);
	                            }));
}

}
