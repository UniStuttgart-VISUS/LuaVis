#include <Client/GUI3/Events/Key.hpp>
#include <Client/GUI3/Events/KeyNative.hpp>
#include <Client/GUI3/Events/MouseEvent.hpp>
#include <Client/Lua/Bridges/InputBridge.hpp>
#include <Shared/Lua/LuaUtils.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Sol2/sol.hpp>
#include <exception>
#include <functional>
#include <vector>

namespace lua
{

InputBridge::InputBridge(wos::InputManager & manager) :
	manager(manager),
	logger("InputBridge")
{
}

InputBridge::~InputBridge()
{
}

void InputBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("input.getKeyboardLookupTable", //
	    std::function<sol::table(sol::this_state)>(
	        [=](sol::this_state luaState) -> sol::object
	        {
		        sol::state_view state(luaState);
		        auto table = state.create_table();
		        for (int i = -1; i < 10000; ++i)
		        {
			        auto key = gui3::Key::fromInt32(i);
			        auto keyName = gui3::Key::toString(key);
			        if (keyName.empty())
			        {
				        break;
			        }
			        else
			        {
				        table[gui3::Key::toLua(key)] = keyName;
			        }
		        }
		        return table;
	        }));

	loader.bind("input.getNativeKeyName", //
	    std::function<std::string(int)>(
	        [=](int id)
	        {
		        return gui3::detail::getNativeKeyName(gui3::Key::toNative(gui3::Key::fromLua(id)));
	        }));

	loader.bind("input.getInput",
	            std::function<sol::object(sol::this_state)>([=](sol::this_state luaState) -> sol::object {
		            try
		            {
			            sol::state_view state(luaState);
			            auto table = state.create_table();

			            table["mouse"] = state.create_table();
			            table["keys"] = state.create_table();
			            table["controllers"] = state.create_table();
			            table["text"] = lua::listToTable(manager.getTextInput(), state);

			            table["mouse"]["focus"] = manager.hasMouseFocus();

			            table["mouse"]["x"] = manager.getMousePosition().x;
			            table["mouse"]["y"] = manager.getMousePosition().y;

			            table["mouse"]["scrollX"] = manager.getScrollAmount().x;
			            table["mouse"]["scrollY"] = manager.getScrollAmount().y;

			            table["mouse"]["left"] = manager.isMouseButtonPressed(gui3::MouseEvent::Left);
			            table["mouse"]["right"] = manager.isMouseButtonPressed(gui3::MouseEvent::Right);
			            table["mouse"]["middle"] = manager.isMouseButtonPressed(gui3::MouseEvent::Middle);
			            table["mouse"]["extra1"] = manager.isMouseButtonPressed(gui3::MouseEvent::Extra1);
			            table["mouse"]["extra2"] = manager.isMouseButtonPressed(gui3::MouseEvent::Extra2);

			            for (const auto & key : manager.getPressedKeys())
			            {
				            if (!key.isUnknown())
				            {
					            table["keys"][gui3::Key::toLua(key)] = true;
				            }
			            }

			            auto eventTable = state.create_table();
			            std::size_t eventIndex = 1;
			            for (const auto & event : manager.pollKeyEvents())
			            {
				            if (!event.key.isUnknown())
				            {
					            eventTable[eventIndex++] =
					                static_cast<double>(gui3::Key::toLua(event.key) * (event.release ? -1 : 1));
					            eventTable[eventIndex++] = event.timestamp.asMicroseconds() / 1000000.0;
				            }
			            }
			            table["keyEvents"] = eventTable;

			            for (auto controllerID : manager.getConnectedControllers())
			            {
				            auto controllerTable = state.create_table();
				            controllerTable["buttons"] = state.create_table();
				            controllerTable["buttonPresses"] = state.create_table();
				            controllerTable["axes"] = state.create_table(8);

				            for (const auto & button : manager.getHeldControllerButtons(controllerID))
				            {
					            controllerTable["buttons"][button + 1] = true;
				            }

				            for (const auto & button : manager.getCurrentFrameControllerButtonPresses(controllerID))
				            {
					            controllerTable["buttonPresses"][button + 1] = true;
				            }

				            auto axes = manager.getControllerAxes(controllerID);
				            for (std::size_t i = 0; i < axes.size(); ++i)
				            {
					            controllerTable["axes"][i + 1] = axes[i];
				            }

				            table["controllers"][controllerID] = controllerTable;
			            }

			            return table;
		            }
		            catch (std::exception & ex)
		            {
			            logger.error("Error getting input: {}", ex.what());
			            return sol::lua_nil;
		            }
	            }));

	loader.bind("input.controllerSetupChanged", std::function<bool()>([=]() {
		            // TODO: replace this with a nicer event-based approach sometime
		            if (lastControllerSetupChangeCount != manager.getControllerSetupChangeCount())
		            {
			            lastControllerSetupChangeCount = manager.getControllerSetupChangeCount();
			            return true;
		            }
		            else
		            {
			            return false;
		            }
	            }));

	loader.bind("input.getConnectedControllers",
	            std::function<sol::object(sol::this_state)>([=](sol::this_state luaState) -> sol::object {
		            try
		            {
			            sol::state_view state(luaState);
			            auto resultTable = state.create_table();

			            for (auto controllerID : manager.getConnectedControllers())
			            {
				            auto controllerTable = state.create_table();
				            const auto & info = manager.getControllerInfo(controllerID);
				            controllerTable["name"] = info.name;
				            controllerTable["vendorID"] = info.vendorID;
				            controllerTable["productID"] = info.productID;
				            controllerTable["instanceID"] = info.instanceID;
				            resultTable[controllerID] = controllerTable;
			            }

			            return resultTable;
		            }
		            catch (std::exception & ex)
		            {
			            logger.error("Error getting connected controllers: {}", ex.what());
			            return sol::lua_nil;
		            }
	            }));
}

}
