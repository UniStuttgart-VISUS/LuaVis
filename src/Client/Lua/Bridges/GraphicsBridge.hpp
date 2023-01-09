#ifndef SRC_CLIENT_LUA_BRIDGES_GRAPHICSBRIDGE_HPP_
#define SRC_CLIENT_LUA_BRIDGES_GRAPHICSBRIDGE_HPP_

#include <Client/GameRenderer/GraphicsManager.hpp>
#include <Client/Lua/Bindings/GraphicsBinding.h>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>

namespace lua
{

class GraphicsBridge : public AbstractBridge
{
public:
	GraphicsBridge(wos::GraphicsManager & manager);
	virtual ~GraphicsBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	wos::GraphicsManager & manager;
};

}

#endif
