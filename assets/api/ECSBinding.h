#ifndef SRC_SHARED_LUA_BINDINGS_ECSBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_ECSBINDING_H_

#include <stdint.h>

#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

/**
 * Handle to an entity component system.
 */
typedef int32_t wosC_ecs_t;

/**
 * Numeric ID of an entity type.
 */
typedef int32_t wosC_ecs_entityTypeID_t;

/**
 * Index of an entity within its entity list.
 */
typedef int32_t wosC_ecs_entityIndex_t;

/**
 * Unique ID for a specific entity within an ECS. Supports IDs of up to 2^53. Use integer values only. 0 = null entity.
 */
typedef double wosC_ecs_entityID_t;

/**
 * Unique ID for a specific field within an ECS.
 */
typedef int32_t wosC_ecs_fieldID_t;

/**
 * Offset of a specific field from the entity's base address. Cache this value for fast field access.
 */
typedef int32_t wosC_ecs_fieldOffset_t;

/**
 * Offset of a specific constant from the entity's base address. Cache this value for fast constant access.
 */
typedef int32_t wosC_ecs_constantOffset_t;

/**
 * Type of a field. Corresponds to the type enum values from Entities/Types.hpp.
 */
typedef int32_t wosC_ecs_fieldType_t;

/**
 * Semantics of a field. Corresponds to the semantics enum values from Entities/Types.hpp.
 */
typedef int32_t wosC_ecs_fieldSemantics_t;

/**
 * Mutability of a field. Corresponds to the mutability enum values from Entities/Types.hpp.
 */
typedef int32_t wosC_ecs_fieldMutability_t;

/**
 * Returns the ID of the field specified by name, or -1 if no such field exists.
 */
WOSC_API wosC_ecs_fieldID_t wosC_ecs_lookUpField(wosC_ecs_t ecsID, const char * componentName, const char * fieldName);

/**
 * Returns the type ID of the entity type specified by name, or -1 if no such entity type exists.
 */
WOSC_API wosC_ecs_entityTypeID_t wosC_ecs_lookUpEntityType(wosC_ecs_t ecsID, const char * entityTypeName);

/**
 * Returns the name of the specified entity type.
 */
WOSC_API const char * wosC_ecs_getEntityTypeName(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityType);

/**
 * Looks up if the specified entity type has a specific component.
 */
WOSC_API bool wosC_ecs_entityTypeHasComponent(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityType,
	const char * componentName);

/**
 * Returns the type of the specified field.
 *
 * Performs bounds checking and returns -1 on an invalid field.
 */
WOSC_API wosC_ecs_fieldType_t wosC_ecs_getFieldType(wosC_ecs_t ecsID, wosC_ecs_fieldID_t fieldID);

/**
 * Returns the semantics of the specified field.
 *
 * Performs bounds checking and returns -1 on an invalid field.
 */
WOSC_API wosC_ecs_fieldSemantics_t wosC_ecs_getFieldSemantics(wosC_ecs_t ecsID, wosC_ecs_fieldID_t fieldID);

/**
 * Returns the mutability of the specified field.
 *
 * Performs bounds checking and returns -1 on an invalid field.
 */
WOSC_API wosC_ecs_fieldMutability_t wosC_ecs_getFieldMutability(wosC_ecs_t ecsID, wosC_ecs_fieldID_t fieldID);

/**
 * Returns the offset of the specified field.
 *
 * Returns -1 on a constant or invalid field.
 */
WOSC_API wosC_ecs_fieldOffset_t wosC_ecs_getFieldOffset(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_fieldID_t fieldID);

/**
 * Returns the offset of the specified constant.
 *
 * Returns -1 on a mutable or invalid field.
 */
WOSC_API wosC_ecs_constantOffset_t wosC_ecs_getConstantOffset(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_fieldID_t fieldID);

/**
 * Creates an entity of the specified type and returns its ID.
 */
WOSC_API wosC_ecs_entityID_t wosC_ecs_spawnEntity(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID);

/**
 * Destroys the entity with the specified type.
 */
WOSC_API void wosC_ecs_despawnEntity(wosC_ecs_t ecsID, wosC_ecs_entityID_t entityID);

/**
 * Checks if the specified entity exists.
 */
