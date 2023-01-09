#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/Interface.hpp>
#include <Client/GUI3/Widget.hpp>
#include <Client/GUI3/Widgets/Misc/Window.hpp>
#include <Client/Lua/Bridges/WindowBridge.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <Sol2/sol.hpp>
#include <cstddef>
#include <exception>
#include <functional>
#include <vector>

namespace lua
{

WindowBridge::WindowBridge(gui3::Widget & widget) :
	widget(widget),
	logger("WindowBridge")
{
}

WindowBridge::~WindowBridge()
{
}

void WindowBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("window.getWidth", std::function<int()>([=]() -> int {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->getSize().x;
		            }
		            else
		            {
			            logger.warn("Attempt to query window size without a connected interface");
			            return 0;
		            }
	            }));

	loader.bind("window.getHeight", std::function<int()>([=]() -> int {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->getSize().y;
		            }
		            else
		            {
			            logger.warn("Attempt to query window size without a connected interface");
			            return 0;
		            }
	            }));

	loader.bind("window.getWidthWindowed",
	    std::function<int()>(
	        [=]() -> int
	        {
		        gui3::Interface * interface = widget.getParentInterface();
		        if (interface)
		        {
			        return interface->getWindowedSize().x;
		        }
		        else
		        {
			        logger.warn("Attempt to query window size without a connected interface");
			        return 0;
		        }
	        }));

	loader.bind("window.getHeightWindowed",
	    std::function<int()>(
	        [=]() -> int
	        {
		        gui3::Interface * interface = widget.getParentInterface();
		        if (interface)
		        {
			        return interface->getWindowedSize().y;
		        }
		        else
		        {
			        logger.warn("Attempt to query window size without a connected interface");
			        return 0;
		        }
	        }));

	loader.bind("window.isFullscreen", std::function<bool()>([=]() {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->isFullscreen();
		            }
		            else
		            {
			            logger.warn("Attempt to query fullscreen state without a connected interface");
			            return false;
		            }
	            }));

	loader.bind("window.setMaximized", std::function<void(bool)>([=](bool maximized) {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->setMaximized(maximized);
		            }
		            else
		            {
			            logger.warn("Attempt to change maximization state without a connected interface");
		            }
	            }));

	loader.bind("window.isMaximized", std::function<bool()>([=]() {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->isMaximized();
		            }
		            else
		            {
			            logger.warn("Attempt to query maximization state without a connected interface");
			            return false;
		            }
	            }));

	loader.bind("window.isFocused", std::function<bool()>([=]() {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            return interface->isFocused();
		            }
		            else
		            {
			            logger.warn("Attempt to query focus state without a connected interface");
			            return false;
		            }
	            }));

	loader.bind("window.isMinimized", //
	    std::function<bool()>(
	        [=]()
	        {
		        gui3::Interface * interface = widget.getParentInterface();
		        if (interface)
		        {
			        return interface->isMinimized();
		        }
		        else
		        {
			        return false;
		        }
	        }));

	loader.bind("window.getDesktopMode", std::function<std::pair<int, int>()>([=]() {
		            return std::make_pair<int, int>(sf::VideoMode::getDesktopMode().width,
		                                            sf::VideoMode::getDesktopMode().height);
	            }));

	loader.bind("window.changeMode", std::function<void(int, int, bool)>([=](int width, int height, bool fullscreen) {
		            gui3::Interface * interface = widget.getParentInterface();
		            if (interface)
		            {
			            if (width <= 0 || height <= 0)
			            {
				            logger.warn("Attempt to change window to invalid size {}x{}", width, height);
			            }
			            else
			            {
				            interface->resize(sf::Vector2u(width, height), fullscreen);
			            }
		            }
		            else
		            {
			            logger.warn("Attempt to change window mode without a connected interface");
		            }
	            }));

	loader.bind("window.listVideoModes",
	            std::function<sol::object(sol::this_state)>([=](sol::this_state luaState) -> sol::object {
		            try
		            {
			            sol::state_view state(luaState);
			            auto table = state.create_table();

			            const auto & modes = sf::VideoMode::getFullscreenModes();
			            for (std::size_t i = 0; i < modes.size(); ++i)
			            {
				            const auto & mode = modes[i];
				            auto modeTable = state.create_table();
				            modeTable["width"] = mode.width;
				            modeTable["height"] = mode.height;
				            table[i + 1] = modeTable;
			            }

			            return table;
		            }
		            catch (std::exception & ex)
		            {
			            logger.error("Error listing video modes: {}", ex.what());
			            return sol::lua_nil;
		            }
	            }));

	loader.bind("window.getFramerateLimit", std::function<int()>([=]() -> int {
		            gui3::Application * application = widget.getParentApplication();
		            if (application)
		            {
			            return application->getFramerateLimit();
		            }
		            else
		            {
			            logger.warn("Attempt to query framerate limit without a connected interface");
			            return 0;
		            }
	            }));

	loader.bind("window.setFramerateLimit", std::function<void(int)>([=](int limit) {
		            gui3::Application * application = widget.getParentApplication();
		            if (application)
		            {
			            logger.trace("Changing framerate limit to {}", limit);
			            application->setFramerateLimit(limit);
		            }
		            else
		            {
			            logger.warn("Attempt to change framerate limit without a connected interface");
		            }
	            }));

	loader.bind("window.setAspectRatio", std::function<void(sol::optional<double>)>([=](sol::optional<double> ratio) {
		            auto * parent = &widget;
		            while (parent)
		            {
			            if (gui3::Window * window = dynamic_cast<gui3::Window *>(parent))
			            {
				            if (ratio)
				            {
					            window->setAspectRatio(*ratio);
					            window->setProportional(true);
					            logger.trace("Setting aspect radio to {}", *ratio);
				            }
				            else
				            {
					            window->setProportional(false);
					            logger.trace("Setting aspect radio to none");
				            }
				            return;
			            }
			            parent = parent->getParent();
		            }
		            logger.warn("Failed to find window to modify aspect ratio");
	            }));

	loader.bind("window.setTitle", std::function<void(std::string)>([=](std::string title) {
		            if (auto * parentInterface = widget.getParentInterface())
		            {
			            parentInterface->setTitle(title);
		            }
	            }));

	loader.bind("window.setClipboard", std::function<void(std::string)>(
	                                       [=](std::string text)
	                                       {
		                                       sf::Clipboard::setString(sf::String::fromUtf8(text.begin(), text.end()));
	                                       }));

	loader.bind("window.getClipboard", std::function<std::string()>(
	                                       [=]()
	                                       {
		                                       auto result = sf::Clipboard::getString().toUtf8();
		                                       return std::string(result.begin(), result.end());
	                                       }));

	loader.bind("window.setCursorVisible", // Controls cursor visibility
	    std::function<void(bool)>(
	        [=](bool visible)
	        {
		        if (auto * parentInterface = widget.getParentInterface())
		        {
			        parentInterface->setUseSystemCursor(visible);
		        }
	        }));
}

}
