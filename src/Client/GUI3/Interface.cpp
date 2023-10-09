/*
 * Interface.cpp
 *
 *  Created on: Jun 15, 2015
 *      Author: marukyu
 */

#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/Events/MouseEvent.hpp>
#include <Client/GUI3/Interface.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/Timer.hpp>
#include <cmath>
#include <limits>
#include <vector>

#ifdef WOS_WINDOWS
#	include <windows.h>
#elif defined(WOS_LINUX)
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#	undef ButtonPress
#	undef ButtonRelease
#endif
#include <GL/gl.h>

using namespace gui3;

Interface::~Interface()
{
	closeWindow();
}

Panel & Interface::getRootContainer()
{
	return myRootContainer;
}

const Panel & Interface::getRootContainer() const
{
	return myRootContainer;
}

Application & Interface::getParentApplication() const
{
	return *myParentApplication;
}

void Interface::setTitle(sf::String title)
{
	myWindowTitle = title;
	myWindow.setTitle(title);
}

sf::String Interface::getTitle() const
{
	return myWindowTitle;
}

void Interface::resize(sf::Vector2u size, bool fullscreen)
{
	if (myWindow.isOpen())
	{
		// If window is or will be fullscreen, recreate it.
		if (myIsFullscreen || fullscreen)
		{
			myWindow.close();
			myHasFocus = true;
			myWindowSize = size;
			myIsFullscreen = fullscreen;
			openWindow();
		}
		else
		{
			myWindow.setSize(size);
		}

		myWindowSize = myWindow.getSize();

		// Immediately resize root container
		myRootContainer.setSize(sf::Vector2f(myWindowSize));
	}
	else
	{
		// Assign variables for future window creation.
		myWindowSize = size;
		myIsFullscreen = fullscreen;
	}

	if (!fullscreen)
	{
		myNonFullscreenSize = myWindowSize;
	}
}

sf::Vector2i Interface::getPosition() const
{
	return myWindowPosition;
}

sf::Vector2u Interface::getSize() const
{
	return myWindowSize;
}

sf::Vector2u Interface::getWindowedSize() const
{
	return myNonFullscreenSize;
}

bool Interface::isFullscreen() const
{
	return myIsFullscreen;
}

bool Interface::isFocused() const
{
	return myHasFocus;
}

void Interface::setUseSystemCursor(bool visible)
{
	if (myIsMouseCursorVisible != visible)
	{
		myIsMouseCursorVisible = visible;
		myWindow.setMouseCursorVisible(myIsMouseCursorVisible);
	}
}

bool Interface::isUsingSystemCursor() const
{
	return myIsMouseCursorVisible;
}

void Interface::setIcon(const sf::Image & icon)
{
	myWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
}

void Interface::setMaximized(bool maximized)
{
	if (!isWindowOpen())
	{
		myNeedMaximize = maximized;
		return;
	}

	if (myIsFullscreen)
	{
		return;
	}

#ifdef WOS_WINDOWS
	if (isMaximized() != maximized)
	{
		ShowWindow(myWindow.getSystemHandle(), maximized ? SW_MAXIMIZE : SW_RESTORE);
	}
	else
	{
		return;
	}
#elif defined(WOS_LINUX)
	XEvent xev = {0};
	Display * display = XOpenDisplay(nullptr);
	if (!display)
	{
		return;
	}
	Atom windowManagerState = XInternAtom(display, "_NET_WM_STATE", false);
	Atom maximizedHorizontal = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	Atom maximizedVertical = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);

	if (windowManagerState && maximizedHorizontal && maximizedVertical)
	{
		xev.type = ClientMessage;
		xev.xclient.window = myWindow.getSystemHandle();
		xev.xclient.message_type = windowManagerState;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = maximized;
		xev.xclient.data.l[1] = maximizedHorizontal;
		xev.xclient.data.l[2] = maximizedVertical;

		XSendEvent(display, DefaultRootWindow(display), false, SubstructureNotifyMask, &xev);
	}

	XCloseDisplay(display);
#else
	// TODO implement maximization on osx
	return;
#endif

	myWindowSize = myWindow.getSize();
	myNonFullscreenSize = myWindowSize;
	myRootContainer.setSize(sf::Vector2f(myWindowSize));
}

bool Interface::isMaximized() const
{
	if (!myWindow.isOpen())
	{
		return myNeedMaximize;
	}

#ifdef WOS_WINDOWS
	WINDOWPLACEMENT placement;
	placement.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(myWindow.getSystemHandle(), &placement);
	return placement.showCmd == SW_MAXIMIZE || placement.showCmd == SW_SHOWMAXIMIZED;
#elif defined(WOS_LINUX)
	XEvent xev = {0};
	Display * display = XOpenDisplay(nullptr);

	bool maximized = false;

	if (!display)
	{
		return maximized;
	}
	Atom windowManagerState = XInternAtom(display, "_NET_WM_STATE", false);
	Atom maximizedHorizontal = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);

	if (windowManagerState && maximizedHorizontal)
	{
		long maxLength = 1024;
		Atom type = 0;
		int format = 0;
		unsigned long partialReadLength = 0;
		unsigned long stateCount = 0;
		Atom * states = 0;

		if (XGetWindowProperty(display, myWindow.getSystemHandle(), windowManagerState, 0l, maxLength, false, XA_ATOM,
		        &type, &format, &stateCount, &partialReadLength, reinterpret_cast<unsigned char **>(&states))
		    == Success)
		{
			for (unsigned long i = 0; i < stateCount; ++i)
			{
				if (states[i] == maximizedHorizontal)
				{
					maximized = true;
					break;
				}
			}
		}
	}

	XCloseDisplay(display);
	return maximized;

