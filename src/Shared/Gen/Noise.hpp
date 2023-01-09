#ifndef NOISE_HPP
#define NOISE_HPP

#include <SFML/Config.hpp>

namespace gen
{

float noise(int x, int y, int seed);
float smoothNoise(float x, float y, int seed);
float interpolate(float a, float b, float x);
float interpolateCos(float a, float b, float x);
float interpolatedNoise(float x, float y, int seed);
float perlinNoise(float x, float y, float p, int octaves, int seed);
float ridge(float h, float offset);
float ridgedMF(int x, int y, int seed, float lacunarity, float gain, float offset, int octaves, float hScale,
               float vScale);
float interpolateBilinear(float x, float y, float stl, float str, float sbl, float sbr);

}

class IntegerNoise
{

public:
	static IntegerNoise & getInstance();

	sf::Int32 noise(sf::Int32 x, sf::Int32 y, sf::Int32 z) const;

private:
	IntegerNoise();
};

#endif
