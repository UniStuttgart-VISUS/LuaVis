#ifndef SRC_SHARED_LUA_BINDINGS_SYSTEMBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_SYSTEMBINDING_H_

#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

WOSC_API double wosC_sys_getClockMicroseconds();

}

#endif
