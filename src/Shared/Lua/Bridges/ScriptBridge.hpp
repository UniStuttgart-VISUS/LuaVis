#ifndef SRC_SHARED_LUA_BRIDGES_SCRIPTBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_SCRIPTBRIDGE_HPP_

#include <Shared/Game/ScriptManager.hpp>
#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace wos
{
class ScriptManager;
}

namespace lua
{

class ScriptBridge : public AbstractBridge
{
public:
	ScriptBridge(wos::ScriptManager & manager);
	virtual ~ScriptBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::ScriptManager & manager;
};

}

#endif
