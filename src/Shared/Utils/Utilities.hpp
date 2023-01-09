#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <limits>
#include <string>
#include <vector>

std::string toUppercase(std::string str);
std::string toLowercase(std::string str);

std::string capitalize(std::string str);
std::string uncapitalize(std::string str);

bool equalsIgnoreCase(const std::string & a, const std::string & b);
bool stringStartsWith(const std::string & string, const std::string & prefix);
bool stringEndsWith(const std::string & string, const std::string & suffix);

std::vector<std::string> splitString(std::string str, const std::string & separator, bool ignoreEmpty = false);
std::string joinString(const std::vector<std::string> & strings, const std::string & separator);

std::string getTimeString();

bool listFiles(const std::string & dir, std::vector<std::string> & vec, bool recursive = false, bool sorted = true);
bool listDirectories(const std::string & dir, std::vector<std::string> & vec, bool recursive = false,
                     bool sorted = true);

bool fileExists(const std::string & filename);
bool isRegularFile(const std::string & filename);
bool isDirectory(const std::string & filename);

bool createDirectory(const std::string & path, bool recursive = false);
bool moveFile(const std::string & sourceFilename, const std::string & targetFilename);
bool deleteFile(const std::string & filename);
bool deleteDirectory(const std::string & path, bool recursive = false);

std::string readFileToString(const std::string & filename,
                             std::size_t maxBytes = std::numeric_limits<std::size_t>::max());
bool writeStringToFile(const std::string & filename, const std::string & data);

std::string extractFileExtension(const std::string & filename);
std::string removeFileExtension(const std::string & filename);

std::string extractFileName(const std::string & path);
std::string removeFileName(const std::string & path);

std::string joinPaths(const std::string & path1, const std::string & path2);

template <typename T>
sf::Rect<T> moveRect(sf::Rect<T> rect, sf::Vector2<T> off)
{
	rect.left += off.x;
	rect.top += off.y;
	return rect;
}
template <typename T, typename T2>
sf::Rect<T> expandRect(sf::Rect<T> rect, sf::Vector2<T2> exp)
{
	rect.left -= exp.x;
	rect.top -= exp.y;
	rect.width += exp.x * 2;
	rect.height += exp.y * 2;
	return rect;
}
template <typename T, typename T2>
sf::Rect<T> expandRect(sf::Rect<T> rect, T2 exp)
{
	return expandRect(rect, {exp, exp});
}

std::string getByteSizeString(sf::Uint64 bytes);
std::string getSfTimeString(sf::Time time);
std::string getRoughSfTimeString(sf::Time time);

template <typename T>
sf::Rect<T> rectIntersect(sf::Rect<T> rect1, sf::Rect<T> rect2)
{
	T x1 = std::max(rect1.left, rect2.left);
	T y1 = std::max(rect1.top, rect2.top);
	T x2 = std::min(rect1.left + rect1.width, rect2.left + rect2.width);
	T y2 = std::min(rect1.top + rect1.height, rect2.top + rect2.height);
	return sf::Rect<T>(x1, y1, x2 - x1, y2 - y1);
}

template <typename T>
sf::Rect<T> rectUnion(sf::Rect<T> rect1, sf::Rect<T> rect2)
{
	T x1 = std::min(rect1.left, rect2.left);
	T y1 = std::min(rect1.top, rect2.top);
	T x2 = std::max(rect1.left + rect1.width, rect2.left + rect2.width);
	T y2 = std::max(rect1.top + rect1.height, rect2.top + rect2.height);
	return sf::Rect<T>(x1, y1, x2 - x1, y2 - y1);
}

template <typename T, typename T2>
sf::Rect<T> rectScale(sf::Rect<T> rect, T2 scaleFactor)
{
	T2 expansion = (scaleFactor - 1) / 2;
	return expandRect(rect, {rect.width * expansion, rect.height * expansion});
}

template <typename T, typename T2>
sf::Rect<T> rectSetSizeCentered(sf::Rect<T> rect, sf::Vector2<T2> size)
{
	return sf::Rect<T>(rect.left - (size.x - rect.width) / 2, rect.top - (size.y - rect.height) / 2, size.x, size.y);
}

template <typename T>
sf::Vector2<T> rectCenter(sf::Rect<T> rect)
{
	return sf::Vector2<T>(rect.left + rect.width / 2, rect.top + rect.height / 2);
}

bool pointInPoly(const std::vector<sf::Vector2f> & vertices, const sf::Vector2f & point);

template <typename T>
void addToSortedVector(std::vector<T> & vector, T value)
{
	auto insertIterator = std::lower_bound(vector.begin(), vector.end(), value);
	if (insertIterator == vector.end() || *insertIterator != value)
	{
		vector.insert(insertIterator, value);
	}
}

template <typename T, typename F>
void addToSortedVector(std::vector<T> & vector, T value, F compare)
{
	auto insertIterator = std::lower_bound(vector.begin(), vector.end(), value, compare);
	if (insertIterator == vector.end() || *insertIterator != value)
	{
		vector.insert(insertIterator, value);
	}
}

template <typename T>
void removeFromSortedVector(std::vector<T> & vector, T value)
{
	auto eraseIterator = std::lower_bound(vector.begin(), vector.end(), value);
	if (eraseIterator != vector.end() && *eraseIterator == value)
	{
		vector.erase(eraseIterator);
	}
}

template <typename T, typename F>
void removeFromSortedVector(std::vector<T> & vector, T value, F compare)
{
	auto eraseIterator = std::lower_bound(vector.begin(), vector.end(), value, compare);
	if (eraseIterator != vector.end() && *eraseIterator == value)
	{
		vector.erase(eraseIterator);
	}
}

template <typename T>
std::size_t allocateVectorSlot(std::vector<T> & vector, T data = T())
{
	for (std::size_t i = 0; i < vector.size(); ++i)
	{
		if (!vector[i])
		{
			vector[i] = std::move(data);
			return i;
		}
	}
	vector.push_back(std::move(data));
	return vector.size() - 1;
}

#endif
