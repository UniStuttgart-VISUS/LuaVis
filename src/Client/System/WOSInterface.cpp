#include <Client/GUI3/Widgets/Panels/Panel.hpp>
#include <Client/System/WOSClient.hpp>
#include <Client/System/WOSInterface.hpp>
#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>

WOSInterface::WOSInterface(WOSClient * parentApplication) :
	gui3::Interface(parentApplication)
{
	static cfg::String gameName("wos.game.title");
	static cfg::Vector2f gameSize("wos.game.size");
	static cfg::Vector2f windowSize("wos.game.window.size");
	static cfg::Bool windowMaximized("wos.game.window.maximized");

	setTitle(parentApplication->getConfig().get(gameName));

	auto size = sf::Vector2u(parentApplication->getConfig().get(windowSize));
	if (size.x > 1 && size.y > 1)
	{
		resize(size, false);
	}
	else
	{
		resize(sf::Vector2u(parentApplication->getConfig().get(gameSize)), false);
	}

	if (parentApplication->getConfig().get(windowMaximized))
	{
		setMaximized(true);
	}

	myRenderer.setWhitePixel(parentApplication->getWhitePixel());
	myRenderer.setConfig(&parentApplication->getConfig());
	getRootContainer().setRendererOverride(&myRenderer);
}

WOSInterface::~WOSInterface()
{
}
