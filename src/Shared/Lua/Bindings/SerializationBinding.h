#ifndef SRC_SHARED_LUA_BINDINGS_SERIALIZATIONBINDING_H_
#define SRC_SHARED_LUA_BINDINGS_SERIALIZATIONBINDING_H_

#include <stdint.h>

#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

/**
 * Handle to a serialization context.
 */
typedef int32_t wosC_serial_t;

/**
 * Handle to a serialization buffer within a context.
 */
typedef int32_t wosC_serial_buffer_t;


/**
 * Creates a new serialization buffer and returns its ID. Returns a negative value if buffer allocations fails.
 */
WOSC_API wosC_serial_buffer_t wosC_serial_newBuffer(wosC_serial_t serialID);

/**
 * Deletes the specified serialization buffer.
 */
WOSC_API void wosC_serial_deleteBuffer(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);

/**
 * Checks if the specified serialization buffer exists.
 */
WOSC_API bool wosC_serial_bufferExists(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);


/**
 * Returns the maximum bit count per serialization buffer.
 */
WOSC_API int32_t wosC_serial_getMaxBufferSize(wosC_serial_t serialID);


/**
 * Sets the current buffer size in bits. This happens automatically as data is written to the buffer.
 */
WOSC_API void wosC_serial_setBufferSize(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t size);

/**
 * Returns the current buffer size in bits.
 */
WOSC_API int32_t wosC_serial_getBufferSize(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);


/**
 * Seeks to the specified bit position within the buffer.
 */
WOSC_API void wosC_serial_setBufferPosition(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t location);

/**
 * Returns the current buffer position in bits.
 */
WOSC_API int32_t wosC_serial_getBufferPosition(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);


/**
 * Checks if the specified serialization buffer is valid (the read pointer is less than or equal to the buffer size).
 */
WOSC_API bool wosC_serial_isBufferValid(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);

/**
 * Checks if the end of the specified serialization buffer has been reached (the read pointer equals the buffer size).
 */
WOSC_API bool wosC_serial_isBufferEndReached(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);


/**
 * Writes the specified number to the buffer as a signed integer with varying bit length.
 */
WOSC_API void wosC_serial_writeInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, double number);

/**
 * Writes the specified number to the buffer as an unsigned integer with varying bit length.
 */
WOSC_API void wosC_serial_writeUnsignedInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, double number);

/**
 * Writes the specified boolean to the buffer using a single bit.
 */
WOSC_API void wosC_serial_writeBoolean(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, bool value);

/**
 * Writes the specified string to the buffer.
 *
 * For variable-length strings, the length needs to be written separately as a prefix.
 */
WOSC_API void wosC_serial_writeString(wosC_serial_t serialID, wosC_serial_buffer_t bufferID,
	const char * string, int32_t length);


/**
 * Reads the specified signed integer from the buffer.
 */
WOSC_API double wosC_serial_readInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);

/**
 * Reads the specified unsigned integer from the buffer.
 */
WOSC_API double wosC_serial_readUnsignedInteger(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);

/**
 * Reads the specified boolean from the buffer.
 */
WOSC_API bool wosC_serial_readBoolean(wosC_serial_t serialID, wosC_serial_buffer_t bufferID);

/**
 * Reads the specified string with the specified length from the buffer.
 *
 * The pointer returned by this function may be invalidated by any other operation on the serialization context.
 */
WOSC_API const char * wosC_serial_readString(wosC_serial_t serialID, wosC_serial_buffer_t bufferID, int32_t length);

}

#endif
