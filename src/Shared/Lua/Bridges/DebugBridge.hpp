#ifndef SRC_SHARED_LUA_BRIDGES_DEBUGBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_DEBUGBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace wos
{
class AbstractGame;
class ScriptManager;
}

namespace lua
{

class DebugBridge : public AbstractBridge
{
public:
	DebugBridge(wos::AbstractGame & game, wos::ScriptManager & scriptManager);
	virtual ~DebugBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::AbstractGame & game;
	wos::ScriptManager & scriptManager;

	Logger logger;
};

}

#endif
