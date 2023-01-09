#include "Shared/Gen/Noise.hpp"

#include <cmath>
#include <vector>

namespace gen
{

std::vector<sf::Int32> fadeArray(256);

std::vector<sf::Uint8> p(512);

std::vector<sf::Uint8> permutation = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142,
    8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
    117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165,
    71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,
    55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,
    18,  169, 200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250,
    124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189,
    28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
    129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,
    242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
    181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114,
    67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180};

sf::Int32 fade(sf::Int32 t)
{
	sf::Int32 t0 = fadeArray[t >> 8], t1 = fadeArray[std::min(255, (t >> 8) + 1)];
	return t0 + ((t & 255) * (t1 - t0) >> 8);
}

sf::Int32 grad(sf::Uint32 hash, sf::Int32 x, sf::Int32 y, sf::Int32 z)
{
	sf::Int32 h = hash & 15;
	sf::Int32 u = h < 8 ? x : y;
	sf::Int32 v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

sf::Int32 lerp(sf::Int32 t, sf::Int32 a, sf::Int32 b)
{
	return a + (t * (b - a) >> 12);
}

sf::Int32 intNoise(sf::Int32 x, sf::Int32 y, sf::Int32 z)
{
	sf::Int32 X = x >> 16 & 255, Y = y >> 16 & 255, Z = z >> 16 & 255, N = 1 << 16;

	x &= N - 1;
	y &= N - 1;
	z &= N - 1;

	sf::Int32 u = fade(x), v = fade(y), w = fade(z), A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z, B = p[X + 1] + Y,
	          BA = p[B] + Z, BB = p[B + 1] + Z;

	return lerp(w,
	            lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - N, y, z)),
	                 lerp(u, grad(p[AB], x, y - N, z), grad(p[BB], x - N, y - N, z))),
	            lerp(v, lerp(u, grad(p[AA + 1], x, y, z - N), grad(p[BA + 1], x - N, y, z - N)),
	                 lerp(u, grad(p[AB + 1], x, y - N, z - N), grad(p[BB + 1], x - N, y - N, z - N))));
}

sf::Int32 randomIntNoise2(sf::Int32 x, sf::Int32 y)
{
	sf::Uint32 value;
	value = sf::Uint32(x) * 3266489917 + 374761393;
	value = (value << 17) | (value >> 15);
	value += sf::Uint32(y) * 3266489917;
	value *= 668265263;
	value ^= value >> 15;
	value *= 2246822519;
	value ^= value >> 13;
	value *= 3266489917;
	value ^= value >> 16;
	return sf::Int32(value & 0x7FFFFFFF);
}

sf::Int32 randomIntNoise3(sf::Int32 x, sf::Int32 y, sf::Int32 z)
{
	return randomIntNoise2(x, y) ^ randomIntNoise2(y, z) ^ randomIntNoise2(z, x);
}

void generatePerm()
{
	for (std::size_t i = 0; i < 256; i++)
	{
		p[256 + i] = p[i] = permutation[i];
	}
}

double fadeFunc(double t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

void generateFade()
{
	for (std::size_t i = 0; i < 256; i++)
	{
		fadeArray[i] = sf::Int32((1 << 12) * fadeFunc(i / 256.0));
	}
}

bool tablesGenerated = false;

float noise(int x, int y, int seed)
{
	int n = x + y * 57;
	n = ((n << 13) + seed) ^ n;
	return (1.f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.f);
}

float smoothNoise(float x, float y, int seed)
{
	float corners =
	    (noise(x - 1, y - 1, seed) + noise(x + 1, y - 1, seed) + noise(x - 1, y + 1, seed) + noise(x + 1, y + 1, seed))
	    / 16.f;
	float sides = (noise(x - 1, y, seed) + noise(x + 1, y, seed) + noise(x, y - 1, seed) + noise(x, y + 1, seed)) / 8.f;
	float center = noise(x, y, seed) / 4;

	return corners + sides + center;
}

float interpolate(float a, float b, float x)
{
	return a * (1 - x) + b * x;
}

float interpolateCos(float a, float b, float x)
{
	float ft = x * 3.1415927f, f = (1.f - std::cos(ft)) * 0.5f;
	return a * (1.f - f) + b * f;
}

float interpolatedNoise(float x, float y, int seed)
{
	int integerX = x;
	float fractionalX = x - integerX;

	int integerY = y;
	float fractionalY = y - integerY;

	float v1 = smoothNoise(integerX, integerY, seed);
	float v2 = smoothNoise(integerX + 1, integerY, seed);
	float v3 = smoothNoise(integerX, integerY + 1, seed);
	float v4 = smoothNoise(integerX + 1, integerY + 1, seed);

	float i1 = interpolate(v1, v2, fractionalX);
	float i2 = interpolate(v3, v4, fractionalX);

	return interpolate(i1, i2, fractionalY);
}

float perlinNoise(float x, float y, float p, int octaves, int seed)
{
	float total = 0.f;
	int n = octaves - 1;
	float frequency;
	float amplitude;

	for (int i = 0; i <= n; i++)
	{
		frequency = 2.f * i;
		amplitude = p * i;

		total = total + interpolatedNoise(x * frequency, y * frequency, seed) * amplitude;
	}

	return total;
}

float ridge(float h, float offset)
{
	h = std::abs(h);
	h = offset - h;
	h = h * h;
	return h;
}

float ridgedMF(int x, int y, int seed, float lacunarity, float gain, float offset, int octaves, float xScale,
               float yScale)
{
	float sum = 0.f;
	float amplitude = 0.5f;
	float frequency = 1.0f;
	float prev = 1.0f;

	float cx = x * xScale;
	float cy = y * yScale;

	for (int i = 0; i < octaves; i++)
	{
		float n = ridge(interpolatedNoise((cx * frequency), (cy * frequency), seed), offset);
		sum += n * amplitude * prev;
		prev = n;
		frequency *= lacunarity;
		amplitude *= gain;
	}

	return sum;
}

float clamp(float x)
{
	if (x < 0.0)
		return 0.0;
	if (x > 1.0)
		return 1.0;
	return x;
}

float clamp(float x, float min, float max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

/// bilinear interpolation algorithm.
/// interpolates a coordinate within 4 points.
float interpolateBilinear(float x, float y, float stl, float str, float sbl, float sbr)
{
	float topval = interpolate(stl, str, x);
	float botval = interpolate(sbl, sbr, x);
	float bilval = interpolate(topval, botval, y);
	return bilval;
}

} // namespace gen

IntegerNoise & IntegerNoise::getInstance()
{
	static IntegerNoise instance;
	return instance;
}

IntegerNoise::IntegerNoise()
{
	if (!gen::tablesGenerated)
	{
		gen::generatePerm();
		gen::generateFade();
		gen::tablesGenerated = true;
	}
}

sf::Int32 IntegerNoise::noise(sf::Int32 x, sf::Int32 y, sf::Int32 z) const
{
	return gen::randomIntNoise3(x, y, z);
}
