#ifndef SRC_CLIENT_LUA_BINDINGS_GRAPHICSBINDING_HPP_
#define SRC_CLIENT_LUA_BINDINGS_GRAPHICSBINDING_HPP_

#include <Client/Lua/Bindings/GraphicsBinding.h>

namespace wosc
{

class Graphics
{
public:
	static constexpr wosC_gfx_imageID_t INVALID_IMAGE_ID = -1;
	static constexpr wosC_gfx_textureID_t INVALID_TEXTURE_ID = -1;

	static constexpr wosC_gfx_transform_t TRANSFORM_IDENTITY = {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
	static constexpr wosC_gfx_rectangle_t RECTANGLE_NULL = {};

	virtual ~Graphics() noexcept;

	virtual wosC_gfx_imageID_t loadImage(const char * name) noexcept = 0;
	virtual void unloadImage(wosC_gfx_imageID_t imageID) noexcept = 0;
	virtual bool isImageLoaded(wosC_gfx_imageID_t imageID) const noexcept = 0;
	virtual wosC_gfx_imageGeometry_t getImageGeometry(wosC_gfx_imageID_t imageID) const noexcept = 0;

	virtual wosC_gfx_vertexBuffer_t newVertexBuffer() noexcept = 0;
	virtual void deleteVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept = 0;
	virtual bool isVertexBufferIDValid(wosC_gfx_vertexBuffer_t vbufferID) const noexcept = 0;

	virtual wosC_gfx_vertexBuffer_size_t getVertexBufferSize(wosC_gfx_vertexBuffer_t vbufferID) noexcept = 0;
	virtual void clearVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept = 0;

	virtual void setVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_textureID_t textureID) noexcept = 0;
	virtual wosC_gfx_textureID_t getVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID) const noexcept = 0;

	virtual void insertVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            wosC_gfx_vertexBuffer_size_t vertexCount) noexcept = 0;
	virtual void removeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            wosC_gfx_vertexBuffer_size_t vertexCount) noexcept = 0;

	virtual void setVertex(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                       wosC_gfx_vertex_t vertex) noexcept = 0;
	virtual wosC_gfx_vertex_t getVertex(wosC_gfx_vertexBuffer_t vbufferID,
	                                    wosC_gfx_vertexBuffer_size_t vertexOffset) const noexcept = 0;

	virtual void copyVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t sourceOffset,
	                          wosC_gfx_vertexBuffer_size_t destOffset,
	                          wosC_gfx_vertexBuffer_size_t vertexCount) noexcept = 0;

	virtual void writeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                           wosC_gfx_vertexBuffer_size_t vertexCount,
	                           const wosC_gfx_vertex_t * vertices) noexcept = 0;
	virtual void readVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                          wosC_gfx_vertexBuffer_size_t vertexCount,
	                          wosC_gfx_vertex_t * vertices) const noexcept = 0;

	virtual void writeVertexTextureIDs(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                   wosC_gfx_vertexBuffer_size_t textureIDCount,
	                                   const wosC_gfx_textureID_t * textureIDs) noexcept = 0;
	virtual void writeVertexZOrders(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                wosC_gfx_vertexBuffer_size_t zOrderCount,
	                                const wosC_gfx_zOrder_t * zOrders) noexcept = 0;

	virtual void setVertexPosition(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                               double x, double y) noexcept = 0;
	virtual void setVertexColor(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            double color) noexcept = 0;
	virtual void setVertexTextureCoordinate(wosC_gfx_vertexBuffer_t vbufferID,
	                                        wosC_gfx_vertexBuffer_size_t vertexOffset, double tx,
	                                        double ty) noexcept = 0;

	virtual void setTransform(wosC_gfx_vertexBuffer_t vbufferID, const wosC_gfx_transform_t * transform) noexcept = 0;
	virtual const wosC_gfx_transform_t * getTransform(wosC_gfx_vertexBuffer_t vbufferID) const noexcept = 0;

	virtual void setClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID,
	                                  const wosC_gfx_rectangle_t * rect) noexcept = 0;
	virtual const wosC_gfx_rectangle_t * getClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID) const noexcept = 0;

	virtual void sortBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept = 0;
	virtual void mergeSortedBuffers(const wosC_gfx_vertexBuffer_t * sourceIDs, wosC_gfx_vertexBuffer_size_t sourceCount,
	                                wosC_gfx_vertexBuffer_t targetID) noexcept = 0;

	virtual void drawVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept = 0;
};

/**
 * Binds a numeric GFX handle to the specified graphics instance and returns it.
 */
wosC_gfx_t bindGFX(Graphics & graphics);

/**
 * Returns the GFX bound to the specified handle.
 */
Graphics & getGFX(wosC_gfx_t gfxID);

/**
 * Unbinds an existing numeric GFX handle.
 */
void unbindGFX(wosC_gfx_t gfxID);

}

#endif
