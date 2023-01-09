#include <Shared/Lua/LuaManager.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <vector>

extern "C"
{
	LUALIB_API int luaopen_utf8(lua_State * L);
}

namespace lua
{

LuaManager::LuaManager()
{
	reset();
}

LuaManager::~LuaManager()
{
}

void LuaManager::reset()
{
	state = makeUnique<sol::state>();
}

sol::function LuaManager::loadScript(const char * scriptData, std::size_t scriptSize, const std::string & name) const
{
	auto result = state->load_buffer(scriptData, scriptSize, "@" + name, sol::load_mode::any);
	if (!result.valid())
	{
		sol::error error = result;
		throw Error(error.what());
	}
	return result.get<sol::function>();
}

void LuaManager::loadBaseLibraries()
{
	state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::os,
	                      sol::lib::math, sol::lib::table, sol::lib::debug, sol::lib::bit32, sol::lib::io,
	                      sol::lib::ffi, sol::lib::jit);
	state->require("utf8", luaopen_utf8, true);
}

void LuaManager::setGlobalImpl(const std::vector<std::string> & pathParts, sol::object value)
{
	sol::table table = state->globals();
	for (std::size_t i = 0; i < pathParts.size() - 1; ++i)
	{
		sol::type type = table[pathParts[i]].get_type();
		switch (type)
		{
		case sol::type::table:
			break;

		case sol::type::lua_nil:
		case sol::type::none:
			table[pathParts[i]] = state->create_table();
			break;

		default:
			throw Error("Non-table encountered at '" + pathParts[i] + "' while attempting to assign global '"
			            + joinString(pathParts, ".") + "' (type = " + sol::type_name(state->lua_state(), type) + ")");
		}

		table = table[pathParts[i]];
	}

	table[pathParts.back()] = value;
}

sol::object LuaManager::getGlobal(const std::string & path) const
{
	auto pathParts = splitGlobalPath(path);

	sol::table table = state->globals();
	for (std::size_t i = 0; i < pathParts.size() - 1; ++i)
	{
		sol::type type = table[pathParts[i]].get_type();
		switch (type)
		{
		case sol::type::table:
			table = table[pathParts[i]];
			break;

		case sol::type::lua_nil:
		case sol::type::none:
			throw Error("Missing table entry '" + pathParts[i] + "' while attempting to retrieve global '" + path
			            + "'");
			break;

		default:
			throw Error("Non-table encountered at '" + pathParts[i] + "' while attempting to retrieve global '" + path
			            + "' (type = " + sol::type_name(state->lua_state(), type) + ")");
		}
	}

	return table[pathParts.back()];
}

sol::state & LuaManager::getState() const
{
	return *state;
}

std::vector<std::string> LuaManager::splitGlobalPath(const std::string & path) const
{
	auto pathParts = splitString(path, ".", true);

	if (pathParts.empty())
	{
		throw Error("Path to global variable cannot be empty");
	}
	else
	{
		return pathParts;
	}
}
}
