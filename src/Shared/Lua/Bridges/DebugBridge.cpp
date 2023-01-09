#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/External/spdlog/fmt/fmt.h>
#include <Shared/Game/AbstractGame.hpp>
#include <Shared/Game/ScriptManager.hpp>
#include <Shared/Lua/Bridges/DebugBridge.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/StringUtils.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <string>

namespace lua
{

cfg::Bool openEditorOnError("wos.game.debug.openEditorOnError");
cfg::String editorCommandLine("wos.game.debug.editorCommandLine");

DebugBridge::DebugBridge(wos::AbstractGame & game, wos::ScriptManager & scriptManager) :
	game(game),
	scriptManager(scriptManager),
	logger("DebugBridge")
{
}

DebugBridge::~DebugBridge()
{
}

void DebugBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("debug.write", std::function<void(std::string)>([=](std::string text) {
		            logger.debug("{}", text);
	            }));

	loader.bind("debug.showError", std::function<void(std::string)>([=](std::string errorText) {
		            game.showError(std::move(errorText));
	            }));

	loader.bind(
	    "debug.openScriptEditor", std::function<void(std::string, int)>([=](std::string resource, int line) {
		    if (!game.getConfig().get(openEditorOnError))
		    {
			    return;
		    }

		    std::string fileName = game.getResources().resolveToFileName(resource);

		    if (!fileName.empty())
		    {
			    using StringUtils::escapeShellString;

			    std::string fileNameArg = escapeShellString(fileName);
			    std::string execString = fmt::format(game.getConfig().get(editorCommandLine), fileNameArg, cNtoS(line));
			    logger.debug("Opening script editor for '{}' with command line '{}'", resource, execString);
			    int retCode = std::system(execString.c_str());
			    if (retCode != 0)
			    {
				    logger.warn("Error code {} returned when opening script editor for '{}'", retCode, resource);
			    }
		    }
		    else
		    {
			    logger.warn("Attempt to open script editor on non-existent script '{}'", resource);
		    }
	    }));

	loader.bind("debug.getLogFileName", std::function<std::string()>(
	                                        [=]() -> std::string
	                                        {
		                                        return Logger::getLogFileName();
	                                        }));
}

}