#else
	// TODO implement maximization on osx
	return false;
#endif
}

bool Interface::isMinimized() const
{
#ifdef WOS_WINDOWS
	WINDOWPLACEMENT placement;
	placement.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(myWindow.getSystemHandle(), &placement);
	return placement.showCmd == SW_MINIMIZE || placement.showCmd == SW_SHOWMINIMIZED;
#else
	// TODO implement minimization on linux/osx
	return !isFocused();
#endif
}

void Interface::display()
{
	myWindow.clear();
	myWindow.setView(sf::View(sf::FloatRect(0, 0, getSize().x, getSize().y)));

	//glEnable(0x9346); // conservative rasterization (NV)

	sf::RenderStates states;
	states.texture = getParentApplication().getMainTexture();
	myRootContainer.onRender(myWindow, states);

	onRender();

	myWindow.display();
}

Interface::Interface(Application * parentApplication) :
    myRootContainer(this),
    myParentApplication(parentApplication),
    myWindowSize(640, 480),
    myNonFullscreenSize(640, 480),
    myWindowTitle("WOS Application")
{
}

sf::RenderTarget & Interface::getRenderTarget()
{
	return myWindow;
}

const sf::RenderTarget & Interface::getRenderTarget() const
{
	return myWindow;
}

bool Interface::isWindowOpen() const
{
	return myWindow.isOpen();
}

void Interface::openWindow()
{
	sf::ContextSettings context;
	// context.antialiasingLevel = 4;

	if (myIsFullscreen)
	{
		myWindow.create(findGoodVideoMode(myWindowSize), myWindowTitle, sf::Style::Fullscreen, context);
	}
	else
	{
		myWindow.create(sf::VideoMode(myWindowSize.x, myWindowSize.y), myWindowTitle, sf::Style::Default, context);

		if (myNeedMaximize)
		{
			setMaximized(true);
			myNeedMaximize = false;
		}
	}

	myWindowPosition = myWindow.getPosition();
	myWindowSize = myWindow.getSize();

	myWindow.setMouseCursorVisible(myIsMouseCursorVisible);
	myWindow.setVerticalSyncEnabled(myIsVSyncEnabled);
}

void Interface::closeWindow()
{
	if (isWindowOpen())
	{
		myRootContainer.fireStateEvent(StateEvent(StateEvent::WindowClosed));
		getParentApplication().cleanUpWindowResources();
		myWindow.close();
	}
}

void Interface::setVerticalSyncEnabled(bool enabled)
{
	if (myIsVSyncEnabled != enabled)
	{
		myIsVSyncEnabled = enabled;

		if (isWindowOpen())
		{
			myWindow.setVerticalSyncEnabled(enabled);
		}
	}
}

bool Interface::isVerticalSyncEnabled() const
{
	return myIsVSyncEnabled;
}

void Interface::process()
{
	myWindowPosition = myWindow.getPosition();

	for (sf::Event event; myWindow.pollEvent(event);)
	{
		processEvent(event);

		if (!isWindowOpen())
		{
			return;
		}
	}

	myRootContainer.setClippingWidgets(false);
	myRootContainer.setSize(sf::Vector2f(getSize()));

	myRootContainer.fireTick();

	if (myRootContainer.isRepaintNeeded())
	{
		myRootContainer.performRepaint();
	}

	display();
}

