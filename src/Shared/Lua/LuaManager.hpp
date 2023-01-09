#ifndef SRC_SHARED_LUA_LUAMANAGER_HPP_
#define SRC_SHARED_LUA_LUAMANAGER_HPP_

#include <Shared/Utils/Error.hpp>
#include <Sol2/sol.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>

namespace lua
{

class LuaManager
{
public:
	LuaManager();
	~LuaManager();

	void reset();

	sol::function loadScript(const char * scriptData, std::size_t scriptSize, const std::string & name = "") const;

	void loadBaseLibraries();

	template <typename T>
	void setGlobal(const std::string & path, T value)
	{
		auto pathParts = splitGlobalPath(path);
		sol::object object(state->lua_state(), sol::in_place, value);
		setGlobalImpl(pathParts, object);
	}

	sol::object getGlobal(const std::string & path) const;

	template <typename... Parameters>
	sol::object execute(sol::function function, Parameters... parameters)
	{
		sol::function_result result = function.call(parameters...);
		if (result.valid())
		{
			return result;
		}
		else
		{
			sol::error error = result;
			throw Error(std::string("Error executing function: ") + error.what());
		}
	}

	template <typename ReturnType, typename... Args>
	void bindFunction(const std::string & path, std::function<ReturnType(Args...)> function)
	{
		setGlobal(path, function);
	}

	sol::state & getState() const;

private:
	std::vector<std::string> splitGlobalPath(const std::string & path) const;

	void setGlobalImpl(const std::vector<std::string> & pathParts, sol::object value);

	std::unique_ptr<sol::state> state;
};

}

#endif
