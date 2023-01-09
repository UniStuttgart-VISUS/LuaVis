#include <Shared/Lua/Bindings/Accel/VisionBinding.h>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/Attributes.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/MiscMath.hpp>
#include <cstddef>
#include <cstdlib>
#include <limits>

template <typename EntryType>
struct AbstractVisMapWrapper
{
	using Entry = EntryType;
	using Coord = wosC_accel_vision_tileCoord_t;

	inline Entry & at(Coord x, Coord y)
	{
#ifdef WOS_DEBUG
		if (!validCoord(x, y))
		{
			throw Error("VisMap access out of bounds");
		}
#endif
		return data[x + y * width];
	}

	inline const Entry & at(Coord x, Coord y) const
	{
#ifdef WOS_DEBUG
		if (!validCoord(x, y))
		{
			throw Error("VisMap access out of bounds");
		}
#endif
		return data[x + y * width];
	}

	inline bool test(Coord x, Coord y, Entry bitmask) const
	{
		return at(x, y) & bitmask;
	}

	inline void set(Coord x, Coord y, Entry bitmask)
	{
		at(x, y) |= bitmask;
	}

	inline void unset(Coord x, Coord y, Entry bitmask)
	{
		at(x, y) &= ~bitmask;
	}

	inline void invert(Coord x, Coord y, Entry bitmask)
	{
		at(x, y) ^= bitmask;
	}

	inline void setChecked(Coord x, Coord y, Entry bitmask)
	{
		if (validCoord(x, y))
		{
			at(x, y) |= bitmask;
		}
	}

	inline bool validCoord(Coord x, Coord y) const
	{
		return x >= clipX1 && y >= clipY1 && x <= clipX2 && y <= clipY2;
	}

	inline bool isClipped() const
	{
		return clipX1 > 0 || clipY1 > 0 || clipX2 < width || clipY2 < height;
	}

	inline operator bool() const
	{
		return data != nullptr;
	}

	Coord width = 0;
	Coord height = 0;

	Coord clipX1 = 0;
	Coord clipY1 = 0;
	Coord clipX2 = 0;
	Coord clipY2 = 0;

	Entry * data = nullptr;
};

/**
 * Wrapper class for a VisMap array
 */
struct VisMap : public AbstractVisMapWrapper<wosC_accel_vision_visMap_entry_t>
{
	using LightCoord = wosC_accel_vision_lightCoord_t;
	using LightIntensity = wosC_accel_vision_lightIntensity_t;
};

/**
 * Wrapper class for a FovMap array
 */
using FovMap = AbstractVisMapWrapper<wosC_accel_vision_fovMap_entry_t>;

/**
 * Wrapper class for a RevealMap array
 */
using RevealMap = AbstractVisMapWrapper<wosC_accel_vision_revealMap_entry_t>;

/**
 * Wrapper class for a LightMap array
 */
using LightMap = AbstractVisMapWrapper<wosC_accel_vision_lightValue_t>;

static const VisMap::Entry bitOffsetLight = wosC_accel_vision_bitOffsetLight;
static const VisMap::Entry bitCountLight = wosC_accel_vision_bitCountLight;
static const VisMap::Entry bitmaskOpaque = 1u << wosC_accel_vision_bitOffsetOpaque;
static const VisMap::Entry bitmaskValid = 1u << wosC_accel_vision_bitOffsetValid;
static const VisMap::Entry bitmaskShadow = 1u << wosC_accel_vision_bitOffsetShadow;
static const VisMap::Entry bitmaskPerspectiveReveal = 1u << wosC_accel_vision_bitOffsetPerspectiveReveal;
static const VisMap::Entry bitmaskPerspectiveOcclude = 1u << wosC_accel_vision_bitOffsetPerspectiveOcclude;
static const VisMap::Entry bitmaskPerspectiveIlluminate = 1u << wosC_accel_vision_bitOffsetPerspectiveIlluminate;
static const VisMap::Entry bitmaskAll = ~0u;

