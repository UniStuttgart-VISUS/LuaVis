#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Game/AbstractGame.hpp>
#include <Shared/Game/ScriptManager.hpp>
#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <cstddef>

namespace wos
{

static cfg::String confScriptPath("wos.game.assets.scripts.path");
static cfg::String confScriptExtension("wos.game.assets.scripts.extension");
static cfg::String confScriptInit("wos.game.assets.scripts.init");
static cfg::String confScriptFFIAPIPath("wos.game.assets.scripts.ffiApiPath");

ScriptManager::ScriptManager(wos::AbstractGame & game) :
	game(game),
	logger("ScriptManager")
{
}

ScriptManager::~ScriptManager()
{
}

void ScriptManager::init()
{
	resetError();
	resetState();
	initLibraries();
	initErrorHandler();
	loadInitScript();
}

void ScriptManager::resetState()
{
	unsetEventFunction();
	lua.reset();
	luaStateInitialized = false;
}

void ScriptManager::initLibraries()
{
	for (const auto & bridge : bridges)
	{
		bridge->load(lua);
	}
	lua.loadBaseLibraries();
}

void ScriptManager::initErrorHandler()
{
	sol::protected_function::set_default_handler(lua.getGlobal("debug.traceback"));
}

void ScriptManager::loadInitScript()
{
	try
	{
		std::string initScript = config().get(confScriptInit);
		sol::function init = loadScript(initScript);
		sol::protected_function_result result = init();

		if (!result.valid())
		{
			sol::error error = result;
			luaStateInitialized = false;
			logger.error("Error during initialization:\n\n{}", error.what());
		}
		else
		{
			luaStateInitialized = result;
			if (!luaStateInitialized)
			{
				logger.error("Error during initialization:\n\n{}", getError());
			}
			else
			{
				logger.debug("Initialization script executed successfully.");
			}
		}
	}
	catch (std::exception & e)
	{
		logger.error("Error during initialization:\n\n{}", e.what());
	}
}

void ScriptManager::setBridges(std::vector<std::shared_ptr<lua::AbstractBridge>> bridges)
{
	this->bridges = bridges;
}

const std::vector<std::shared_ptr<lua::AbstractBridge>> & ScriptManager::getBridges() const
{
	return bridges;
}

sol::function ScriptManager::loadScript(std::string script)
{
	if (script.find('/') != std::string::npos)
	{
		logger.warn("'/' in module name: {}", script);
	}

	script = res::normalizeResourceName(std::move(script));

	logger.debug("Loading script '{}'...", script);

	std::vector<char> scriptData;
	if (!resources().loadResource(scriptNameToFileName(script), scriptData))
	{
		throw Error("Failed to load script '" + script + "'");
	}
	else
	{
		// TODO extract the unprefixed scriptName->scriptPath conversion function somewhere else
		std::replace(script.begin(), script.end(), '.', '/');
		return lua.loadScript(scriptData.data(), scriptData.size(), script + config().get(confScriptExtension));
	}
}

bool ScriptManager::scriptExists(const std::string & scriptName) const
{
	return resources().resourceExists(scriptNameToFileName(scriptName));
}

std::vector<std::string> ScriptManager::getAllScripts() const
{
	auto scripts = resources().getResourceList(config().get(confScriptPath),
	                                           fs::ListFiles | fs::ListRecursive | fs::ListSorted | fs::ListFullPath);

	scripts.erase(std::remove_if(scripts.begin(), scripts.end(),
	                             [=](const std::string & script) {
		                             return !isScriptFile(script);
	                             }),
	              scripts.end());

	for (auto & script : scripts)
	{
		script = fileNameToScriptName(std::move(script));
	}

	return scripts;
}

bool ScriptManager::isScriptFile(const std::string & fileName) const
{
	return stringStartsWith(fileName, config().get(confScriptPath) + "/")
	       && stringEndsWith(fileName, config().get(confScriptExtension));
}

std::string ScriptManager::scriptNameToFileName(std::string scriptName) const
{
	std::replace(scriptName.begin(), scriptName.end(), '.', '/');

	return config().get(confScriptPath) + "/" + res::normalizeResourceName(std::move(scriptName))
	       + config().get(confScriptExtension);
}

std::string ScriptManager::fileNameToScriptName(std::string fileName) const
{
	fileName = res::normalizeResourceName(fileName);

	if (isScriptFile(fileName))
	{
		std::size_t prefixSize = config().get(confScriptPath).size() + 1;
		std::size_t suffixSize = config().get(confScriptExtension).size();
		fileName = fileName.substr(prefixSize, fileName.size() - prefixSize - suffixSize);
	}

	std::replace(fileName.begin(), fileName.end(), '/', '.');

	return fileName;
}

void ScriptManager::setEventFunction(sol::function function)
{
	eventFunction = function;
}

void ScriptManager::unsetEventFunction()
{
	eventFunction = {};
}

void ScriptManager::setError(std::string error)
{
	if (!error.empty())
	{
		logger.error("Uncaught script error:\n{}", error);
	}
	luaErrorText = std::move(error);
	luaErrorOccurred = true;
}

const std::string & ScriptManager::getError() const
{
	return luaErrorText;
}

bool ScriptManager::hasError() const
{
	return luaErrorOccurred;
}

void ScriptManager::resetError()
{
	luaErrorText.clear();
	luaErrorOccurred = false;
}

void ScriptManager::callEventFunction(Event eventType, sol::object eventParameter)
{
	if (eventFunction)
	{
		eventFunction.value()((int) eventType, eventParameter);
	}
	else
	{
		throw Error("No event handler function specified!");
	}
}

bool ScriptManager::hasEventFunction() const
{
	return static_cast<bool>(eventFunction);
}

std::string ScriptManager::getFFIHeader() const
{
	std::string mergedHeader;
	auto resourceList = resources().getResourceList(
	    config().get(confScriptFFIAPIPath), fs::ListFiles | fs::ListRecursive | fs::ListSorted | fs::ListFullPath);

	// TODO add proper topological sorting
	std::sort(resourceList.begin(), resourceList.end());

	for (const auto & headerFilename : resourceList)
	{
		std::vector<char> headerData;
		if (!resources().loadResource(headerFilename, headerData))
		{
			continue;
		}

		std::string header(headerData.data(), headerData.size());

		std::size_t openingBrace = header.find_first_of('{');
		std::size_t closingBrace = header.find_last_of('}');

		if (openingBrace != std::string::npos && closingBrace != std::string::npos && openingBrace < header.size()
		    && openingBrace + 1 < closingBrace)
		{
			header = header.substr(openingBrace + 1, closingBrace - openingBrace - 1);

			static const std::string apiString = "WOSC_API";
			std::size_t found = 0;
			while ((found = header.find(apiString, found)) != std::string::npos)
			{
				header.erase(found, apiString.size());
			}

			mergedHeader += header;
			mergedHeader += "\n";
		}
	}
	return mergedHeader;
}

cfg::Config & ScriptManager::config() const
{
	return game.getConfig();
}

res::AbstractSource & ScriptManager::resources() const
{
	return game.getResources();
}

}
