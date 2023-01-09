#ifndef SRC_SHARED_LUA_BRIDGES_UTILITYBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_UTILITYBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace wos
{
class AbstractGame;
}

namespace lua
{

class UtilityBridge : public AbstractBridge
{
public:
	UtilityBridge();
	virtual ~UtilityBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	Logger logger;
};

}

#endif
