#ifndef SRC_SHARED_LUA_IMPL_LUAJIT_HPP_
#define SRC_SHARED_LUA_IMPL_LUAJIT_HPP_

#include <Shared/Lua/LuaTypes.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Sol2/sol.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

struct lua_State;

namespace lua
{

class LuaJIT
{
public:
	LuaJIT();
	~LuaJIT();

	void executeScript(const char * scriptData, std::size_t scriptSize, const std::string & name = "unnamed script");

	void pushVariable(const std::string & variableName);
	void push(const Value & value);
	Value pop();

	template <typename Ret, typename... Args>
	void bindFunction(const std::string & globalVar, std::function<Ret(Args...)> function)
	{
		bindFunction(splitString(globalVar, ".", true), function);
	}

	template <typename Ret, typename... Args>
	void bindFunction(const std::vector<std::string> & globalPath, std::function<Ret(Args...)> function)
	{
		if (globalPath.empty())
		{
			throw Error("Function name cannot be empty.");
		}

		sol::state_view stateWrapper(myState);
		sol::table table = stateWrapper.globals();
		for (std::size_t i = 0; i < globalPath.size() - 1; ++i)
		{
			switch (table[globalPath[i]].get_type())
			{
			case sol::type::table:
				break;

			case sol::type::lua_nil:
				table[globalPath[i]] = stateWrapper.create_table();
				break;

			default:
				throw Error("Non-table encountered at '" + globalPath[i] + "' while attempting to bind function to '"
				            + joinString(globalPath, ".") + "'.");
			}

			table = table[globalPath[i]];
		}
		table[globalPath.back()] = function;
	}

	void execute(std::size_t parameterCount, std::size_t returnValueCount);

private:
	Value toValue(int index) const;

	lua_State * myState;
};

}

#endif