static const VisMap::Coord lightTileSize = wosC_accel_vision_lightTileSize;
static const VisMap::LightIntensity lightRevealThreshold = wosC_accel_vision_lightRevealThreshold;
static const VisMap::LightIntensity lightLevelMax = wosC_accel_vision_lightLevelMax;
static const LightMap::Entry brightnessRevealed = wosC_accel_vision_brightnessRevealed;
static const LightMap::Entry brightnessShadowed = wosC_accel_vision_brightnessShadowed;
static const LightMap::Entry brightnessMax = wosC_accel_vision_brightnessMax;

static const VisMap::LightIntensity lightBrightnessRatio = lightLevelMax / brightnessMax;

template <typename T>
int sign(T value)
{
	return (0 < value) - (value < 0);
}

template <typename T>
T gcd(T a, T b)
{
	return b != 0 ? gcd(b, a % b) : a;
}

template <typename T>
T isCoprimePair(T a, T b)
{
	return gcd(std::abs(a), std::abs(b)) <= 1;
}

inline uint32_t toTwosComplement(int32_t value, uint32_t size)
{
	return (uint32_t(value) << (32u - size)) >> (32u - size);
}

inline int32_t fromTwosComplement(uint32_t value, uint32_t size)
{
	auto mask = 1u << (size - 1);
	return (value ^ mask) - mask;
}

inline bool bitTestMask(uint32_t bitset, uint32_t bitmask)
{
	return bitset & bitmask;
}

inline uint32_t bitGenRange(uint32_t offset, uint32_t size)
{
	return (~0u << (32u - size)) >> (32u - offset - size);
}

inline uint32_t bitGetRange(uint32_t bitset, uint32_t offset, uint32_t size)
{
	return (bitset & bitGenRange(offset, size)) >> offset;
}

inline void bitSetRange(uint32_t & bitset, uint32_t offset, uint32_t size, uint32_t value)
{
	auto mask = bitGenRange(offset, size);
	bitset = ((bitset & ~mask) | ((value << offset) & mask));
}

inline int32_t bitGetSignedRange(uint32_t bitset, uint32_t offset, uint32_t size)
{
	return fromTwosComplement(bitGetRange(bitset, offset, size), size);
}

inline void bitSetSignedRange(uint32_t & bitset, uint32_t offset, uint32_t size, int32_t value)
{
	bitSetRange(bitset, offset, size, toTwosComplement(value, size));
}

static Logger logger()
{
	static Logger logInstance("VisionAccelerator");
	return logInstance;
}

template <typename WrapperType, typename StructureType>
WrapperType getWrapperGeneric(const StructureType * map)
{
	WrapperType wrapper;
	try
	{
		auto arrayInfo = wosc::ArrayContext::getContextByID(map->arrayContext).getArrayInfo(map->arrayID);

		if (map->width == 0 || map->height == 0 || map->clipWidth == 0 || map->clipHeight == 0)
		{
			// Zero-sized structure: do nothing
			return wrapper;
		}

		if (map->width < 0 || map->height < 0)
		{
			logger().error("Invalid VisMap size ({}x{})", map->width, map->height);
			return wrapper;
		}

		if (map->clipX < 0 || map->clipY < 0 || map->clipWidth < 0 || map->clipHeight < 0
		    || map->clipX + map->clipWidth > map->width || map->clipY + map->clipHeight > map->height)
		{
			logger().error("Invalid VisMap clipping rectangle ({}, {}; {}x{}) for VisMap size ({}x{})", map->clipX,
			               map->clipY, map->clipWidth, map->clipHeight, map->width, map->height);
			return wrapper;
		}

		auto expectedArrayLength = sizeof(typename WrapperType::Entry) * map->width * map->height;
		if (arrayInfo.size != expectedArrayLength)
		{
			logger().error("VisMap size mismatch (size: {}x{}; expected array length: {}, actual array length: {})",
			               map->width, map->height, expectedArrayLength, arrayInfo.size);
			return wrapper;
		}

		wrapper.data = reinterpret_cast<typename WrapperType::Entry *>(arrayInfo.data);
		wrapper.width = map->width;
		wrapper.height = map->height;
		wrapper.clipX1 = map->clipX;
		wrapper.clipY1 = map->clipY;
		wrapper.clipX2 = map->clipX + map->clipWidth - 1;
		wrapper.clipY2 = map->clipY + map->clipHeight - 1;
		return wrapper;
	}
	catch (std::exception & ex)
	{
		logger().error("Error retrieving VisMap data: {}", ex.what());
		return wrapper;
	}
}

