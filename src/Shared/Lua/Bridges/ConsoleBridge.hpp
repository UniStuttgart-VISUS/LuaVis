#ifndef SRC_SHARED_LUA_BRIDGES_CONSOLEBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_CONSOLEBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace wos
{
class AbstractGame;
}

namespace lua
{

class ConsoleBridge : public AbstractBridge
{
public:
	ConsoleBridge();
	virtual ~ConsoleBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	Logger logger;
};

}

#endif
