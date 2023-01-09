#ifndef SRC_CLIENT_LUA_BRIDGES_INPUTBRIDGE_HPP_
#define SRC_CLIENT_LUA_BRIDGES_INPUTBRIDGE_HPP_

#include <Client/Game/InputManager.hpp>
#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace lua
{

class InputBridge : public AbstractBridge
{
public:
	InputBridge(wos::InputManager & manager);
	virtual ~InputBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::InputManager & manager;

	// TODO use event-based approach instead
	unsigned int lastControllerSetupChangeCount = -1;

	Logger logger;
};

}

#endif