inline VisMap getVisMapWrapper(const wosC_accel_vision_visMap_t * visMap)
{
	return getWrapperGeneric<VisMap>(visMap);
}

inline FovMap getFovMapWrapper(const wosC_accel_vision_fovMap_t * fovMap)
{
	return getWrapperGeneric<FovMap>(fovMap);
}

inline RevealMap getRevealMapWrapper(const wosC_accel_vision_revealMap_t * revealMap)
{
	return getWrapperGeneric<RevealMap>(revealMap);
}

inline LightMap getLightMapWrapper(const wosC_accel_vision_lightMap_t * lightMap)
{
	return getWrapperGeneric<LightMap>(lightMap);
}

template <typename Map>
void clearMapBits(Map map, typename Map::Entry mask)
{
	if (map.isClipped())
	{
		for (VisMap::Coord y = map.clipY1; y <= map.clipY2; ++y)
		{
			for (VisMap::Coord x = map.clipX1; x <= map.clipX2; ++x)
			{
				map.unset(x, y, mask);
			}
		}
	}
	else
	{
		// Fast path for whole-vismap updates
		for (std::size_t i = 0; i < map.width * map.height; ++i)
		{
			map.data[i] &= ~mask;
		}
	}
}

template <typename Map>
void setMapBits(Map map, typename Map::Entry mask)
{
	if (map.isClipped())
	{
		for (VisMap::Coord y = map.clipY1; y <= map.clipY2; ++y)
		{
			for (VisMap::Coord x = map.clipX1; x <= map.clipX2; ++x)
			{
				map.set(x, y, mask);
			}
		}
	}
	else
	{
		// Fast path for whole-vismap updates
		for (std::size_t i = 0; i < map.width * map.height; ++i)
		{
			map.data[i] |= mask;
		}
	}
}

template <typename Map>
void invertMapBits(Map map, typename Map::Entry mask)
{
	if (map.isClipped())
	{
		for (VisMap::Coord y = map.clipY1; y <= map.clipY2; ++y)
		{
			for (VisMap::Coord x = map.clipX1; x <= map.clipX2; ++x)
			{
				map.invert(x, y, mask);
			}
		}
	}
	else
	{
		// Fast path for whole-vismap updates
		for (std::size_t i = 0; i < map.width * map.height; ++i)
		{
			map.data[i] ^= mask;
		}
	}
}

template <typename Map>
void mapAddCircle(Map map, typename Map::Entry mask, VisMap::Coord cx, VisMap::Coord cy, VisMap::LightCoord radius)
{
	VisMap::LightCoord squareDistanceThreshold = radius * radius;
	VisMap::Coord extent = radius / lightTileSize;

	for (VisMap::Coord y = std::max(map.clipY1, cy - extent); y <= std::min(map.clipY2, cy + extent); ++y)
	{
		for (VisMap::Coord x = std::max(map.clipX1, cx - extent); x <= std::min(map.clipX2, cx + extent); ++x)
		{
			VisMap::Coord dx = x - cx, dy = y - cy;
			VisMap::LightCoord squareDistance = (dx * dx + dy * dy) * lightTileSize * lightTileSize;
			if (squareDistance <= squareDistanceThreshold)
			{
				map.set(x, y, mask);
			}
		}
	}
}

void wosC_accel_vision_fovClear(wosC_accel_vision_fovMap_t * fovMap, wosC_accel_vision_fovMap_entry_t mask)
{
	auto fovMapWrapper = getFovMapWrapper(fovMap);
	if (!fovMapWrapper)
	{
		logger().trace("Invalid FovMap passed to fovClear");
		return;
	}

	clearMapBits(fovMapWrapper, mask);
}

