#include <Shared/Config/DataTypes.hpp>
#include <Shared/Config/JSONConfig.hpp>
#include <Shared/External/RapidJSON/error/en.h>
#include <Shared/External/RapidJSON/error/error.h>
#include <Shared/External/RapidJSON/prettywriter.h>
#include <Shared/External/RapidJSON/rapidjson.h>
#include <Shared/External/RapidJSON/reader.h>
#include <Shared/External/RapidJSON/stringbuffer.h>
#include <Shared/External/RapidJSON/writer.h>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <cassert>

namespace cfg
{

const std::string JSONConfig::LENGTH_NODE = "length";
const std::string JSONConfig::LENGTH_SUFFIX = ".length";

JSONConfig::JSONConfig()
{
	document = makeUnique<rapidjson::Document>();
}

JSONConfig::~JSONConfig()
{
}

void JSONConfig::loadFromMemory(const char * data, std::size_t size)
{
	document = makeUnique<rapidjson::Document>();
	rapidjson::ParseResult result =
	    document->Parse<rapidjson::kParseTrailingCommasFlag | rapidjson::kParseCommentsFlag>(data, size);

	if (result.IsError())
	{
		throw Error(std::string("Error parsing JSON: ") + rapidjson::GetParseError_En(result.Code())
		            + " [Error location: character " + cNtoS(result.Offset()) + "]");
	}
}

void JSONConfig::loadFromString(const std::string & json)
{
	loadFromMemory(json.data(), json.size());
}

std::string JSONConfig::saveToString(Style style) const
{
	if (document == nullptr)
	{
		return "";
	}

	rapidjson::StringBuffer buffer;

	switch (style)
	{
	case Style::Compact:
	default:
	{
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document->Accept(writer);
	}
	break;
	case Style::Pretty:
	{
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		document->Accept(writer);
	}
	break;
	}
	return buffer.GetString();
}

Value JSONConfig::readValue(const std::string & key) const
{
	if (document == nullptr)
	{
		return Value();
	}

	std::vector<PathComponent> pathComponents = getPathComponents(key);

	// Special case: "[...][type]" suffixes
	if (pathComponents.size() >= 2 && pathComponents.back().getType() == PathComponent::Type::DataType)
	{
		switch (pathComponents[pathComponents.size() - 2].getType())
		{
		case PathComponent::Type::DataType:
		case PathComponent::Type::ArrayLength:
			return Value(sf::Int64(NodeType::Int));

		case PathComponent::Type::KeyList:
			return Value(sf::Int64(NodeType::Array));

		default:
			break;
		}
	}

	const rapidjson::Value * currentNode = document.get();
	PathParseState parseState = PathParseState::Node;

	for (const PathComponent & component : pathComponents)
	{
		switch (component.getType())
		{
		case PathComponent::Type::ObjectKey:
		{
			if (!currentNode->IsObject())
			{
				return Value();
			}

			auto result = currentNode->FindMember(component.getKey().c_str());

			if (result == currentNode->MemberEnd())
			{
				return Value();
			}

			currentNode = &result->value;
			break;
		}

		case PathComponent::Type::ArrayIndex:
			if (parseState == PathParseState::Node && currentNode->IsArray()
			    && component.getIndex() < currentNode->Size())
			{
				currentNode = &((*currentNode)[component.getIndex()]);
			}
			else if (parseState == PathParseState::KeyList && currentNode->IsObject()
			         && component.getIndex() < currentNode->MemberCount())
			{
				return getNodeValue((currentNode->MemberBegin() + component.getIndex())->name);
			}
			else
			{
				return Value();
			}

			break;

		case PathComponent::Type::ArrayLength:
			if (parseState == PathParseState::Node && currentNode->IsArray())
			{
				return Value((sf::Uint64) currentNode->Size());
			}
			else if (parseState == PathParseState::KeyList && currentNode->IsObject())
			{
				return Value((sf::Uint64) currentNode->MemberCount());
			}
			else
			{
				return Value();
			}
			break;

		case PathComponent::Type::KeyList:
			if (parseState == PathParseState::Node && currentNode->IsObject())
			{
				parseState = PathParseState::KeyList;
			}
			else
			{
				return Value();
			}
			break;

		case PathComponent::Type::DataType:
			if (parseState == PathParseState::Node)
			{
				return Value(sf::Int64(getNodeType(*currentNode)));
			}
			else
			{
				return Value();
			}
			break;

		default:
			return Value();
		}
	}

	return getNodeValue(*currentNode);
}

void JSONConfig::writeValue(const std::string & key, Value value)
{
	if (document == nullptr)
	{
		return;
	}

	rapidjson::Value * currentNode = document.get();

	std::vector<PathComponent> pathComponents = getPathComponents(key);

	for (const PathComponent & component : pathComponents)
	{
		switch (component.getType())
		{
		case PathComponent::Type::ObjectKey:
		{
			if (!currentNode->IsObject())
			{
				currentNode->SetObject();
			}

			auto result = currentNode->FindMember(component.getKey().c_str());

			if (result == currentNode->MemberEnd())
			{
				// Member does not exist, create it.
				currentNode->AddMember(rapidjson::Value(component.getKey().c_str(), document->GetAllocator()),
				                       rapidjson::Value(rapidjson::kObjectType), document->GetAllocator());

				result = currentNode->FindMember(component.getKey().c_str());

				assert(result != currentNode->MemberEnd());
			}

			currentNode = &result->value;
			break;
		}

		case PathComponent::Type::ArrayIndex:
			if (!currentNode->IsArray())
			{
				currentNode->SetArray();
			}

			if (currentNode->Capacity() <= component.getIndex())
			{
				currentNode->Reserve(component.getIndex() + 1, document->GetAllocator());
			}

			while (currentNode->Size() <= component.getIndex())
			{
				currentNode->PushBack(rapidjson::Value(), document->GetAllocator());
			}

			currentNode = &((*currentNode)[component.getIndex()]);
			break;

		case PathComponent::Type::ArrayLength:
			if (value.type == NodeType::Int)
			{
				sf::Uint64 arrayLength = cStoUL(value.content);
				if (!currentNode->IsArray())
				{
					currentNode->SetArray();
				}

				while (currentNode->Size() > arrayLength)
				{
					currentNode->PopBack();
				}

				currentNode->Reserve(arrayLength, document->GetAllocator());

				while (currentNode->Size() < arrayLength)
				{
					currentNode->PushBack(rapidjson::Value(), document->GetAllocator());
				}
			}
			return;

		default:
			return;
		}
	}

	setNodeValue(*currentNode, std::move(value));
}

std::vector<JSONConfig::PathComponent> JSONConfig::getPathComponents(const std::string & key) const
{
	std::vector<PathComponent> components;

	for (std::string & pathString : splitString(key, "."))
	{
		if (pathString.empty())
		{
			throw Error("Malformed config path \"" + key + "\" (expected key name)");
		}

		std::size_t openBracketIndex = pathString.find_first_of('[');

		if (openBracketIndex != std::string::npos)
		{
			std::string pathSubStr = pathString.substr(0, openBracketIndex);
			if (!pathSubStr.empty())
			{
				checkPathString(pathSubStr);
				components.emplace_back(PathComponent::Type::ObjectKey, std::move(pathSubStr));
			}

			while (openBracketIndex != std::string::npos)
			{
				std::size_t closedBracketIndex = pathString.find_first_of(']', openBracketIndex + 1);

				if (closedBracketIndex == std::string::npos)
				{
					throw Error("Malformed config path \"" + key + "\" (expected ']')");
				}

				std::string bracketStr =
				    pathString.substr(openBracketIndex + 1, closedBracketIndex - openBracketIndex - 1);

				if (bracketStr == "length")
				{
					components.emplace_back(PathComponent::Type::ArrayLength);
				}
				else if (bracketStr == "keys")
				{
					components.emplace_back(PathComponent::Type::KeyList);
				}
				else if (bracketStr == "type")
				{
					components.emplace_back(PathComponent::Type::DataType);
				}
				else if (!bracketStr.empty() && stringIsPositiveInteger(bracketStr))
				{
					components.emplace_back(PathComponent::Type::ArrayIndex, cStoUL(bracketStr));
				}
				else
				{
					throw Error("Malformed config path \"" + key + "\" (expected array index)");
				}
				openBracketIndex = pathString.find_first_of('[', closedBracketIndex);
			}
		}
		else
		{
			checkPathString(pathString);
			components.emplace_back(PathComponent::Type::ObjectKey, std::move(pathString));
		}
	}

	return components;
}

void JSONConfig::checkPathString(const std::string & pathString) const
{
	for (char ch : pathString)
	{
		if ((ch < 'a' || ch > 'z') && (ch < 'A' || ch > 'Z') && (ch < '0' || ch > '9') && ch != '_')
		{
			throw Error("Malformed config node name \"" + pathString + "\" (only A-Z, a-z, 0-9 and _ are allowed)");
		}
	}
}

void JSONConfig::setNodeValue(rapidjson::Value & node, Value value)
{
	switch (value.type)
	{
	case NodeType::Bool:
		node.SetBool(value.content == "true");
		break;
	case NodeType::Int:
		node.SetInt64(cStoL(value.content));
		break;
	case NodeType::String:
		node.SetString(value.content.data(), value.content.size(), document->GetAllocator());
		break;
	case NodeType::Float:
		node.SetDouble(cStoD(value.content));
		break;
	case NodeType::Missing:
	case NodeType::Null:
	default:
		node.SetNull();
		break;
	}
}

Value JSONConfig::getNodeValue(const rapidjson::Value & node) const
{
	if (node.IsTrue())
	{
		return Value(true);
	}
	else if (node.IsFalse())
	{
		return Value(false);
	}
	else if (node.IsDouble())
	{
		return Value(node.GetDouble());
	}
	else if (node.IsInt64())
	{
		return Value((sf::Int64) node.GetInt64());
	}
	else if (node.IsUint64())
	{
		return Value((sf::Uint64) node.GetUint64());
	}
	else if (node.IsString())
	{
		return Value(std::string(node.GetString(), node.GetStringLength()));
	}

	return Value();
}

NodeType JSONConfig::getNodeType(const rapidjson::Value & node) const
{
	if (node.IsArray())
	{
		return NodeType::Array;
	}
	else if (node.IsObject())
	{
		return NodeType::Map;
	}
	// TODO optimize getting node type
	return getNodeValue(node).type;
}

}
