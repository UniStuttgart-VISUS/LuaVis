#ifndef SRC_SHARED_CONFIG_JSONCONFIGSOURCE_HPP_
#define SRC_SHARED_CONFIG_JSONCONFIGSOURCE_HPP_

#include <Shared/Config/ConfigSource.hpp>
#include <Shared/External/RapidJSON/document.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class DataStream;

namespace cfg
{

class JSONConfig : public ConfigSource
{
public:
	static const std::string LENGTH_NODE;
	static const std::string LENGTH_SUFFIX;

	JSONConfig();
	virtual ~JSONConfig();

	void loadFromMemory(const char * data, std::size_t size);
	void loadFromString(const std::string & json);

	enum class Style
	{
		Pretty,
		Compact
	};

	std::string saveToString(Style style = Style::Pretty) const;

	virtual Value readValue(const std::string & key) const override;
	virtual void writeValue(const std::string & key, Value value) override;

private:
	struct PathComponent
	{
	public:
		enum class Type
		{
			ObjectKey,
			ArrayIndex,

			ArrayLength,
			KeyList,
			DataType
		};

		PathComponent(Type type, unsigned int index = 0) : type(type), index(index)
		{
		}

		PathComponent(Type type, std::string key) :
			type(type),
			index(0),
			key(std::move(key))
		{
		}

		Type getType() const
		{
			return type;
		}

		unsigned int getIndex() const
		{
			return index;
		}

		const std::string & getKey() const
		{
			return key;
		}

	private:
		Type type;
		unsigned int index;
		std::string key;
	};

	enum class PathParseState
	{
		Node,
		KeyList,
	};

	std::vector<PathComponent> getPathComponents(const std::string & key) const;
	void checkPathString(const std::string & pathString) const;

	void setNodeValue(rapidjson::Value & node, Value value);
	Value getNodeValue(const rapidjson::Value & node) const;
	NodeType getNodeType(const rapidjson::Value & node) const;

	std::unique_ptr<rapidjson::Document> document;

	friend class JSONHandler;
};

}

#endif /* SRC_SHARED_CONFIG_JSONCONFIGSOURCE_HPP_ */
