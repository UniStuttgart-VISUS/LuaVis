#include <Client/GUI3/Events/Key.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <cstddef>
#include <map>
#include <vector>

namespace gui3
{

const Key Key::NONE = Key();

Key::Key() :
	code(-2)
{
}

bool Key::isControl() const
{
	return *this == Key::fromSFML(sf::Keyboard::LControl) || *this == Key::fromSFML(sf::Keyboard::RControl);
}

bool Key::isShift() const
{
	return *this == Key::fromSFML(sf::Keyboard::LShift) || *this == Key::fromSFML(sf::Keyboard::RShift);
}

bool Key::isAlt() const
{
	return *this == Key::fromSFML(sf::Keyboard::LAlt) || *this == Key::fromSFML(sf::Keyboard::RAlt);
}

bool Key::isMeta() const
{
	return *this == Key::fromSFML(sf::Keyboard::LSystem) || *this == Key::fromSFML(sf::Keyboard::RSystem);
}

bool Key::isUnknown() const
{
	return code < 0;
}

bool Key::isNative() const
{
	return code >= 65536;
}

bool Key::operator==(const Key & key) const
{
	return code == key.code;
}

bool Key::operator!=(const Key & key) const
{
	return code != key.code;
}

bool Key::operator<(const Key & key) const
{
	return code < key.code;
}

Key Key::fromSFML(sf::Keyboard::Key key)
{
	Key keyObject;
	keyObject.code = key;
	return keyObject;
}

sf::Keyboard::Key Key::toSFML(Key key)
{
	return (sf::Keyboard::Key) key.code;
}

Key Key::fromNative(sf::Int32 native)
{
	Key key;
	key.code = native + 65536;
	return key;
}

sf::Int32 Key::toNative(Key key)
{
	return key.code - 65536;
}

Key Key::fromInt32(sf::Int32 code)
{
	Key key;
	key.code = code;
	return key;
}

sf::Int32 Key::toInt32(Key key)
{
	return key.code;
}

Key Key::fromLua(sf::Int32 id)
{
	Key key;
	key.code = id - 2;
	return key;
}

sf::Int32 Key::toLua(Key key)
{
	return key.code + 2;
}

// clang-format off
static std::vector<std::string> keyNames = {
    "unknown",   "a",       "b",        "c",        "d",         "e",       "f",         "g",        "h",
    "i",         "j",       "k",        "l",        "m",         "n",       "o",         "p",        "q",
    "r",         "s",       "t",        "u",        "v",         "w",       "x",         "y",        "z",
    "0",         "1",       "2",        "3",        "4",         "5",       "6",         "7",        "8",
    "9",         "escape",  "lcontrol", "lshift",   "lalt",      "lsystem", "rcontrol",  "rshift",   "ralt",
    "rsystem",   "menu",    "lbracket", "rbracket", "semicolon", "comma",   "period",    "quote",    "slash",
    "backslash", "tilde",   "equal",    "dash",     "space",     "enter",   "backspace", "tab",      "pageup",
    "pagedown",  "end",     "home",     "insert",   "delete",    "add",     "subtract",  "multiply", "divide",
    "left",      "right",   "up",       "down",     "numpad0",   "numpad1", "numpad2",   "numpad3",  "numpad4",
    "numpad5",   "numpad6", "numpad7",  "numpad8",  "numpad9",   "f1",      "f2",        "f3",       "f4",
    "f5",        "f6",      "f7",       "f8",       "f9",        "f10",     "f11",       "f12",      "f13",
    "f14",       "f15",     "pause"};
// clang-format on

static std::map<std::string, Key> keyNameLookupMap;

void initKeyNameLookupMapIfNeeded()
{
	if (keyNameLookupMap.empty())
	{
		for (std::size_t i = 0; i < keyNames.size(); ++i)
		{
			keyNameLookupMap[toLowercase(keyNames[i])] = Key::fromInt32(sf::Int32(i) - 1);
		}
	}
}

Key Key::fromString(std::string name)
{
	initKeyNameLookupMapIfNeeded();

	auto it = keyNameLookupMap.find(toLowercase(name));
	if (it == keyNameLookupMap.end())
	{
		return NONE;
	}
	else
	{
		return it->second;
	}
}

const std::string & Key::toString(Key key)
{
	if (key.code > sf::Int32(keyNames.size()) - 2 || key.code < -1)
	{
		static std::string empty;
		return empty;
	}
	else
	{
		return keyNames[key.code + 1];
	}
}

}
