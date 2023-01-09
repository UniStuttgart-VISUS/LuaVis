#ifndef SRC_CLIENT_GAME_INPUTMANAGER_HPP_
#define SRC_CLIENT_GAME_INPUTMANAGER_HPP_

#include <Client/GUI3/Events/ControllerEvent.hpp>
#include <Client/GUI3/Events/Key.hpp>
#include <Client/GUI3/Events/KeyEvent.hpp>
#include <Client/GUI3/Events/MouseEvent.hpp>
#include <Client/GUI3/Events/StateEvent.hpp>
#include <Client/GUI3/Utils/MouseMonitor.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <array>
#include <set>

namespace gui3
{
class Widget;
}

namespace wos
{

class InputManager
{
public:
	using ControllerID = sf::Uint32;
	using ControllerButtonID = sf::Uint32;

	template <typename T>
	using Set = std::set<T>;

	struct ControllerInfo
	{
		// Descriptive name for this controller.
		std::string name;

		// Unique identifier for this controller's manufacturer.
		sf::Uint32 vendorID = 0;

		// Unique identifier for this controller's hardware.
		sf::Uint32 productID = 0;

		// If multiple instances of the same type of controller are connected, this distinguishes them.
		sf::Uint32 instanceID = 0;
	};

	struct KeyEvent
	{
		gui3::Key key;
		bool release = false;
		sf::Time timestamp;
	};

	using ControllerAxes = std::array<float, 8>;

	/**
	 * Creates an input manager and binds it to the specified widget, listening for mouse and keyboard events.
	 */
	InputManager(gui3::Widget & inputTarget);

	/**
	 * Virtual destructor.
	 */
	virtual ~InputManager();

	/**
	 * Returns true if the target widget has mouse focus, false otherwise.
	 */
	bool hasMouseFocus() const;

	/**
	 * Returns the mouse position relative to the target widget.
	 */
	sf::Vector2f getMousePosition() const;

	/**
	 * Returns true if the specified mouse button is pressed, false otherwise.
	 */
	bool isMouseButtonPressed(gui3::MouseEvent::Button button) const;

	/**
	 * Returns the distance scrolled horizontally/vertically in the current frame.
	 */
	sf::Vector2f getScrollAmount() const;

	/**
	 * Returns true if the target widget has keyboard focus, false otherwise.
	 */
	bool hasKeyFocus() const;

	/**
	 * Returns all keys that are currently held down.
	 */
	const Set<gui3::Key> & getHeldKeys() const;

	/**
	 * Returns all keys that were just pressed in the current frame.
	 */
	const Set<gui3::Key> & getCurrentFrameKeyPresses() const;

	/**
	 * Returns all keys that are currently held OR were pressed in the current frame.
	 *
	 * This ensures that all keypresses are returned for at least one frame.
	 */
	Set<gui3::Key> getPressedKeys() const;

	/**
	 * Returns all keys that were pressed or released since the last call to this function.
	 */
	std::vector<KeyEvent> pollKeyEvents();

	/**
	 * Returns all text that has been entered in the current frame.
	 */
	const std::vector<sf::Uint32> & getTextInput() const;

	/**
	 * Returns the IDs of all connected controllers.
	 */
	const Set<ControllerID> & getConnectedControllers() const;

	/**
	 * Returns identifying information about the specified controller.
	 */
	bool isControllerConnected(ControllerID id) const;

	/**
	 * Returns the number of times the controller setup has changed (controller added or removed).
	 */
	unsigned int getControllerSetupChangeCount() const;

	/**
	 * Returns identifying information about the specified controller.
	 */
	const ControllerInfo & getControllerInfo(ControllerID id) const;

	/**
	 * Returns all controller buttons currently pressed on the specified controller.
	 */
	const Set<ControllerButtonID> & getHeldControllerButtons(ControllerID id) const;

	/**
	 * Returns all controller buttons that were just pressed in the current frame on the specified controller.
	 */
	const Set<ControllerButtonID> & getCurrentFrameControllerButtonPresses(ControllerID id) const;

	/**
	 * Returns the controller's current axis state.
	 */
	const ControllerAxes & getControllerAxes(ControllerID id) const;

private:
	struct ControllerState
	{
		ControllerInfo info;
		bool connected = false;
		Set<ControllerButtonID> heldButtons;
		Set<ControllerButtonID> framePressedButtons;
		ControllerAxes axes;
	};

	ControllerState & getOrCreateControllerState(ControllerID id);
	const ControllerState & getControllerState(ControllerID id) const;

	void cleanUpControllerState(ControllerState & state);
	void initControllerState(ControllerID id, ControllerState & state);

	void handleMouseEvent(gui3::MouseEvent event);
	void handleKeyEvent(gui3::KeyEvent event);
	void handleControllerEvent(gui3::ControllerEvent event);

	bool isControlPressed() const;
	bool isAltPressed() const;

	gui3::MouseMonitor mouseMonitor;
	bool targetHasFocus = false;

	Set<gui3::Key> heldKeys;
	Set<gui3::Key> framePressedKeys;
	std::vector<KeyEvent> keyEvents;
	sf::Vector2f scrollAmount;

	// TODO use an event-based approach instead
	unsigned int controllerSetupChangeCount = 0;
	Set<ControllerID> controllerIDs;
	std::vector<ControllerState> controllers;

	Callback<> tickCallback;
	Callback<gui3::MouseEvent> mouseCallback;
	Callback<gui3::KeyEvent> keyCallback;
	Callback<gui3::ControllerEvent> controllerCallback;
	Callback<gui3::StateEvent> stateCallback;

	std::vector<sf::Uint32> textInput;
	sf::Uint32 textInputLatest = 0;

	Logger logger;
};

}

#endif
