#include <Shared/Lua/Bridges/ConsoleBridge.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <functional>
#include <iostream>
#include <string>

#if defined(WOS_LINUX) || defined(WOS_OSX)
#	include <stdio.h>
#	include <sys/select.h>
#	include <unistd.h>
#endif

namespace lua
{

ConsoleBridge::ConsoleBridge() :
	logger("ConsoleBridge")
{
}

ConsoleBridge::~ConsoleBridge()
{
}

void ConsoleBridge::onLoad(BridgeLoader & loader)
{
	// TODO move this to a dedicated class
	// TODO windows
	loader.bind(
	    "console.getLineAsync", std::function<sol::object(sol::this_state)>([=](sol::this_state state) -> sol::object {
#if defined(WOS_LINUX) || defined(WOS_OSX)
		    if (isatty(fileno(stdin)))
		    {
			    fd_set readFDs, writeFDs, exceptFDs;
			    FD_ZERO(&readFDs);
			    FD_ZERO(&writeFDs);
			    FD_ZERO(&exceptFDs);
			    FD_SET(fileno(stdin), &readFDs);

			    struct timeval timeout;
			    timeout.tv_sec = 0;
			    timeout.tv_usec = 0;

			    if (select(fileno(stdin) + 1, &readFDs, &writeFDs, &exceptFDs, &timeout) > 0 && FD_ISSET(0, &readFDs))
			    {
				    std::string line;
				    std::getline(std::cin, line);
				    return sol::make_object(state, line);
			    }
		    }
#endif
		    return sol::make_object(state, sol::lua_nil);
	    }));
}

}
