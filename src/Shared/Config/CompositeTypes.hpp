#ifndef SRC_SHARED_CONFIG_COMPOSITETYPES_HPP_
#define SRC_SHARED_CONFIG_COMPOSITETYPES_HPP_

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/Config/DataTypes.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#ifdef WOS_SERVER

namespace sf
{
struct Color
{
	Color() :
		Color(0, 0, 0, 0)
	{
	}

	Color(sf::Uint8 r, sf::Uint8 g, sf::Uint8 b, sf::Uint8 a) :
		r(r),
		g(g),
		b(b),
		a(a)
	{
	}

	sf::Uint8 r, g, b, a;
};
}

#endif

namespace cfg
{

template <typename KeyType, bool transient>
class PrimitiveType
{
};

template <typename KeyType>
class PrimitiveType<KeyType, false>
{
public:
	using DataType = typename KeyType::DataType;

	PrimitiveType(std::string key) :
		key(std::move(key))
	{
	}

	DataType onGet(const Config & config) const
	{
		return config.getCachedValue(key);
	}

	void onSet(Config & config, DataType value) const
	{
		config.setCachedValue(key, value);
	}

private:
	KeyType key;
};

template <typename KeyType>
class PrimitiveType<KeyType, true>
{
public:
	using DataType = typename KeyType::DataType;

	PrimitiveType(std::string key) :
		key(std::move(key))
	{
	}

	DataType onGet(const Config & config) const
	{
		return config.getTransientValue<KeyType>(key);
	}

	void onSet(Config & config, DataType value) const
	{
		config.setTransientValue<KeyType>(key, value);
	}

private:
	std::string key;
};

template <bool transient = false>
using BasicString = PrimitiveType<detail::StringKey, transient>;

template <bool transient = false>
using BasicInt = PrimitiveType<detail::IntKey, transient>;

template <bool transient = false>
using BasicFloat = PrimitiveType<detail::FloatKey, transient>;

template <bool transient = false>
using BasicBool = PrimitiveType<detail::BoolKey, transient>;

template <typename ElementType, bool transient = false>
class BasicList
{
public:
	using DataType = std::vector<typename ElementType::DataType>;

	BasicList(std::string key) :
		key(key),
		lengthKey(key + "[length]")
	{
	}

	const BasicInt<transient> & length() const
	{
		return lengthKey;
	}

	DataType onGet(const Config & config) const
	{
		DataType list(config.get(length()));
		updateKeyCount(list.size());

		for (std::size_t i = 0; i < list.size(); ++i)
		{
			list[i] = config.get(elementKeys[i]);
		}

		return list;
	}

	void onSet(Config & config, DataType value) const
	{
		config.set(length(), value.size());
		updateKeyCount(value.size());

		for (std::size_t i = 0; i < value.size(); ++i)
		{
			config.set(elementKeys[i], value[i]);
		}
	}

	const ElementType & operator[](std::size_t index) const
	{
		if (index < elementKeys.size())
		{
			return elementKeys[index];
		}
		else
		{
			return getElementKey(index);
		}
	}

private:
	void updateKeyCount(std::size_t keyCount) const
	{
		if (elementKeys.size() != keyCount)
		{
			std::size_t oldSize = elementKeys.size();

			if (oldSize < keyCount)
			{
				for (std::size_t i = oldSize; i < keyCount; ++i)
				{
					elementKeys.emplace_back(getElementKey(i));
				}
			}
			else if (oldSize > keyCount)
			{
				elementKeys.erase(elementKeys.begin() + keyCount, elementKeys.begin() + oldSize);
			}
		}
	}

	ElementType getElementKey(std::size_t index) const
	{
		return ElementType(key + "[" + cNtoS(index) + "]");
	}

	std::string key;
	BasicInt<transient> lengthKey;
	mutable std::vector<ElementType> elementKeys;
};

template <bool transient = false>
class BasicVector2f
{
public:
	BasicVector2f(std::string key) :
		length(key + "[length]"),
		x(key + "[0]"),
		y(key + "[1]")
	{
	}

	using DataType = sf::Vector2f;

	DataType onGet(const Config & config) const
	{
		if (config.get(length) == 2)
		{
			return sf::Vector2f(config.get(x), config.get(y));
		}
		else
		{
			return sf::Vector2f();
		}
	}

	void onSet(Config & config, DataType value) const
	{
		config.set(x, value.x);
		config.set(y, value.y);
	}

private:
	BasicInt<transient> length;
	BasicFloat<transient> x;
	BasicFloat<transient> y;
};

template <bool transient = false>
class BasicColor
{
public:
	using DataType = sf::Color;

	BasicColor(std::string key) :
		length(key + "[length]"),
		r(key + "[0]"),
		g(key + "[1]"),
		b(key + "[2]"),
		a(key + "[3]")
	{
	}

	DataType onGet(const Config & config) const
	{
		if (config.get(length) == 4)
		{
			return sf::Color(config.get(r), config.get(g), config.get(b), config.get(a));
		}
		else if (config.get(length) == 3)
		{
			return sf::Color(config.get(r), config.get(g), config.get(b), 255);
		}
		else
		{
			return sf::Color();
		}
	}

	void onSet(Config & config, DataType value) const
	{
		config.set(r, value.r);
		config.set(g, value.g);
		config.set(b, value.b);
		config.set(a, value.a);
	}

private:
	BasicInt<transient> length;
	BasicInt<transient> r;
	BasicInt<transient> g;
	BasicInt<transient> b;
	BasicInt<transient> a;
};

using Bool = BasicBool<false>;
using Int = BasicInt<false>;
using Float = BasicFloat<false>;
using String = BasicString<false>;
using Color = BasicColor<false>;
using Vector2f = BasicVector2f<false>;

template <typename T>
using List = BasicList<T, false>;

namespace transient
{

using Bool = BasicBool<true>;
using Int = BasicInt<true>;
using Float = BasicFloat<true>;
using String = BasicString<true>;
using Color = BasicColor<true>;
using Vector2f = BasicVector2f<true>;

template <typename T>
using List = BasicList<T, true>;

}

}

#endif
