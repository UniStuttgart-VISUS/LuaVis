#ifndef SRC_SHARED_LUA_BINDINGS_ACCEL_VISIONBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_ACCEL_VISIONBINDING_H_

#include <Shared/Lua/Bindings/ArrayBinding.h>
#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

	typedef int32_t wosC_accel_vision_tileCoord_t;
	typedef int32_t wosC_accel_vision_lightCoord_t;
	typedef int32_t wosC_accel_vision_lightIntensity_t;

	typedef uint32_t wosC_accel_vision_visMap_entry_t;
	typedef uint32_t wosC_accel_vision_fovMap_entry_t;
	typedef uint32_t wosC_accel_vision_revealMap_entry_t;
	typedef uint8_t wosC_accel_vision_lightValue_t;

	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetLight = 0;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitCountLight = 24;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetOpaque = 26;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetValid = 27;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetShadow = 28;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetPerspectiveReveal = 29;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetPerspectiveOcclude = 30;
	static const wosC_accel_vision_visMap_entry_t wosC_accel_vision_bitOffsetPerspectiveIlluminate = 31;

	static const wosC_accel_vision_lightCoord_t wosC_accel_vision_lightTileSize = 256;
	static const wosC_accel_vision_lightIntensity_t wosC_accel_vision_lightRevealThreshold = 7650;
	static const wosC_accel_vision_lightIntensity_t wosC_accel_vision_lightLevelMax = 25500;

	static const wosC_accel_vision_lightValue_t wosC_accel_vision_brightnessRevealed = 91;
	static const wosC_accel_vision_lightValue_t wosC_accel_vision_brightnessShadowed = 66;
	static const wosC_accel_vision_lightValue_t wosC_accel_vision_brightnessMax = 255;

	typedef struct
	{
		wosC_array_context_t arrayContext;
		wosC_array_id_t arrayID;

		wosC_accel_vision_tileCoord_t width;
		wosC_accel_vision_tileCoord_t height;

		wosC_accel_vision_tileCoord_t clipX;
		wosC_accel_vision_tileCoord_t clipY;
		wosC_accel_vision_tileCoord_t clipWidth;
		wosC_accel_vision_tileCoord_t clipHeight;
	} wosC_accel_vision_map_t;

	typedef wosC_accel_vision_map_t wosC_accel_vision_visMap_t;
	typedef wosC_accel_vision_map_t wosC_accel_vision_fovMap_t;
	typedef wosC_accel_vision_map_t wosC_accel_vision_revealMap_t;
	typedef wosC_accel_vision_map_t wosC_accel_vision_lightMap_t;

	WOSC_API void wosC_accel_vision_fovClear(wosC_accel_vision_fovMap_t * fovMap,
	                                         wosC_accel_vision_fovMap_entry_t mask);

	WOSC_API void wosC_accel_vision_fovAddRaycast(wosC_accel_vision_visMap_t * visMap,
	                                              wosC_accel_vision_fovMap_t * fovMap,
	                                              wosC_accel_vision_fovMap_entry_t mask,
	                                              wosC_accel_vision_tileCoord_t x, wosC_accel_vision_tileCoord_t y);

	WOSC_API void wosC_accel_vision_fovAddCircle(wosC_accel_vision_fovMap_t * fovMap,
	                                             wosC_accel_vision_fovMap_entry_t mask, wosC_accel_vision_tileCoord_t x,
	                                             wosC_accel_vision_tileCoord_t y,
	                                             wosC_accel_vision_lightCoord_t radius);

	WOSC_API void wosC_accel_vision_fovReveal(wosC_accel_vision_visMap_t * visMap, wosC_accel_vision_fovMap_t * fovMap,
	                                          wosC_accel_vision_revealMap_t * revealMap);

	WOSC_API void wosC_accel_vision_shadowClear(wosC_accel_vision_visMap_t * visMap);

	WOSC_API void wosC_accel_vision_shadowAddCircle(wosC_accel_vision_visMap_t * visMap,
	                                                wosC_accel_vision_tileCoord_t x, wosC_accel_vision_tileCoord_t y,
	                                                wosC_accel_vision_lightCoord_t radius);

	WOSC_API void wosC_accel_vision_shadowInvert(wosC_accel_vision_visMap_t * visMap);

	WOSC_API void wosC_accel_vision_perspectiveClear(wosC_accel_vision_visMap_t * visMap);

	WOSC_API void wosC_accel_vision_perspectiveFill(wosC_accel_vision_visMap_t * visMap);

	WOSC_API void wosC_accel_vision_perspectiveAddCircle(wosC_accel_vision_visMap_t * visMap,
	                                                     wosC_accel_vision_tileCoord_t x,
	                                                     wosC_accel_vision_tileCoord_t y,
	                                                     wosC_accel_vision_lightCoord_t radius);

	WOSC_API void wosC_accel_vision_lightApplyRadial(wosC_accel_vision_visMap_t * visMap,
	                                                 wosC_accel_vision_tileCoord_t x, wosC_accel_vision_tileCoord_t y,
	                                                 wosC_accel_vision_lightCoord_t innerRadius,
	                                                 wosC_accel_vision_lightCoord_t outerRadius,
	                                                 wosC_accel_vision_lightIntensity_t intensity);

	WOSC_API void wosC_accel_vision_updateLightMap(wosC_accel_vision_visMap_t * visMap,
	                                               wosC_accel_vision_fovMap_t * fovMap,
	                                               wosC_accel_vision_revealMap_t * revealMap,
	                                               wosC_accel_vision_lightMap_t * lightMap, float factor);
}

#endif
