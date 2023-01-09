#ifndef SRC_CLIENT_GUI3_EVENTS_CONTROLLEREVENT_HPP_
#define SRC_CLIENT_GUI3_EVENTS_CONTROLLEREVENT_HPP_

namespace gui3
{

class ControllerEvent
{
public:
	using ControllerID = unsigned int;
	using ButtonID = unsigned int;

	enum Type
	{
		None = 0,
		Connect = 1 << 0,
		Disconnect = 1 << 1,
		ButtonPress = 1 << 2,
		ButtonRelease = 1 << 3,
		AxisMove = 1 << 4,

		/**
		 * Full event mask (for event forwarding).
		 */
		Any = 0x7fffffff
	};

	ControllerEvent(Type type, ControllerID controller, ButtonID button = 0, float axisValue = 0.f) :
		type(type),
		controller(controller),
		button(button),
		axisValue(axisValue)
	{
	}

	const Type type;
	const ControllerID controller;
	const ButtonID button;
	const float axisValue;
};

}

#endif
