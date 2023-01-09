#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>

#include <Shared/Lua/Bindings/SystemBinding.h>
#include <Shared/Utils/Hash.hpp>
#include <Shared/Utils/Timer.hpp>

extern "C"
{
	double wosC_sys_getClockMicroseconds()
	{
		return Timer::getGlobalTime().asMicroseconds();
	}

	void wosC_sys_sleep(double microseconds)
	{
		sf::sleep(sf::microseconds(microseconds));
	}

	int32_t wosC_sys_getHash(const char * data, int32_t length)
	{
		return static_cast<int32_t>(hash::dataHash32(data, std::max<int32_t>(0, length)));
	}
}
