#ifndef SRC_SHARED_CONFIG_DATATYPES_HPP_
#define SRC_SHARED_CONFIG_DATATYPES_HPP_

#include <SFML/Config.hpp>
#include <Shared/Utils/StaticKey.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>

namespace cfg
{

enum class NodeType
{
	Missing,
	Null,

	String,
	Bool,
	Int,
	Float,

	Array,
	Map,
};

// TODO rewrite to eliminate string conversion
struct Value
{
	Value(NodeType type = NodeType::Missing) :
		type(type),
		content()
	{
	}

	Value(std::string content) :
		type(NodeType::String),
		content(std::move(content))
	{
	}

	Value(bool value) :
		type(NodeType::Bool),
		content(value ? "true" : "false")
	{
	}

	Value(sf::Int64 value) :
		type(NodeType::Int),
		content(cNtoS(value))
	{
	}

	Value(sf::Uint64 value) :
		type(NodeType::Int),
		content(cNtoS(value))
	{
	}

	Value(double value) :
		type(NodeType::Float),
		content(cNtoS(value))
	{
	}

	NodeType type;
	std::string content;
};

namespace detail
{

class CacheKey
{
public:
	using ID = std::uint32_t;
	static constexpr ID InvalidID = std::numeric_limits<ID>::max();

	CacheKey(std::string key);

	ID getID() const;
	const std::string & getKey() const;

	static ID addKey(std::string key);
	static ID lookUpKey(const std::string & key);
	static const std::string & lookUpID(ID id);

private:
	static std::unordered_map<std::string, ID> & getKeyMap();
	static std::vector<std::string> & getKeyList();

	ID id;
	std::string key;
};

class KeyBase
{
public:
	KeyBase(std::string key);

	CacheKey::ID getID() const;
	const std::string & getKey() const;

private:
	CacheKey cacheKey;
};

class StringKey : public KeyBase
{
public:
	using DataType = std::string;

	inline static DataType convertTo(Value value)
	{
		return std::move(value.content);
	}

	inline static Value convertFrom(DataType value)
	{
		return Value(std::move(value));
	}

	StringKey(std::string key) :
		KeyBase(std::move(key))
	{
	}
};

class BoolKey : public KeyBase
{
public:
	using DataType = bool;

	static constexpr const char * TRUE_VALUE = "true";
	static constexpr const char * FALSE_VALUE = "false";

	inline static DataType convertTo(Value value)
	{
		if (value.content == TRUE_VALUE)
		{
			return true;
		}
		else if (value.content.empty() || value.content == FALSE_VALUE)
		{
			return false;
		}
		else
		{
			return cStoI(value.content) != 0;
		}
	}

	inline static Value convertFrom(DataType value)
	{
		return Value(value);
	}

	BoolKey(std::string key) :
		KeyBase(std::move(key))
	{
	}
};

class IntKey : public KeyBase
{
public:
	using DataType = sf::Int64;

	inline static DataType convertTo(Value value)
	{
		return cStoN<DataType>(value.content);
	}

	inline static Value convertFrom(DataType value)
	{
		return Value(value);
	}

	IntKey(std::string key) :
		KeyBase(std::move(key))
	{
	}
};

class FloatKey : public KeyBase
{
public:
	using DataType = double;

	inline static DataType convertTo(Value value)
	{
		return cStoN<DataType>(value.content);
	}

	inline static Value convertFrom(DataType value)
	{
		return Value(value);
	}

	FloatKey(std::string key) :
		KeyBase(std::move(key))
	{
	}
};

}

}

#endif
