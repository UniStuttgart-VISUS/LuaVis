#ifndef SRC_SHARED_LUA_BRIDGES_COREBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_COREBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace wos
{
class AbstractGame;
}

namespace lua
{

class CoreBridge : public AbstractBridge
{
public:
	CoreBridge(wos::AbstractGame & game);
	virtual ~CoreBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::AbstractGame & game;

	Logger logger;
};

}

#endif
