#include <Shared/Lua/Impl/LuaJIT.hpp>
#include <Shared/Utils/Error.hpp>

namespace lua
{

LuaJIT::LuaJIT()
{
	myState = luaL_newstate();
	luaL_openlibs(myState);
	/*
	 luaopen_base(myState);
	 luaopen_bit(myState);
	 luaopen_math(myState);
	 luaopen_ffi(myState);
	 luaopen_package(myState);
	 */
}

LuaJIT::~LuaJIT()
{
	lua_close(myState);
}

void LuaJIT::executeScript(const char * scriptData, std::size_t scriptSize, const std::string & name)
{
	if (luaL_loadbuffer(myState, scriptData, scriptSize, ("@" + name).c_str()) == 0)
	{
		execute(0, 0);
	}
	else
	{
		throw Error(pop().getString());
	}
}

void LuaJIT::pushVariable(const std::string & variableName)
{
	lua_getglobal(myState, variableName.c_str());
}

void LuaJIT::push(const Value & value)
{
	switch (value.getType())
	{
	case Value::Nil:
	default:
		lua_pushnil(myState);
		break;
	case Value::Number:
		lua_pushnumber(myState, value.getNumber());
		break;
	case Value::String:
		lua_pushlstring(myState, value.getString().data(), value.getString().size());
		break;
	case Value::Boolean:
		lua_pushboolean(myState, value.getBoolean());
		break;
	}
}

Value LuaJIT::pop()
{
	Value value = toValue(-1);
	lua_pop(myState, 1);
	return value;
}

void LuaJIT::execute(std::size_t parameterCount, std::size_t returnValueCount)
{
	if (lua_pcall(myState, parameterCount, returnValueCount, 0) != 0)
	{
		throw Error(pop().getString());
	}
}

Value LuaJIT::toValue(int index) const
{
	if (lua_isnil(myState, index))
	{
		return Value();
	}
	else if (lua_isnumber(myState, index))
	{
		return lua_tonumber(myState, index);
	}
	else if (lua_isstring(myState, index))
	{
		std::size_t length = 0;
		const char * str = lua_tolstring(myState, index, &length);
		return std::string(str, length);
	}
	else if (lua_isboolean(myState, index))
	{
		return (bool) lua_toboolean(myState, index);
	}
	else if (lua_isfunction(myState, index))
	{
		return Value::Function;
	}

	return Value::Unknown;
}

}
