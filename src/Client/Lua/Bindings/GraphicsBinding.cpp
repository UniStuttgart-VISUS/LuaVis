#include <Client/Lua/Bindings/GraphicsBinding.hpp>
#include <vector>

namespace wosc
{

std::vector<Graphics *> boundGraphics;

constexpr wosC_gfx_transform_t wosc::Graphics::TRANSFORM_IDENTITY;
constexpr wosC_gfx_rectangle_t wosc::Graphics::RECTANGLE_NULL;

Graphics::~Graphics() noexcept
{
}

wosC_gfx_t bindGFX(Graphics & graphics)
{
	for (wosC_gfx_t i = 0; i < (wosC_gfx_t) boundGraphics.size(); ++i)
	{
		if (boundGraphics[i] == nullptr)
		{
			boundGraphics[i] = &graphics;
			return i;
		}
	}
	boundGraphics.push_back(&graphics);
	return boundGraphics.size() - 1;
}

Graphics & getGFX(wosC_gfx_t gfxID)
{
	return *boundGraphics[gfxID];
}

void unbindGFX(wosC_gfx_t gfxID)
{
	boundGraphics[gfxID] = nullptr;
	while (!boundGraphics.empty() && boundGraphics.back() == nullptr)
	{
		boundGraphics.pop_back();
	}
}

}

extern "C"
{
#define WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, PARAMETER_LIST, PARAMETER_NAMES) \
	RETURN_TYPE wosC_gfx_##FUNC_NAME(wosC_gfx_t gfxID PARAMETER_LIST)               \
	{                                                                               \
		wosc::Graphics & gfx = wosc::getGFX(gfxID);                                 \
		return gfx.FUNC_NAME(PARAMETER_NAMES);                                      \
	}

#define WOSC_COMMA ,

#define WOSC_GFX_GLUE_ARG0(RETURN_TYPE, FUNC_NAME) WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, , )
#define WOSC_GFX_GLUE_ARG1(RETURN_TYPE, FUNC_NAME, T1, N1) \
	WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, WOSC_COMMA T1 N1, N1)
#define WOSC_GFX_GLUE_ARG2(RETURN_TYPE, FUNC_NAME, T1, N1, T2, N2) \
	WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, WOSC_COMMA T1 N1 WOSC_COMMA T2 N2, N1 WOSC_COMMA N2)
#define WOSC_GFX_GLUE_ARG3(RETURN_TYPE, FUNC_NAME, T1, N1, T2, N2, T3, N3)                         \
	WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, WOSC_COMMA T1 N1 WOSC_COMMA T2 N2 WOSC_COMMA T3 N3, \
	                   N1 WOSC_COMMA N2 WOSC_COMMA N3)
#define WOSC_GFX_GLUE_ARG4(RETURN_TYPE, FUNC_NAME, T1, N1, T2, N2, T3, N3, T4, N4)                                  \
	WOSC_GFX_GLUE_IMPL(RETURN_TYPE, FUNC_NAME, WOSC_COMMA T1 N1 WOSC_COMMA T2 N2 WOSC_COMMA T3 N3 WOSC_COMMA T4 N4, \
	                   N1 WOSC_COMMA N2 WOSC_COMMA N3 WOSC_COMMA N4)

	WOSC_GFX_GLUE_ARG1(wosC_gfx_imageID_t, loadImage, const char *, name)
	WOSC_GFX_GLUE_ARG1(void, unloadImage, wosC_gfx_imageID_t, imageID)
	WOSC_GFX_GLUE_ARG1(bool, isImageLoaded, wosC_gfx_imageID_t, imageID)
	WOSC_GFX_GLUE_ARG1(wosC_gfx_imageGeometry_t, getImageGeometry, wosC_gfx_imageID_t, imageID)
	WOSC_GFX_GLUE_ARG0(wosC_gfx_vertexBuffer_t, newVertexBuffer)
	WOSC_GFX_GLUE_ARG1(void, deleteVertexBuffer, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG1(bool, isVertexBufferIDValid, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG1(wosC_gfx_vertexBuffer_size_t, getVertexBufferSize, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG1(void, clearVertexBuffer, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG2(void, setVertexBufferTexture, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_textureID_t,
	                   textureID)
	WOSC_GFX_GLUE_ARG1(wosC_gfx_textureID_t, getVertexBufferTexture, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG3(void, insertVertices, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, vertexCount)
	WOSC_GFX_GLUE_ARG3(void, removeVertices, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, vertexCount)
	WOSC_GFX_GLUE_ARG3(void, setVertex, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t, vertexOffset,
	                   wosC_gfx_vertex_t, vertex)
	WOSC_GFX_GLUE_ARG2(wosC_gfx_vertex_t, getVertex, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset)
	WOSC_GFX_GLUE_ARG4(void, copyVertices, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   sourceOffset, wosC_gfx_vertexBuffer_size_t, destOffset, wosC_gfx_vertexBuffer_size_t,
	                   vertexCount)
	WOSC_GFX_GLUE_ARG4(void, writeVertices, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, vertexCount, const wosC_gfx_vertex_t *, vertices)
	WOSC_GFX_GLUE_ARG4(void, readVertices, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, vertexCount, wosC_gfx_vertex_t *, vertices)
	WOSC_GFX_GLUE_ARG4(void, writeVertexTextureIDs, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, textureIDCount, const wosC_gfx_textureID_t *,
	                   textureIDs)
	WOSC_GFX_GLUE_ARG4(void, writeVertexZOrders, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, wosC_gfx_vertexBuffer_size_t, zOrderCount, const wosC_gfx_zOrder_t *, zOrders)
	WOSC_GFX_GLUE_ARG4(void, setVertexPosition, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, double, x, double, y)
	WOSC_GFX_GLUE_ARG3(void, setVertexColor, wosC_gfx_vertexBuffer_t, vbufferID, wosC_gfx_vertexBuffer_size_t,
	                   vertexOffset, double, color)
	WOSC_GFX_GLUE_ARG4(void, setVertexTextureCoordinate, wosC_gfx_vertexBuffer_t, vbufferID,
	                   wosC_gfx_vertexBuffer_size_t, vertexOffset, double, tx, double, ty)
	WOSC_GFX_GLUE_ARG2(void, setTransform, wosC_gfx_vertexBuffer_t, vbufferID, const wosC_gfx_transform_t *, transform)
	WOSC_GFX_GLUE_ARG1(const wosC_gfx_transform_t *, getTransform, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG2(void, setClippingRectangle, wosC_gfx_vertexBuffer_t, vbufferID, const wosC_gfx_rectangle_t *,
	                   rect)
	WOSC_GFX_GLUE_ARG1(const wosC_gfx_rectangle_t *, getClippingRectangle, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG1(void, sortBuffer, wosC_gfx_vertexBuffer_t, vbufferID)
	WOSC_GFX_GLUE_ARG3(void, mergeSortedBuffers, const wosC_gfx_vertexBuffer_t *, sourceIDs,
	                   wosC_gfx_vertexBuffer_size_t, sourceCount, wosC_gfx_vertexBuffer_t, targetID)
	WOSC_GFX_GLUE_ARG1(void, drawVertexBuffer, wosC_gfx_vertexBuffer_t, vbufferID)
}
