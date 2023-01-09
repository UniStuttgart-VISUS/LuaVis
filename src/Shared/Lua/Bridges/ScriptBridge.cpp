#include <Shared/Game/ScriptWorker.hpp>
#include <Shared/Lua/Bridges/ScriptBridge.hpp>
#include <Shared/Lua/LuaUtils.hpp>
#include <Shared/Utils/Error.hpp>
#include <Sol2/sol.hpp>
#include <functional>
#include <string>

namespace lua
{

ScriptBridge::ScriptBridge(wos::ScriptManager & manager) :
	manager(manager)
{
}

ScriptBridge::~ScriptBridge()
{
}

void ScriptBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("script.load", std::function<sol::object(sol::this_state state, std::string)>(
	                               [=](sol::this_state state, std::string scriptName) -> sol::object {
		                               try
		                               {
			                               return manager.loadScript(scriptName);
		                               }
		                               catch (Error & err)
		                               {
			                               return sol::make_object(state.L, err.getErrorText());
		                               }
		                               catch (std::exception & ex)
		                               {
			                               return sol::make_object(state.L, ex.what());
		                               }
	                               }));

	loader.bind("script.scriptExists", std::function<bool(std::string)>([=](std::string scriptName) {
		            return manager.scriptExists(scriptName);
	            }));

	loader.bind("script.listScripts", std::function<sol::table(sol::this_state)>([=](sol::this_state state) {
		            return listToTable(manager.getAllScripts(), state);
	            }));

	loader.bind("script.setEventFunction", std::function<void(sol::object)>([=](sol::object tickFunc) {
		            if (tickFunc.get_type() == sol::type::function)
		            {
			            manager.setEventFunction(tickFunc);
		            }
		            else
		            {
			            manager.unsetEventFunction();
		            }
	            }));

	loader.bind("script.getFFIHeader", std::function<std::string()>([=]() {
		            return manager.getFFIHeader();
	            }));

	// clang-format off
	loader.registerType<wos::ScriptWorker>("ScriptWorker",
		"clearIO", &wos::ScriptWorker::clearIO,
		"start", &wos::ScriptWorker::start,
		"requestStop", &wos::ScriptWorker::requestStop,
		"isRunning", &wos::ScriptWorker::isRunning,
		"isStopRequested", &wos::ScriptWorker::isStopRequested,
		"join", &wos::ScriptWorker::join,
		"pushInput", &wos::ScriptWorker::pushInput,
		"hasOutput", &wos::ScriptWorker::hasOutput,
		"peekOutput", &wos::ScriptWorker::peekOutput,
		"popOutput", &wos::ScriptWorker::popOutput
	);
	// clang-format on

	loader.bind("script.createWorker", std::function<std::shared_ptr<wos::ScriptWorker>()>(
	                                       [=]()
	                                       {
		                                       return std::make_shared<wos::ScriptWorker>();
	                                       }));
}

}
