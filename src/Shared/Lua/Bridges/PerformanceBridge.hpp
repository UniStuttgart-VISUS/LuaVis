#ifndef SRC_SHARED_LUA_BRIDGES_PERFORMANCEBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_PERFORMANCEBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace wos
{
class PerformanceCounter;
}

namespace lua
{

class PerformanceBridge : public AbstractBridge
{
public:
	PerformanceBridge(wos::PerformanceCounter & performance);
	virtual ~PerformanceBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::PerformanceCounter & performance;
};

}

#endif
