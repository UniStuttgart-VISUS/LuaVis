#ifndef SRC_CLIENT_GUI3_EVENTS_KEY_HPP_
#define SRC_CLIENT_GUI3_EVENTS_KEY_HPP_

#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>

namespace gui3
{

class Key
{
public:
	static const Key NONE;

	Key();

	bool isControl() const;
	bool isShift() const;
	bool isAlt() const;
	bool isMeta() const;

	bool isUnknown() const;
	bool isNative() const;

	bool operator==(const Key & key) const;
	bool operator!=(const Key & key) const;
	bool operator<(const Key & key) const;

	static Key fromSFML(sf::Keyboard::Key key);
	static sf::Keyboard::Key toSFML(Key key);

	static Key fromNative(sf::Int32 native);
	static sf::Int32 toNative(Key key);

	static Key fromInt32(sf::Int32 code);
	static sf::Int32 toInt32(Key key);

	static Key fromLua(sf::Int32 id);
	static sf::Int32 toLua(Key key);

	static Key fromString(std::string name);
	static const std::string & toString(Key key);

private:
	sf::Int32 code;
};

}

#endif
