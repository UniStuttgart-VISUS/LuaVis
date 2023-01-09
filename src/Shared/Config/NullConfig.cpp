#include <Shared/Config/DataTypes.hpp>
#include <Shared/Config/NullConfig.hpp>

namespace cfg
{

NullConfig::NullConfig()
{
}

NullConfig::~NullConfig()
{
}

Value NullConfig::readValue(const std::string & key) const
{
	return Value(NodeType::Null);
}

void NullConfig::writeValue(const std::string & key, Value value)
{
}

}
