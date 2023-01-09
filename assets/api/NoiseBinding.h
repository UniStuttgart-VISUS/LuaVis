#ifndef SRC_SHARED_LUA_BINDINGS_NOISEBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_NOISEBINDING_H_

#include <Shared/Lua/Bindings/BindingAPI.hpp>
#include <stdint.h>

extern "C"
{

WOSC_API int32_t wosC_noise_gen3(int32_t x, int32_t y, int32_t z);

}

#endif
