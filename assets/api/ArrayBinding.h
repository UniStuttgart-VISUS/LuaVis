#ifndef SRC_SHARED_LUA_BINDINGS_ARRAYBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_ARRAYBINDING_H_

#include <stdint.h>

#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

/**
 * Array item data type.
 */
typedef uint8_t wosC_array_item_t;

/**
 * Array pointer type.
 */
typedef wosC_array_item_t * wosC_array_t;

/**
 * Handle to an array context.
 */
typedef int32_t wosC_array_context_t;

/**
 * Numeric ID of an array.
 */
typedef int32_t wosC_array_id_t;

/**
 * Size type of an array.
 */
typedef int32_t wosC_array_size_t;

/**
 * Struct holding information about an array's data.
 */
typedef struct
{
	wosC_array_t data;
	wosC_array_size_t size;
} wosC_array_info_t;

/**
 * Struct uniquely identifying an array (used for deletion).
 */
typedef struct
{
	wosC_array_context_t context;
	wosC_array_id_t array;
} wosC_array_ref_t;

/**
 * Creates a new array and returns a pointer to it, or a null pointer on allocation failure.
 *
 * The context ID is used for automatic deallocation on reset.
 */
WOSC_API wosC_array_id_t wosC_array_new(wosC_array_context_t context, wosC_array_size_t size);

/**
 * Returns the info of an existing array by ID.
 */
WOSC_API wosC_array_info_t wosC_array_getArrayInfo(wosC_array_context_t context, wosC_array_id_t array);

/**
 * Deletes the specified array.
 */
WOSC_API void wosC_array_delete(wosC_array_ref_t reference);

/**
 * Acquires/relinquishes ownership of the array. This serves as a flag to prevent use-after-free problems.
 */
WOSC_API void wosC_array_setOwnership(wosC_array_context_t context, wosC_array_id_t array, bool own);

/**
 * Checks if ownership of the array has already been taken by an external function.
 */
WOSC_API bool wosC_array_isOwned(wosC_array_context_t context, wosC_array_id_t array);

}

#endif