void Interface::processEvent(const sf::Event & event)
{
	switch (event.type)
	{
	case sf::Event::MouseMoved:
		myRootContainer.fireMouseEvent(
		    MouseEvent::generatePositionalEvent(MouseEvent::Move, sf::Vector2f(event.mouseMove.x, event.mouseMove.y)));
		break;

	case sf::Event::MouseEntered:
		myRootContainer.fireMouseEvent(
		    MouseEvent::generatePositionalEvent(MouseEvent::Enter, sf::Vector2f(sf::Mouse::getPosition(myWindow))));
		break;

	case sf::Event::MouseLeft:
		myRootContainer.fireMouseEvent(
		    MouseEvent::generatePositionalEvent(MouseEvent::Leave, sf::Vector2f(sf::Mouse::getPosition(myWindow))));
		break;

	case sf::Event::MouseButtonPressed:
		myRootContainer.fireMouseEvent(MouseEvent::generateButtonEvent(
		    MouseEvent::ButtonDown, sf::Vector2f(event.mouseButton.x, event.mouseButton.y),
		    MouseEvent::getButtonConstantFromSFML(event.mouseButton.button)));
		break;

	case sf::Event::MouseButtonReleased:
		myRootContainer.fireMouseEvent(MouseEvent::generateButtonEvent(
		    MouseEvent::ButtonUp, sf::Vector2f(event.mouseButton.x, event.mouseButton.y),
		    MouseEvent::getButtonConstantFromSFML(event.mouseButton.button)));
		break;

	case sf::Event::MouseWheelScrolled:
	{
		myRootContainer.fireMouseEvent(MouseEvent::generateScrollEvent(
		    MouseEvent::getScrollTypeConstantFromSFML(event.mouseWheelScroll.wheel),
		    sf::Vector2f(event.mouseWheelScroll.x, event.mouseWheelScroll.y), event.mouseWheelScroll.delta));
		break;
	}

	case sf::Event::KeyPressed:
		myRootContainer.fireKeyboardEvent(handleKeyEvent(event, KeyEvent::Press));
		break;

	case sf::Event::KeyReleased:
		myRootContainer.fireKeyboardEvent(handleKeyEvent(event, KeyEvent::Release));
		break;

	case sf::Event::TextEntered:
		myRootContainer.fireKeyboardEvent(KeyEvent(KeyEvent::Input, event.text.unicode, Timer::getGlobalTime()));
		break;

	case sf::Event::JoystickConnected:
		myRootContainer.fireControllerEvent(
		    ControllerEvent(ControllerEvent::Connect, event.joystickConnect.joystickId));
		break;

	case sf::Event::JoystickDisconnected:
		myRootContainer.fireControllerEvent(
		    ControllerEvent(ControllerEvent::Disconnect, event.joystickConnect.joystickId));
		break;

	case sf::Event::JoystickButtonPressed:
		myRootContainer.fireControllerEvent(ControllerEvent(
		    ControllerEvent::ButtonPress, event.joystickButton.joystickId, event.joystickButton.button));
		break;

	case sf::Event::JoystickButtonReleased:
		myRootContainer.fireControllerEvent(ControllerEvent(
		    ControllerEvent::ButtonRelease, event.joystickButton.joystickId, event.joystickButton.button));
		break;

	case sf::Event::JoystickMoved:
		myRootContainer.fireControllerEvent(ControllerEvent(ControllerEvent::AxisMove, event.joystickMove.joystickId,
		                                                    event.joystickMove.axis, event.joystickMove.position));
		break;

	case sf::Event::Closed:
		closeWindow();
		break;

	case sf::Event::GainedFocus:
		myHasFocus = true;
		break;

	case sf::Event::LostFocus:
		myHasFocus = false;
		break;

	case sf::Event::Resized:
		myWindowSize.x = event.size.width;
		myWindowSize.y = event.size.height;
		break;

	default:
		break;
	}

	// Allow derived interface to process further events.
	onEvent(event);
}

KeyEvent Interface::handleKeyEvent(const sf::Event & event, KeyEvent::Type type) const
{
	sf::Time timestamp = Timer::getGlobalTime();
	Key key = Key::fromSFML(event.key.code);

#ifdef SFML_HAS_KEY_EVENT_FIELD_TIMESTAMP
	// Check if this version of SFML supports accurate key event timestamps
	timestamp += sf::microseconds(event.key.timestamp) - myWindow.getTimestamp();
#endif

#ifdef SFML_HAS_KEY_EVENT_FIELD_NATIVE
	// Check if this version of SFML supports native IDs for "unknown" keys
	if (event.key.code == sf::Keyboard::Unknown)
	{
		key = Key::fromNative(event.key.native);
	}
#endif

	return KeyEvent(type, key, timestamp);
}

void Interface::onEvent(const sf::Event & event)
{
}

void Interface::onRender()
{
}

Interface::RootContainer::RootContainer(Interface * parentInterface)
{
	myParentInterface = parentInterface;
	setClippingWidgets(false);
}

Interface::RootContainer::~RootContainer()
{
}

Interface * Interface::RootContainer::getParentInterface() const
{
	return myParentInterface;
}

sf::VideoMode Interface::findGoodVideoMode(sf::Vector2u compare)
{
	const std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();

	if (modes.empty())
	{
		return sf::VideoMode::getDesktopMode();
	}

	sf::VideoMode bestMode = sf::VideoMode::getDesktopMode();

	// Mode "quality", difference between attempted and available mode. The lower, the better.
	int bestModeQuality = std::numeric_limits<int>::max();

	for (unsigned int i = 0; i < modes.size(); ++i)
	{
		const int curModeQuality = std::abs((int) modes[i].width - (int) compare.x)
		                           + std::abs((int) modes[i].height - (int) compare.y) - modes[i].bitsPerPixel;

		if (curModeQuality < bestModeQuality)
		{
			bestMode = modes[i];
			bestModeQuality = curModeQuality;
		}
	}

	return bestMode;
}
