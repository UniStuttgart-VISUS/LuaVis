#ifndef SRC_SHARED_UTILS_CASEINSENSITIVEMAP_HPP_
#define SRC_SHARED_UTILS_CASEINSENSITIVEMAP_HPP_

#include <algorithm>
#include <cctype>
#include <map>
#include <string>

namespace priv
{

struct CaseInsensitiveComparator
{
	inline bool operator()(const std::string & s1, const std::string & s2) const
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), [](char ch1, char ch2) {
			return std::tolower(ch1) < std::tolower(ch2);
		});
	}
};

}

template <typename ValueType>
using CaseInsensitiveMap = std::map<std::string, ValueType, priv::CaseInsensitiveComparator>;

#endif
