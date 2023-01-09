#include <Shared/Content/LuaSource.hpp>
#include <Shared/Lua/LuaUtils.hpp>

namespace res
{

LuaSource::LuaSource(wosc::ArrayContext * arrayContext) :
	arrayContext(arrayContext),
	logger("LuaSource")
{
}

LuaSource::LuaSource(wosc::ArrayContext * arrayContext, sol::function function) :
	LuaSource(arrayContext)
{
	this->function = function;
}

LuaSource::~LuaSource()
{
}

void LuaSource::setFunction(sol::function function)
{
	this->function = function;
}

void LuaSource::unsetFunction()
{
	this->function = {};
}

bool LuaSource::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	try
	{
		if (function)
		{
			sol::state_view stateView = sol::state_view(function.value().lua_state());
			sol::table parameter = stateView.create_table();
			parameter["event"] = "load";
			parameter["name"] = resourceName;

			function.value()(parameter);

			auto data = parameter["data"];
			switch (data.get_type())
			{
			case sol::type::number:
			{
				if (arrayContext)
				{
					auto arrayInfo = arrayContext->getArrayInfo(lua::getOr<wosc::ArrayContext::ArrayID>(data, 0));
					if (arrayInfo.data)
					{
						dataTarget.assign((const char *) arrayInfo.data,
						                  (const char *) arrayInfo.data + arrayInfo.size);
						return true;
					}
					else
					{
						logger.warn("Invalid array returned for virtual Lua resource '{}'", resourceName);
					}
				}
				else
				{
					logger.warn("Attempt to load virtual Lua resource '{}' via array without a valid array context",
					            resourceName);
				}
				break;
			}

			case sol::type::string:
			{
				auto stringData = lua::getOr<std::string>(data, "");
				dataTarget.assign(stringData.begin(), stringData.end());
				return true;
			}

			case sol::type::lua_nil:
			case sol::type::none:
				break;

			default:
				logger.warn("Invalid data type '{}' returned for virtual Lua resource '{}'",
				            sol::type_name(stateView.lua_state(), data.get_type()), resourceName);
				break;
			}
		}
	}
	catch (std::exception & ex)
	{
		logger.error("Error while loading virtual Lua resource '{}': {}", resourceName, ex.what());
	}

	return false;
}

std::vector<std::string> LuaSource::getResourceList(std::string prefix, fs::ListFlags flags) const
{
	try
	{
		if (function)
		{
			sol::table parameter = sol::state_view(function.value().lua_state()).create_table();
			parameter["event"] = "list";
			parameter["path"] = prefix;
			parameter["flags"] = int(flags);

			function.value()(parameter);
			auto result = parameter["resources"];
			if (result.get_type() == sol::type::table)
			{
				return lua::tableToVector<std::string>(result);
			}
		}
	}
	catch (std::exception & ex)
	{
		logger.error("Error while listing virtual Lua resources in '{}': {}", prefix, ex.what());
	}

	return {};
}

bool LuaSource::resourceExists(const std::string & resourceName) const
{
	try
	{
		if (function)
		{
			sol::table parameter = sol::state_view(function.value().lua_state()).create_table();
			parameter["event"] = "check";
			parameter["name"] = resourceName;

			function.value()(parameter);
			return lua::getOr<bool>(parameter["exists"], false);
		}
	}
	catch (std::exception & ex)
	{
		logger.error("Error while loading virtual Lua resource '{}': {}", resourceName, ex.what());
	}

	return false;
}

void LuaSource::pollChanges()
{
	try
	{
		sol::table parameter = sol::state_view(function.value().lua_state()).create_table();
		parameter["event"] = "poll";

		while (true)
		{
			function.value()(parameter);
			int type = lua::getOr<int>(parameter["type"], 0);
			if (type != 0)
			{
				fireEvent(ResourceEvent((ResourceEvent::Type) type, lua::getOr<std::string>(parameter["name"], "")));
			}
			else
			{
				return;
			}
		}
	}
	catch (std::exception & ex)
	{
		logger.error("Error while polling virtual Lua resource: {}", ex.what());
	}
}

}
