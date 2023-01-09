#ifndef SRC_CLIENT_GUI3_EVENTS_TOUCHEVENT_HPP_
#define SRC_CLIENT_GUI3_EVENTS_TOUCHEVENT_HPP_

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

namespace gui3
{

class TouchEvent
{
public:
	using Finger = int;

	enum Type
	{
		/**
		 * Empty event mask.
		 */
		None = 0,

		/**
		 * Called whenever a finger begins touching the widget.
		 */
		Press = 1 << 0,

		/**
		 * Called whenever a finger that began its touch on the widget is released.
		 */
		Release = 1 << 1,

		/**
		 * Called whenever a finger beginning its touch on the widget is moved.
		 */
		Move = 1 << 2,

		/**
		 * Called whenever a finger begins and ends its touch without waiting too long or moving too far.
		 */
		Tap = 1 << 3,

		/**
		 * Called whenever a finger is pressed down for some time without moving too far.
		 */
		LongPress = 1 << 4,

		/**
		 * Full event mask (for event forwarding).
		 */
		Any = 0x7fffffff
	};

	TouchEvent(Type type, Finger finger, sf::Vector2f position, sf::Vector2f origin, sf::Time duration) :
		type(type),
		finger(finger),
		position(position),
		origin(origin),
		duration(duration)
	{
	}

	TouchEvent(const TouchEvent & event, sf::Vector2f position) :
		type(event.type),
		finger(event.finger),
		position(position),
		origin(event.origin),
		duration(event.duration)
	{
	}

	const Type type;
	const Finger finger;
	const sf::Vector2f position;
	const sf::Vector2f origin;
	const sf::Time duration;
};

}

#endif
