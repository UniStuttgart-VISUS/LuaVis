#ifndef SRC_SHARED_LUA_BRIDGES_CONFIGBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_CONFIGBRIDGE_HPP_

#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace cfg
{
class Config;
}
namespace wos
{
class AbstractGame;
}


namespace lua
{

class ConfigBridge : public AbstractBridge
{
public:
	ConfigBridge(wos::AbstractGame & game);
	virtual ~ConfigBridge();

protected:
	void onLoad(BridgeLoader & loader) override;

private:
	cfg::Config & getConfig() const;

	wos::AbstractGame & game;
};

}

#endif
