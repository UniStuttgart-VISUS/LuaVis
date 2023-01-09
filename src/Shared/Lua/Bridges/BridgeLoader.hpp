#ifndef SRC_SHARED_LUA_BRIDGES_BRIDGELOADER_HPP_
#define SRC_SHARED_LUA_BRIDGES_BRIDGELOADER_HPP_

#include <Shared/Lua/LuaManager.hpp>
#include <functional>
#include <string>

namespace lua
{

class BridgeLoader
{
public:
	BridgeLoader(LuaManager & manager);

	template <typename Ret, typename... Args>
	void bind(std::string name, std::function<Ret(Args...)> function)
	{
		if (name.empty())
		{
			throw Error("Function name cannot be empty!");
		}
		manager.setGlobal("bridge." + name, function);
	}

	template <typename Class, typename... Args>
	void registerType(const std::string & name, Args &&... args)
	{
		manager.getState().new_usertype<Class>(name, std::forward<Args>(args)...);
	}

private:
	LuaManager & manager;
};

}

#endif