// Returns a mask with bits from lsb to msb (both inclusive) set to 1.
// e.g. mask(3, 4) = 0b11000
static uint64_t slopeMask(int64_t lsb, int64_t msb)
{
	uint64_t lower = static_cast<uint64_t>(1) << clamp<int64_t>(0, lsb, 62);
	uint64_t upper = static_cast<uint64_t>(1) << clamp<int64_t>(0, msb, 63);
	return upper ^ (upper - 1) ^ (lower - 1);
}

static bool isOpaque(const VisMap & visMap, VisMap::Coord x, VisMap::Coord y)
{
	return !visMap.validCoord(x, y) || visMap.test(x, y, bitmaskOpaque);
}

inline void fovAddOctant(const VisMap & visMap, FovMap & fovMap, FovMap::Entry mask, FovMap::Coord cx, FovMap::Coord cy,
                         FovMap::Coord dx, FovMap::Coord dy, FovMap::Coord px, FovMap::Coord py)
{
	uint64_t slopes = ~0;

	for (int d = 1; slopes; d++)
	{
		int x = cx + d * dx;
		int y = cy + d * dy;

		uint64_t blockedSlopes = 0;

		for (int p = 0; p <= d; p++)
		{
			int minSlope = 63 * (2 * p - 1) / (2 * d + 1);
			int maxSlope = 63 * (2 * p + 1) / (2 * d - 1);

			if (p == d && isOpaque(visMap, x - dx, y - dy))
			{
				slopes &= ~(static_cast<uint64_t>(1) << 63);
			}

			if ((slopes & slopeMask(minSlope - (p < 11), maxSlope - (p == 0))) && fovMap.validCoord(x, y))
			{
				fovMap.set(x, y, mask);
			}

			if (isOpaque(visMap, x, y))
			{
				blockedSlopes |= slopeMask(std::max(minSlope, p), maxSlope - 1);
			}

			x += px, y += py;
		}

		slopes &= ~blockedSlopes;
	}
}

void fovAddRaycast(VisMap visMap, FovMap fovMap, FovMap::Entry mask, VisMap::Coord cx, VisMap::Coord cy)
{
	if (!visMap.validCoord(cx, cy))
	{
		return;
	}

	fovMap.set(cx, cy, mask);
	fovAddOctant(visMap, fovMap, mask, cx, cy, +1, 0, 0, +1);
	fovAddOctant(visMap, fovMap, mask, cx, cy, +1, 0, 0, -1);
	fovAddOctant(visMap, fovMap, mask, cx, cy, -1, 0, 0, +1);
	fovAddOctant(visMap, fovMap, mask, cx, cy, -1, 0, 0, -1);
	fovAddOctant(visMap, fovMap, mask, cx, cy, 0, +1, +1, 0);
	fovAddOctant(visMap, fovMap, mask, cx, cy, 0, +1, -1, 0);
	fovAddOctant(visMap, fovMap, mask, cx, cy, 0, -1, +1, 0);
	fovAddOctant(visMap, fovMap, mask, cx, cy, 0, -1, -1, 0);
}

void wosC_accel_vision_fovAddRaycast(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_fovMap_t * fovMap,
                                     wosC_accel_vision_fovMap_entry_t mask, wosC_accel_vision_tileCoord_t x,
                                     wosC_accel_vision_tileCoord_t y)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to fovAddRaycast");
		return;
	}

	auto fovMapWrapper = getFovMapWrapper(fovMap);
	if (!fovMapWrapper)
	{
		logger().trace("Invalid FovMap passed to fovAddRaycast");
		return;
	}

	fovAddRaycast(visMapWrapper, fovMapWrapper, mask, x, y);
}

void wosC_accel_vision_fovAddCircle(wosC_accel_vision_fovMap_t * fovMap, wosC_accel_vision_fovMap_entry_t mask,
                                    wosC_accel_vision_tileCoord_t x, wosC_accel_vision_tileCoord_t y,
                                    wosC_accel_vision_lightCoord_t radius)
{
	auto fovMapWrapper = getFovMapWrapper(fovMap);
	if (!fovMapWrapper)
	{
		logger().trace("Invalid FovMap passed to fovAddCircle");
		return;
	}

	mapAddCircle(fovMapWrapper, mask, x, y, radius);
}

