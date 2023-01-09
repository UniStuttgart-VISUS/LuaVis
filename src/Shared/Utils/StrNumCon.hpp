#ifndef STR_NUM_CON_HPP
#define STR_NUM_CON_HPP

#include <SFML/Config.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdlib>
#include <string>

namespace detail
{

inline std::string removeTrailingZeros(std::string str)
{
	auto dot = str.find_last_of('.');
	if (dot != std::string::npos)
	{
		auto nonZero = str.find_last_not_of('0');
		auto erasePos = (nonZero == dot ? nonZero + 2 : nonZero + 1);
		if (erasePos <= str.size())
		{
			str.erase(erasePos);
		}
	}
	return str;
}

}

// hex character functions...

inline sf::Int8 hex2Num(sf::Uint32 hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 0xA;
	if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 0xA;
	else
		return 0;
}

inline char num2Hex(sf::Int8 num)
{
	if (num >= 0 && num <= 9)
		return '0' + num;
	if (num >= 0xA && num <= 0xF)
		return num + 'A' - 0xA;
	else
		return '0';
}

// utility functions...

inline bool stringIsPositiveInteger(const std::string & str)
{
	for (char c : str)
	{
		if (c < '0' || c > '9')
		{
			return false;
		}
	}
	return true;
}

template <class T>
std::string cNtoS(T num)
{
	return std::to_string(num);
}

template <>
inline std::string cNtoS(float num)
{
	return detail::removeTrailingZeros(std::to_string(num));
}

template <>
inline std::string cNtoS(double num)
{
	return detail::removeTrailingZeros(std::to_string(num));
}

template <typename TVec>
std::string cNtoS(sf::Vector2<TVec> vec)
{
	std::string str1, str2;
	cNtoS(vec.x, str1);
	cNtoS(vec.y, str2);
	return str1 + ";" + str2;
}

template <typename T>
inline T cStoN(const std::string & str)
{
	char * unused;
	return std::strtol(str.c_str(), &unused, 10);
}

template <>
inline unsigned int cStoN(const std::string & str)
{
	char * unused;
	return std::strtoul(str.c_str(), &unused, 10);
}

template <>
inline unsigned long cStoN(const std::string & str)
{
	char * unused;
	return std::strtoul(str.c_str(), &unused, 10);
}

template <>
inline long long cStoN(const std::string & str)
{
	char * unused;
	return std::strtoull(str.c_str(), &unused, 10);
}

template <>
inline unsigned long long cStoN(const std::string & str)
{
	char * unused;
	return std::strtoull(str.c_str(), &unused, 10);
}

template <>
inline float cStoN(const std::string & str)
{
	char * unused;
	return std::strtof(str.c_str(), &unused);
}

template <>
inline double cStoN(const std::string & str)
{
	char * unused;
	return std::strtod(str.c_str(), &unused);
}

template <typename TVec>
sf::Vector2<TVec> cStoV(const std::string & str)
{
	TVec num1, num2;
	std::string::size_type found = str.find_first_of(';');

	if (found == std::string::npos || found == str.size() - 1)
		return sf::Vector2<TVec>();

	if (!str2Num(num1, str.substr(0, found)))
		return sf::Vector2<TVec>();
	if (!str2Num(num2, str.substr(found + 1)))
		return sf::Vector2<TVec>();
	return sf::Vector2<TVec>(num1, num2);
}

inline sf::Uint8 cStoUB(const std::string & str)
{
	return cStoN<sf::Uint8>(str);
}
inline sf::Uint16 cStoUS(const std::string & str)
{
	return cStoN<sf::Uint16>(str);
}
inline sf::Uint32 cStoUI(const std::string & str)
{
	return cStoN<sf::Uint32>(str);
}
inline sf::Uint64 cStoUL(const std::string & str)
{
	return cStoN<sf::Uint64>(str);
}
inline sf::Int8 cStoB(const std::string & str)
{
	return cStoN<sf::Int8>(str);
}
inline sf::Int16 cStoS(const std::string & str)
{
	return cStoN<sf::Int16>(str);
}
inline sf::Int32 cStoI(const std::string & str)
{
	return cStoN<sf::Int32>(str);
}
inline sf::Int64 cStoL(const std::string & str)
{
	return cStoN<sf::Int64>(str);
}
inline float cStoF(const std::string & str)
{
	return cStoN<float>(str);
}
inline double cStoD(const std::string & str)
{
	return cStoN<double>(str);
}

#endif
