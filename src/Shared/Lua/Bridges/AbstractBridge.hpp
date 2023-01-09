#ifndef SRC_SHARED_LUA_BRIDGES_ABSTRACTBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_ABSTRACTBRIDGE_HPP_


namespace lua
{
class LuaManager;
class BridgeLoader;

class AbstractBridge
{
public:
	AbstractBridge();
	virtual ~AbstractBridge();

	void load(LuaManager & manager);

protected:
	virtual void onLoad(BridgeLoader & loader) = 0;
};

}

#endif