void fovReveal(VisMap visMap, FovMap fovMap, RevealMap revealMap)
{
	for (VisMap::Coord y = visMap.clipY1; y <= visMap.clipY2; ++y)
	{
		for (VisMap::Coord x = visMap.clipX1; x <= visMap.clipX2; ++x)
		{
			VisMap::Entry entry = visMap.at(x, y);
			if (bitGetSignedRange(entry, bitOffsetLight, bitCountLight) >= lightRevealThreshold
			    && !(entry & bitmaskShadow))
			{
				revealMap.set(x, y, fovMap.at(x, y));
			}
		}
	}
}

void wosC_accel_vision_fovReveal(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_fovMap_t * fovMap,
                                 wosC_accel_vision_revealMap_t * revealMap)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to fovReveal");
		return;
	}

	auto fovMapWrapper = getFovMapWrapper(fovMap);
	if (!fovMapWrapper)
	{
		logger().trace("Invalid FovMap passed to fovReveal");
		return;
	}

	auto revealMapWrapper = getRevealMapWrapper(revealMap);
	if (!revealMapWrapper)
	{
		logger().trace("Invalid RevealMap passed to fovReveal");
		return;
	}

	fovReveal(visMapWrapper, fovMapWrapper, revealMapWrapper);
}

void wosC_accel_vision_shadowClear(wosC_accel_vision_visMap_t * visMap)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to shadowClear");
		return;
	}

	clearMapBits(visMapWrapper, bitmaskShadow);
}

void wosC_accel_vision_shadowAddCircle(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_tileCoord_t x,
                                       wosC_accel_vision_tileCoord_t y, wosC_accel_vision_lightCoord_t radius)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to shadowAddCircle");
		return;
	}

	mapAddCircle(visMapWrapper, bitmaskShadow, x, y, radius);
}

void wosC_accel_vision_shadowInvert(wosC_accel_vision_visMap_t * visMap)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to shadowClear");
		return;
	}

	invertMapBits(visMapWrapper, bitmaskShadow);
}

void wosC_accel_vision_perspectiveClear(wosC_accel_vision_visMap_t * visMap)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to perspectiveClear");
		return;
	}

	clearMapBits(visMapWrapper, bitmaskPerspectiveOcclude);
}

void wosC_accel_vision_perspectiveFill(wosC_accel_vision_visMap_t * visMap)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to perspectiveFill");
		return;
	}

	setMapBits(visMapWrapper, bitmaskPerspectiveOcclude);
}

void wosC_accel_vision_perspectiveAddCircle(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_tileCoord_t x,
                                            wosC_accel_vision_tileCoord_t y, wosC_accel_vision_lightCoord_t radius)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to perspectiveAddCircle");
		return;
	}

	mapAddCircle(visMapWrapper, bitmaskPerspectiveOcclude, x, y, radius);
}

void lightApplyRadial(VisMap visMap, VisMap::Coord cx, VisMap::Coord cy, VisMap::LightCoord innerRadius,
                      VisMap::LightCoord outerRadius, VisMap::LightIntensity intensity)
{
	VisMap::Coord extent = outerRadius / lightTileSize;

	for (VisMap::Coord y = std::max(visMap.clipY1, cy - extent); y <= std::min(visMap.clipY2, cy + extent); ++y)
	{
		for (VisMap::Coord x = std::max(visMap.clipX1, cx - extent); x <= std::min(visMap.clipX2, cx + extent); ++x)
		{
			VisMap::Coord dx = x - cx, dy = y - cy;
			VisMap::LightCoord distance = clamp<VisMap::LightCoord>(
			    innerRadius, std::sqrt((dx * dx + dy * dy) * lightTileSize * lightTileSize), outerRadius);

			VisMap::Entry & entry = visMap.at(x, y);
			VisMap::LightIntensity lightValue = bitGetSignedRange(entry, bitOffsetLight, bitCountLight);

			if (innerRadius >= outerRadius)
			{
				lightValue += intensity;
			}
			else
			{
				lightValue += ((outerRadius - distance) * intensity) / (outerRadius - innerRadius);
			}

			bitSetSignedRange(entry, bitOffsetLight, bitCountLight, lightValue);
		}
	}
}

