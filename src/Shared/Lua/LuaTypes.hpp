#ifndef SRC_SHARED_LUA_LUATYPES_HPP_
#define SRC_SHARED_LUA_LUATYPES_HPP_

#include <Shared/Utils/StrNumCon.hpp>
#include <limits>
#include <stdint.h>
#include <string>

namespace lua
{

using ScriptID = int32_t;
constexpr ScriptID SCRIPT_NONE = 0;

class Value
{
public:
	enum Type
	{
		Nil,
		Number,
		String,
		Boolean,
		Function,
		Unknown
	};

	Value() :
		type(Nil),
		number(),
		string()
	{
	}

	Value(double number) :
		type(Number),
		number(number),
		string()
	{
	}

	Value(std::string string) :
		type(String),
		number(),
		string(string)
	{
	}

	Value(Type type) :
		type(type),
		number(),
		string()
	{
	}

	inline static Value fromBoolean(bool value)
	{
		Value ret(value ? 1 : 0);
		ret.type = Boolean;
		return ret;
	}

	inline Type getType() const
	{
		return type;
	}

	inline bool hasValue() const
	{
		return type != Nil;
	}

	inline bool isNil() const
	{
		return type == Nil;
	}

	inline double getNumber() const
	{
		switch (type)
		{
		case Nil:
		default:
			return std::numeric_limits<double>::quiet_NaN();
		case Number:
		case Boolean:
			return number;
		case String:
			return cStoD(string);
		}
	}

	std::string getString() const
	{
		switch (type)
		{
		case Nil:
		default:
			return "nil";
		case Number:
			return cNtoS(number);
		case String:
			return string;
		case Boolean:
			return getBoolean() ? "true" : "false";
		}
	}

	bool getBoolean() const
	{
		switch (type)
		{
		case Nil:
		default:
			return false;
		case Number:
		case Boolean:
			return number != 0;
		case String:
			return string == "true";
		}
	}

private:
	Type type;
	double number;
	std::string string;
};

}

#endif
