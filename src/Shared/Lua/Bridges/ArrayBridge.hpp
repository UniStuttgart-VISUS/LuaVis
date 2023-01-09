#ifndef SRC_SHARED_LUA_BRIDGES_ARRAYBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_ARRAYBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace wosc
{
class ArrayContext;
}

namespace lua
{

class ArrayBridge : public AbstractBridge
{
public:
	ArrayBridge(wosc::ArrayContext & arrayContext);
	virtual ~ArrayBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wosc::ArrayContext & arrayContext;
};

}

#endif
