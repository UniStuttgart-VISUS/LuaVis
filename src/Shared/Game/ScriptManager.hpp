#ifndef SRC_SHARED_GAME_SCRIPTMANAGER_HPP_
#define SRC_SHARED_GAME_SCRIPTMANAGER_HPP_

#include <Shared/Lua/LuaManager.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Sol2/sol.hpp>
#include <memory>
#include <string>
#include <vector>

namespace res
{
class AbstractSource;
}

namespace lua
{
class AbstractBridge;
}

namespace cfg
{
class Config;
}


namespace wos
{
class AbstractGame;
class ResourceLoader;

class ScriptManager
{
public:
	enum class Event
	{
		TICK = 1,
		EXIT,
	};

	ScriptManager(wos::AbstractGame & game);
	virtual ~ScriptManager();

	void init();

	void resetState();
	void initLibraries();
	void initErrorHandler();
	void loadInitScript();

	void setBridges(std::vector<std::shared_ptr<lua::AbstractBridge>> bridges);
	const std::vector<std::shared_ptr<lua::AbstractBridge>> & getBridges() const;

	sol::function loadScript(std::string script);
	bool scriptExists(const std::string & scriptName) const;

	std::vector<std::string> getAllScripts() const;

	bool isScriptFile(const std::string & fileName) const;

	std::string scriptNameToFileName(std::string scriptName) const;
	std::string fileNameToScriptName(std::string fileName) const;

	void setEventFunction(sol::function function);
	void unsetEventFunction();
	void callEventFunction(Event eventType, sol::object eventParameter = sol::lua_nil);
	bool hasEventFunction() const;

	void setError(std::string error);
	const std::string & getError() const;

	bool hasError() const;
	void resetError();

	std::string getFFIHeader() const;

private:
	cfg::Config & config() const;
	res::AbstractSource & resources() const;

	wos::AbstractGame & game;

	lua::LuaManager lua;
	bool luaStateInitialized = false;
	bool luaErrorOccurred = false;
	std::string luaErrorText;

	sol::optional<sol::function> eventFunction;

	std::vector<std::shared_ptr<lua::AbstractBridge>> bridges;

	Logger logger;
};

}

#endif
