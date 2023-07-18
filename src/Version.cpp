#include <Version.hpp>

#ifndef WOS_BUILD_NUMBER
#	include "BuildNum.hpp"
#endif

#ifndef WOS_BUILD_TIMESTAMP
#	if __has_include("BuildTimestamp.hpp")
#		include "BuildTimestamp.hpp"
#	else
#		define WOS_BUILD_TIMESTAMP 0
#	endif
#endif

#ifndef WOS_APPLICATION_NAME
#	define WOS_APPLICATION_NAME LuaVis
#endif

#define WOS_SYMBOL_TO_STRING_IMPL(SYMBOL) #SYMBOL
#define WOS_SYMBOL_TO_STRING(SYMBOL) WOS_SYMBOL_TO_STRING_IMPL(SYMBOL)

namespace version
{
const int build = WOS_BUILD_NUMBER;
const char * applicationName = WOS_SYMBOL_TO_STRING(WOS_APPLICATION_NAME);
const unsigned long long buildTimestamp = WOS_BUILD_TIMESTAMP;
}
