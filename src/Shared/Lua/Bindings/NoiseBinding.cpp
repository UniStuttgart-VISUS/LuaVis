#include <Shared/Gen/Noise.hpp>
#include <Shared/Lua/Bindings/NoiseBinding.h>

extern "C"
{

	int32_t wosC_noise_gen3(int32_t x, int32_t y, int32_t z)
	{
		return IntegerNoise::getInstance().noise(x, y, z);
	}
}