WOSC_API bool wosC_ecs_entityExists(wosC_ecs_t ecsID, wosC_ecs_entityID_t entityID);

/**
 * Returns the entity type ID of the specified entity ID.
 *
 * Returns -1 on non-existent entity.
 */
WOSC_API wosC_ecs_entityTypeID_t wosC_ecs_getEntityType(wosC_ecs_t ecsID, wosC_ecs_entityID_t entityID);

/**
 * Returns the entity index belonging to the specified entity ID.
 *
 * Returns -1 on non-existent entity.
 */
WOSC_API wosC_ecs_entityIndex_t wosC_ecs_getEntityIndex(wosC_ecs_t ecsID, wosC_ecs_entityID_t entityID);

/**
 * Returns the index of the first entity in the specified entity list, for iteration begin.
 */
WOSC_API wosC_ecs_entityIndex_t wosC_ecs_getFirstEntity(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID);

/**
 * Returns the index of the next entity in the specified entity list, for iteration step.
 */
WOSC_API wosC_ecs_entityIndex_t wosC_ecs_getNextEntity(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex);

/**
 * Returns the index of the last entity in the specified entity list, for iteration end.
 */
WOSC_API wosC_ecs_entityIndex_t wosC_ecs_getLastEntity(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID);

/**
 * Returns the unique ID of the entity in the specified list at the specified index.
 */
WOSC_API wosC_ecs_entityID_t wosC_ecs_getEntityID(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex);

/**
 * Returns the value of the field with a specific type. The type must match exactly.
 */
WOSC_API bool wosC_ecs_getFieldBool(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldBool(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, bool value);

WOSC_API int8_t wosC_ecs_getFieldInt8(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldInt8(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int8_t value);

WOSC_API int16_t wosC_ecs_getFieldInt16(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldInt16(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int16_t value);

WOSC_API int32_t wosC_ecs_getFieldInt32(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldInt32(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int32_t value);

WOSC_API int64_t wosC_ecs_getFieldInt64(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldInt64(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int64_t value);

WOSC_API double wosC_ecs_getFieldFloat(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldFloat(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, double value);

WOSC_API const char * wosC_ecs_getFieldString(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API int32_t wosC_ecs_getFieldStringLength(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setFieldString(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, const char * value, int32_t length);

WOSC_API int32_t wosC_ecs_getRefListLength(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset);
WOSC_API void wosC_ecs_setRefListLength(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int32_t length);
WOSC_API void wosC_ecs_setRefListEntry(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int32_t index, wosC_ecs_entityID_t value);
WOSC_API wosC_ecs_entityID_t wosC_ecs_getRefListEntry(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, int32_t index);
WOSC_API void wosC_ecs_addSortedRefListEntry(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, wosC_ecs_entityID_t entityID);
WOSC_API void wosC_ecs_removeSortedRefListEntry(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_entityIndex_t entityIndex, wosC_ecs_fieldOffset_t fieldOffset, wosC_ecs_entityID_t entityID);

/**
 * Returns the value of the constant with a specific type. The type must match exactly.
 */
WOSC_API bool wosC_ecs_getConstantBool(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantBool(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, bool value);

WOSC_API int8_t wosC_ecs_getConstantInt8(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantInt8(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, int8_t value);

WOSC_API int16_t wosC_ecs_getConstantInt16(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantInt16(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, int16_t value);

WOSC_API int32_t wosC_ecs_getConstantInt32(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantInt32(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, int32_t value);

WOSC_API int64_t wosC_ecs_getConstantInt64(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantInt64(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, int64_t value);

WOSC_API double wosC_ecs_getConstantFloat(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantFloat(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, double value);

WOSC_API const char * wosC_ecs_getConstantString(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API int32_t wosC_ecs_getConstantStringLength(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset);
WOSC_API void wosC_ecs_setConstantString(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID,
	wosC_ecs_constantOffset_t constantOffset, const char * value, int32_t length);

// EXPERIMENTAL

WOSC_API char * wosC_ecs_getEntityFieldPointer(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID);
WOSC_API int32_t wosC_ecs_getEntityFieldByteCount(wosC_ecs_t ecsID, wosC_ecs_entityTypeID_t entityTypeID);

}

#endif
