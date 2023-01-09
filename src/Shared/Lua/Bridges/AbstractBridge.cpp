#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace lua
{

AbstractBridge::AbstractBridge()
{
}

AbstractBridge::~AbstractBridge()
{
}

void AbstractBridge::load(LuaManager & manager)
{
	lua::BridgeLoader bridgeLoader(manager);
	onLoad(bridgeLoader);
}

}