void wosC_accel_vision_lightApplyRadial(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_tileCoord_t x,
                                        wosC_accel_vision_tileCoord_t y, wosC_accel_vision_lightCoord_t innerRadius,
                                        wosC_accel_vision_lightCoord_t outerRadius,
                                        wosC_accel_vision_lightIntensity_t intensity)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to lightApplyRadial");
		return;
	}

	lightApplyRadial(visMapWrapper, x, y, innerRadius, outerRadius, intensity);
}

void updateLightMap(VisMap visMap, FovMap fovMap, RevealMap revealMap, LightMap lightMap, float factor)
{
	for (VisMap::Coord y = visMap.clipY1; y <= visMap.clipY2; ++y)
	{
		for (VisMap::Coord x = visMap.clipX1; x <= visMap.clipX2; ++x)
		{
			// Get current visibility map entry for reading level light and vision flags
			auto & visEntry = visMap.at(x, y);

			// Read the light level from all cumulative light sources at the current tile
			auto targetLight = bitGetSignedRange(visEntry, bitOffsetLight, bitCountLight);

			// Force light level to 0 for tiles outside the player's field of view
			targetLight *= fovMap.test(x, y, bitmaskAll);

			// Force light level to 0 for shadowed tiles
			bool shadowed = bitTestMask(visEntry, bitmaskShadow);
			targetLight *= (1 - shadowed);

			// Normalize to the [0, 255] range used by graphics
			targetLight /= lightBrightnessRatio;

			// Set the light level to at least brightnessRevealed for revealed tiles
			bool revealed = bitTestMask(visEntry, bitmaskPerspectiveReveal) | revealMap.test(x, y, bitmaskAll);
			shadowed &= !bitTestMask(visEntry, bitmaskOpaque);
			targetLight = std::max(targetLight, (brightnessRevealed - brightnessShadowed * shadowed) * revealed);

			// Force light level to 0 for tiles outside the perspective's view radius
			targetLight *= bitTestMask(visEntry, bitmaskPerspectiveOcclude);

			// Force light level to 255 for tiles illuminated by perspective
			targetLight |= brightnessMax * bitTestMask(visEntry, bitmaskPerspectiveIlluminate);

			// Gradually apply effective light level, with a limited approach rate
			// TODO Add light level flickering
			auto & lightEntry = lightMap.at(x, y);
			lightEntry = clamp<int>(0, interpolateLinear<int>(lightEntry, targetLight, factor), brightnessMax);
		}
	}
}

void wosC_accel_vision_updateLightMap(
		wosC_accel_vision_visMap_t * visMap,
		wosC_accel_vision_fovMap_t * fovMap,
		wosC_accel_vision_revealMap_t * revealMap,
		wosC_accel_vision_lightMap_t * lightMap,
		float factor)
{
	auto visMapWrapper = getVisMapWrapper(visMap);
	if (!visMapWrapper)
	{
		logger().trace("Invalid VisMap passed to updateLightmap");
		return;
	}

	auto fovMapWrapper = getFovMapWrapper(fovMap);
	if (!fovMapWrapper)
	{
		logger().trace("Invalid FovMap passed to fovUpdateVisMapFlags");
		return;
	}

	auto revealMapWrapper = getRevealMapWrapper(revealMap);
	if (!revealMapWrapper)
	{
		logger().trace("Invalid RevealMap passed to fovUpdateVisMapFlags");
		return;
	}

	auto lightMapWrapper = getLightMapWrapper(lightMap);
	if (!lightMapWrapper)
	{
		logger().trace("Invalid LightMap passed to updateLightmap");
		return;
	}

	updateLightMap(visMapWrapper, fovMapWrapper, revealMapWrapper, lightMapWrapper, factor);
}
