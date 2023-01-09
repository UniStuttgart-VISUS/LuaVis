#ifndef SRC_CLIENT_LUA_BINDINGS_GRAPHICSBINDING_H_
#define SRC_CLIENT_LUA_BINDINGS_GRAPHICSBINDING_H_

#include <stdint.h>

#include <Shared/Lua/Bindings/BindingAPI.hpp>

extern "C"
{

	/**
	 * Handle to a graphics system.
	 */
	typedef int32_t wosC_gfx_t;

	/**
	 * Numeric ID of an "image", referring to a rectangular area on a texture containing data from a specific image
	 * file.
	 *
	 * Multiple different texture IDs may alias to the same area.
	 */
	typedef int32_t wosC_gfx_imageID_t;

	/**
	 * Numeric ID of a "texture page".
	 */
	typedef int32_t wosC_gfx_textureID_t;

	/**
	 * Per-vertex Z-index.
	 */
	typedef double wosC_gfx_zOrder_t;

	/**
	 * Data structure containing information on which texture an image is stored on, and where on the texture it is
	 * stored.
	 */
	typedef struct
	{
		double x;
		double y;
		double w;
		double h;
		wosC_gfx_textureID_t texture;
	} wosC_gfx_imageGeometry_t;

	/**
	 * Numeric ID of a vertex buffer.
	 */
	typedef int32_t wosC_gfx_vertexBuffer_t;

	/**
	 * Size type for vertex buffers.
	 */
	typedef int32_t wosC_gfx_vertexBuffer_size_t;

	typedef struct
	{
		float x;
		float y;
		int32_t color;
		float tx;
		float ty;
	} wosC_gfx_vertex_t;

	typedef struct
	{
		double x;
		double y;
		double w;
		double h;
	} wosC_gfx_rectangle_t;

	typedef struct
	{
		double matrix[9];
	} wosC_gfx_transform_t;


	WOSC_API wosC_gfx_imageID_t wosC_gfx_loadImage(wosC_gfx_t gfxID, const char * name);
	WOSC_API void wosC_gfx_unloadImage(wosC_gfx_t gfxID, wosC_gfx_imageID_t imageID);
	WOSC_API bool wosC_gfx_isImageLoaded(wosC_gfx_t gfxID, wosC_gfx_imageID_t imageID);
	WOSC_API wosC_gfx_imageGeometry_t wosC_gfx_getImageGeometry(wosC_gfx_t gfxID, wosC_gfx_imageID_t imageID);

	WOSC_API wosC_gfx_vertexBuffer_t wosC_gfx_newVertexBuffer(wosC_gfx_t gfxID);
	WOSC_API void wosC_gfx_deleteVertexBuffer(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);
	WOSC_API bool wosC_gfx_isVertexBufferIDValid(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);

	WOSC_API wosC_gfx_vertexBuffer_size_t wosC_gfx_getVertexBufferSize(wosC_gfx_t gfxID,
	                                                                   wosC_gfx_vertexBuffer_t vbufferID);
	WOSC_API void wosC_gfx_clearVertexBuffer(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);

	WOSC_API void wosC_gfx_setVertexBufferTexture(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                              wosC_gfx_textureID_t textureID);
	WOSC_API wosC_gfx_textureID_t wosC_gfx_getVertexBufferTexture(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);

	WOSC_API void wosC_gfx_insertVertices(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                      wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                      wosC_gfx_vertexBuffer_size_t vertexCount);
	WOSC_API void wosC_gfx_removeVertices(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                      wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                      wosC_gfx_vertexBuffer_size_t vertexCount);

	WOSC_API void wosC_gfx_setVertex(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                 wosC_gfx_vertexBuffer_size_t vertexOffset, wosC_gfx_vertex_t vertex);
	WOSC_API wosC_gfx_vertex_t wosC_gfx_getVertex(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                              wosC_gfx_vertexBuffer_size_t vertexOffset);
	WOSC_API void wosC_gfx_copyVertices(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                    wosC_gfx_vertexBuffer_size_t sourceOffset,
	                                    wosC_gfx_vertexBuffer_size_t destOffset,
	                                    wosC_gfx_vertexBuffer_size_t vertexCount);

	WOSC_API void wosC_gfx_writeVertices(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                     wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                     wosC_gfx_vertexBuffer_size_t vertexCount, const wosC_gfx_vertex_t * vertices);
	WOSC_API void wosC_gfx_readVertices(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                    wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                    wosC_gfx_vertexBuffer_size_t vertexCount, wosC_gfx_vertex_t * vertices);

	WOSC_API void wosC_gfx_writeVertexTextureIDs(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                             wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                             wosC_gfx_vertexBuffer_size_t textureIDCount,
	                                             const wosC_gfx_textureID_t * textureIDs);

	WOSC_API void wosC_gfx_writeVertexZOrders(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                          wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                          wosC_gfx_vertexBuffer_size_t zOrderCount,
	                                          const wosC_gfx_zOrder_t * zOrders);

	WOSC_API void wosC_gfx_setVertexPosition(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                         wosC_gfx_vertexBuffer_size_t vertexOffset, double x, double y);
	WOSC_API void wosC_gfx_setVertexColor(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                      wosC_gfx_vertexBuffer_size_t vertexOffset, double color);
	WOSC_API void wosC_gfx_setVertexTextureCoordinate(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                                  wosC_gfx_vertexBuffer_size_t vertexOffset, double tx, double ty);

	WOSC_API void wosC_gfx_setTransform(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                    const wosC_gfx_transform_t * transform);
	WOSC_API const wosC_gfx_transform_t * wosC_gfx_getTransform(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);

	WOSC_API void wosC_gfx_setClippingRectangle(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID,
	                                            const wosC_gfx_rectangle_t * rect);
	WOSC_API const wosC_gfx_rectangle_t * wosC_gfx_getClippingRectangle(wosC_gfx_t gfxID,
	                                                                    wosC_gfx_vertexBuffer_t vbufferID);

	WOSC_API void wosC_gfx_sortBuffer(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);
	WOSC_API void wosC_gfx_mergeSortedBuffers(wosC_gfx_t gfxID, const wosC_gfx_vertexBuffer_t * sourceIDs,
	                                          wosC_gfx_vertexBuffer_size_t sourceCount,
	                                          wosC_gfx_vertexBuffer_t targetID);

	WOSC_API void wosC_gfx_drawVertexBuffer(wosC_gfx_t gfxID, wosC_gfx_vertexBuffer_t vbufferID);
}

#endif
