#include <Client/GUI3/Interface.hpp>
#include <Client/GUI3/Widget.hpp>
#include <Client/Game/InputManager.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Joystick.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <utility>

namespace wos
{

InputManager::InputManager(gui3::Widget & inputTarget) :
	mouseMonitor(inputTarget),
	logger("InputManager")
{
	mouseCallback = inputTarget.addMouseCallback(
	    [=](gui3::MouseEvent event) {
		    handleMouseEvent(event);
	    },
	    gui3::MouseEvent::Any, -100);

	keyCallback = inputTarget.addKeyboardCallback(
	    [=](gui3::KeyEvent event) {
		    handleKeyEvent(event);
	    },
	    gui3::KeyEvent::Any, -100);

	controllerCallback = inputTarget.addControllerCallback(
	    [=](gui3::ControllerEvent event) {
		    handleControllerEvent(event);
	    },
	    gui3::ControllerEvent::Any, -100);

	tickCallback = inputTarget.addTickCallback(
	    [&]() {
		    framePressedKeys.clear();
		    for (auto & controller : controllers)
		    {
			    controller.framePressedButtons.clear();
		    }
		    textInput.clear();

		    // TODO replace this hack with a proper application-wide focus implementation
		    if (inputTarget.getParentInterface() != nullptr && !inputTarget.getParentInterface()->isFocused())
		    {
			    heldKeys.clear();
			    for (auto & controller : controllers)
			    {
				    controller.heldButtons.clear();
				    controller.axes.fill(0.f);
			    }
		    }

		    scrollAmount = sf::Vector2f();
	    },
	    100);

	stateCallback = inputTarget.addStateCallback(
	    [=](gui3::StateEvent event) {
		    if (event.type == gui3::StateEvent::FocusGained)
		    {
			    targetHasFocus = true;
			    logger.trace("Keyboard focus gained");
		    }
		    else
		    {
			    targetHasFocus = false;
			    logger.trace("Keyboard focus lost");
		    }
	    },
	    gui3::StateEvent::FocusGained | gui3::StateEvent::FocusLost);

	targetHasFocus = inputTarget.isFocused();
}

InputManager::~InputManager()
{
}

bool InputManager::hasMouseFocus() const
{
	return mouseMonitor.isMouseOver();
}

sf::Vector2f InputManager::getMousePosition() const
{
	return mouseMonitor.getMousePosition();
}

bool InputManager::isMouseButtonPressed(gui3::MouseEvent::Button button) const
{
	return mouseMonitor.isMouseDown(button);
}

sf::Vector2f InputManager::getScrollAmount() const
{
	return scrollAmount;
}

bool InputManager::hasKeyFocus() const
{
	return targetHasFocus;
}

const InputManager::Set<gui3::Key> & InputManager::getHeldKeys() const
{
	return heldKeys;
}

const InputManager::Set<gui3::Key> & InputManager::getCurrentFrameKeyPresses() const
{
	return framePressedKeys;
}

InputManager::Set<gui3::Key> InputManager::getPressedKeys() const
{
	auto keys = heldKeys;
	keys.insert(framePressedKeys.begin(), framePressedKeys.end());
	return keys;
}

std::vector<InputManager::KeyEvent> InputManager::pollKeyEvents()
{
	auto result = std::move(keyEvents);
	keyEvents.clear();
	return result;
}

const std::vector<sf::Uint32> & InputManager::getTextInput() const
{
	return textInput;
}

const InputManager::Set<InputManager::ControllerID> & InputManager::getConnectedControllers() const
{
	return controllerIDs;
}

bool InputManager::isControllerConnected(ControllerID id) const
{
	return controllerIDs.count(id);
}

unsigned int InputManager::getControllerSetupChangeCount() const
{
	return controllerSetupChangeCount;
}

const InputManager::ControllerInfo & InputManager::getControllerInfo(ControllerID id) const
{
	return getControllerState(id).info;
}

const InputManager::Set<InputManager::ControllerButtonID> &
InputManager::getHeldControllerButtons(ControllerID id) const
{
	return getControllerState(id).heldButtons;
}

const InputManager::Set<InputManager::ControllerButtonID> &
InputManager::getCurrentFrameControllerButtonPresses(ControllerID id) const
{
	return getControllerState(id).framePressedButtons;
}

const InputManager::ControllerAxes & InputManager::getControllerAxes(ControllerID id) const
{
	return getControllerState(id).axes;
}

InputManager::ControllerState & InputManager::getOrCreateControllerState(ControllerID id)
{
	if (id >= controllers.size())
	{
		controllers.resize(id + 1);
	}
	return controllers[id];
}

const InputManager::ControllerState & InputManager::getControllerState(ControllerID id) const
{
	if (id < controllers.size())
	{
		return controllers[id];
	}
	else
	{
		static ControllerState nullState;
		return nullState;
	}
}

void InputManager::cleanUpControllerState(ControllerState & state)
{
	state.axes.fill(0.f);
	state.heldButtons.clear();
	state.framePressedButtons.clear();
}

void InputManager::initControllerState(ControllerID id, ControllerState & state)
{
	if (!state.connected)
	{
		cleanUpControllerState(state);

		// TODO handle multiple active instances of same-identification controllers
		controllerIDs.insert(id);
		state.connected = true;
		controllerSetupChangeCount++;

		auto identification = sf::Joystick::getIdentification(id);
		state.info.name = identification.name;
		state.info.vendorID = identification.vendorId;
		state.info.productID = identification.productId;
	}
}

void InputManager::handleKeyEvent(gui3::KeyEvent event)
{
	switch (event.type)
	{
	case gui3::KeyEvent::Press:
		// Avoid key repeat events
		if (!heldKeys.count(event.key))
		{
			framePressedKeys.insert(event.key);
		}

		if (!heldKeys.count(event.key))
		{
			KeyEvent keyEvent;
			keyEvent.key = event.key;
			keyEvent.timestamp = event.timestamp;
			keyEvents.push_back(keyEvent);

			heldKeys.insert(event.key);
		}

		// Allow pasting text via Ctrl-V
		if (gui3::Key::toSFML(event.key) == sf::Keyboard::V && isControlPressed() && !isAltPressed())
		{
			sf::String clipboardData = sf::Clipboard::getString();
			textInput.insert(textInput.end(), clipboardData.begin(), clipboardData.end());
		}
		break;

	case gui3::KeyEvent::Release:
	{
		KeyEvent keyEvent;
		keyEvent.release = true;
		keyEvent.key = event.key;
		keyEvent.timestamp = event.timestamp;
		keyEvents.push_back(keyEvent);

		heldKeys.erase(event.key);
		break;
	}

	case gui3::KeyEvent::Input:
		if (event.character == '\r')
		{
			// Convert carriage return to newline
			textInput.push_back('\n');
		}
		else if (event.character != '\n' || textInputLatest != '\r')
		{
			// Skip any LF immediately following a CR character; add other characters as normal
			textInput.push_back(event.character);
		}
		textInputLatest = event.character;
		break;

	default:
		break;
	}
}

void InputManager::handleControllerEvent(gui3::ControllerEvent event)
{
	auto & controllerState = getOrCreateControllerState(event.controller);

	switch (event.type)
	{
	case gui3::ControllerEvent::Connect:
		initControllerState(event.controller, controllerState);
		break;

	case gui3::ControllerEvent::Disconnect:
		controllerSetupChangeCount++;
		controllerIDs.erase(event.controller);
		controllerState.connected = false;
		cleanUpControllerState(controllerState);
		break;

	case gui3::ControllerEvent::ButtonPress:
		initControllerState(event.controller, controllerState);
		controllerState.heldButtons.insert(event.button);
		controllerState.framePressedButtons.insert(event.button);
		break;

	case gui3::ControllerEvent::ButtonRelease:
		initControllerState(event.controller, controllerState);
		controllerState.heldButtons.erase(event.button);
		break;

	case gui3::ControllerEvent::AxisMove:
		initControllerState(event.controller, controllerState);
		if (event.button < controllerState.axes.size())
		{
			controllerState.axes[event.button] = event.axisValue;
		}
		break;

	default:
		break;
	}
}

bool InputManager::isControlPressed() const
{
	return heldKeys.count(gui3::Key::fromSFML(sf::Keyboard::LControl))
	       || heldKeys.count(gui3::Key::fromSFML(sf::Keyboard::RControl));
}

bool InputManager::isAltPressed() const
{
	return heldKeys.count(gui3::Key::fromSFML(sf::Keyboard::LAlt))
	       || heldKeys.count(gui3::Key::fromSFML(sf::Keyboard::RAlt));
}

void InputManager::handleMouseEvent(gui3::MouseEvent event)
{
	switch (event.type)
	{
	case gui3::MouseEvent::ScrollX:
		scrollAmount.x += event.scrollAmount;
		break;

	case gui3::MouseEvent::ScrollY:
		scrollAmount.y += event.scrollAmount;
		break;

	default:
		break;
	}
}

}
