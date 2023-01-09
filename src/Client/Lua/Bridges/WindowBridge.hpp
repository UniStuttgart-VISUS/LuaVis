#ifndef SRC_CLIENT_LUA_BRIDGES_WINDOWBRIDGE_HPP_
#define SRC_CLIENT_LUA_BRIDGES_WINDOWBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace gui3
{
class Widget;
}

namespace lua
{

class WindowBridge : public AbstractBridge
{
public:
	WindowBridge(gui3::Widget & widget);
	virtual ~WindowBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	gui3::Widget & widget;
	Logger logger;
};

}

#endif
