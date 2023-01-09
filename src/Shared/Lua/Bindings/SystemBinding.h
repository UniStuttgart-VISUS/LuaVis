#ifndef SRC_SHARED_LUA_BINDINGS_SYSTEMBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_SYSTEMBINDING_H_

#include <Shared/Lua/Bindings/BindingAPI.hpp>
#include <stdint.h>

extern "C"
{
	WOSC_API double wosC_sys_getClockMicroseconds();
	WOSC_API void wosC_sys_sleep(double microseconds);

	WOSC_API int32_t wosC_sys_getHash(const char * data, int32_t length);
}

#endif
