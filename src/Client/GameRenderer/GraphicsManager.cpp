#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/Widget.hpp>
#include <Client/Game/LocalGame.hpp>
#include <Client/GameRenderer/GraphicsManager.hpp>
#include <Client/Graphics/UtilitiesSf.hpp>
#include <Client/System/WOSResourceManager.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Utils/ContainerUtils.hpp>
#include <Shared/Utils/Hash.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/MiscMath.hpp>
#include <Shared/Utils/VectorMul.hpp>
#include <algorithm>
#include <cstddef>
#include <iterator>

namespace wos
{
static cfg::Bool textureFiltering("wos.game.graphics.filterTextures");

constexpr std::size_t GraphicsManager::QUAD_SIZE;

GraphicsManager::GraphicsManager(LocalGame & game) : game(game), logger("GraphicsManager")
{
	gfxID = wosc::bindGFX(*this);
}

GraphicsManager::~GraphicsManager()
{
	wosc::unbindGFX(gfxID);
}

void GraphicsManager::reset()
{
	images.clear();
	vertexBuffers.clear();
	drawOrder.clear();
}

void GraphicsManager::setArrayContext(wosc::ArrayContext * arrayContext)
{
	this->arrayContext = arrayContext;
}

wosc::ArrayContext * GraphicsManager::getArrayContext() const
{
	return arrayContext;
}

wosC_gfx_t GraphicsManager::getID() const
{
	return gfxID;
}

wosC_gfx_imageID_t GraphicsManager::loadImage(const char * name) noexcept
{
	if (getApplication() == nullptr)
	{
		logger.error("Failed to load image '{}': not linked to application", name);
		return INVALID_IMAGE_ID;
	}

	auto image = name == nullptr ? nullptr : getApplication()->getResourceManager().acquireImage(name);

	if (image == nullptr)
	{
		logger.error("Failed to load image '{}': file not found or not a valid image", name);
		return INVALID_IMAGE_ID;
	}
	else
	{
		for (std::size_t i = 0; i < images.size(); ++i)
		{
			if (images[i] == nullptr)
			{
				images[i] = std::move(image);
				return i;
			}
		}
		images.push_back(std::move(image));
		return images.size() - 1;
	}
}

void GraphicsManager::unloadImage(wosC_gfx_imageID_t imageID) noexcept
{
	if (imageID >= 0 && (std::size_t) imageID < images.size())
	{
		images[imageID] = nullptr;
		while (!images.empty() && images.back() == nullptr)
		{
			images.pop_back();
		}
	}
	else
	{
		logger.warn("Attempt to unload invalid image with ID '{}'", imageID);
	}
}

bool GraphicsManager::isImageLoaded(wosC_gfx_imageID_t imageID) const noexcept
{
	return imageID >= 0 && (std::size_t) imageID < images.size() && images[imageID] != nullptr;
}

wosC_gfx_imageGeometry_t GraphicsManager::getImageGeometry(wosC_gfx_imageID_t imageID) const noexcept
{
	if (!isImageLoaded(imageID))
	{
		logger.warn("Attempt to get geometry of invalid image with ID '{}'", imageID);
		wosC_gfx_imageGeometry_t geometry = {};
		geometry.texture = INVALID_TEXTURE_ID;
		return geometry;
	}
	else
	{
		wosC_gfx_imageGeometry_t geometry;
		auto & image = *images[imageID];
		geometry.x = image.getTextureRect().left;
		geometry.y = image.getTextureRect().top;
		geometry.w = image.getTextureRect().width;
		geometry.h = image.getTextureRect().height;
		geometry.texture = image.getTexturePage();
		return geometry;
	}
}

wosC_gfx_vertexBuffer_t GraphicsManager::newVertexBuffer() noexcept
{
	VertexBuffer buffer;
	buffer.valid = true;
	for (std::size_t i = 0; i < vertexBuffers.size(); ++i)
	{
		if (!vertexBuffers[i].valid)
		{
			vertexBuffers[i] = std::move(buffer);
			return i;
		}
	}
	vertexBuffers.push_back(std::move(buffer));
	return vertexBuffers.size() - 1;
}

void GraphicsManager::deleteVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept
{
	if (vbufferID >= 0 && (std::size_t) vbufferID < vertexBuffers.size())
	{
		vertexBuffers[vbufferID] = VertexBuffer();
		while (!vertexBuffers.empty() && !vertexBuffers.back().valid)
		{
			vertexBuffers.pop_back();
		}
	}
}

bool GraphicsManager::isVertexBufferIDValid(wosC_gfx_vertexBuffer_t vbufferID) const noexcept
{
	return vbufferID >= 0 && (std::size_t) vbufferID < vertexBuffers.size() && vertexBuffers[vbufferID].valid;
}

wosC_gfx_vertexBuffer_size_t GraphicsManager::getVertexBufferSize(wosC_gfx_vertexBuffer_t vbufferID) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		return vertexBuffers[vbufferID].vertices.size();
	}
	else
	{
		logger.warn("Attempt to get size of invalid vertex buffer with ID '{}'", vbufferID);
		return 0;
	}
}

void GraphicsManager::clearVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		vertexBuffers[vbufferID].vertices.clear();
	}
	else
	{
		logger.warn("Attempt to clear invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::setVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_textureID_t textureID) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		vertexBuffers[vbufferID].texturePage = textureID;
	}
	else
	{
		logger.warn("Attempt to set texture of invalid vertex buffer with ID '{}'", vbufferID);
	}
}

wosC_gfx_textureID_t GraphicsManager::getVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID) const noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		return vertexBuffers[vbufferID].texturePage;
	}
	else
	{
		logger.warn("Attempt to get texture of invalid vertex buffer with ID '{}'", vbufferID);
		return INVALID_TEXTURE_ID;
	}
}

void GraphicsManager::insertVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                     wosC_gfx_vertexBuffer_size_t vertexCount) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset <= buffer.size())
		{
			buffer.insert(buffer.begin() + vertexOffset, vertexCount, sf::Vertex());
		}
		else
		{
			logger.warn("Out-of-range insertion of {} vertices at offset {} in vertex buffer with ID '{}' and size {}",
			            vertexCount, vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to insert vertices into invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::removeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                     wosC_gfx_vertexBuffer_size_t vertexCount) noexcept
{
	if (isVertexBufferIDValid(vbufferID) && vertexOffset >= 0 && vertexCount >= 0)
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset + (std::size_t) vertexCount <= buffer.size())
		{
			buffer.erase(buffer.begin() + vertexOffset, buffer.begin() + vertexOffset + vertexCount);
		}
		else
		{
			logger.warn("Out of range removal of  {} at offset {} in vertex buffer with ID '{}' and size {}",
			            vertexCount, vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to remove vertices from invalid vertex buffer with ID '{}'", vbufferID);
	}
}

// This is only necessary to compensate SFML's internal byte order swap within sf::Color's Uint32 c'tor/getter
static sf::Uint32 swapByteOrder(sf::Uint32 value)
{
	return (value >> 24) | ((value << 8) & 0x00FF0000) | ((value >> 8) & 0x0000FF00) | (value << 24);
}

void GraphicsManager::setVertex(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                wosC_gfx_vertex_t vertex) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset < buffer.size())
		{
			auto & bufferVertex = buffer[vertexOffset];
			bufferVertex.position = {(float) vertex.x, (float) vertex.y};
			bufferVertex.texCoords = {(float) vertex.tx, (float) vertex.ty};
			bufferVertex.color = sf::Color(swapByteOrder(vertex.color));
		}
		else
		{
			logger.warn("Attempt to set invalid vertex at {} in vertex buffer with ID '{}'", vertexOffset, vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to set vertex in invalid vertex buffer with ID '{}'", vbufferID);
	}
}

wosC_gfx_vertex_t GraphicsManager::getVertex(wosC_gfx_vertexBuffer_t vbufferID,
                                             wosC_gfx_vertexBuffer_size_t vertexOffset) const noexcept
{
	wosC_gfx_vertex_t vertex = {};

	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset < buffer.size())
		{
			auto & bufferVertex = buffer[vertexOffset];
			vertex.x = bufferVertex.position.x;
			vertex.y = bufferVertex.position.y;
			vertex.tx = bufferVertex.texCoords.x;
			vertex.ty = bufferVertex.texCoords.y;
			vertex.color = swapByteOrder(bufferVertex.color.toInteger());
		}
		else
		{
			logger.warn("Attempt to get invalid vertex at {} in vertex buffer with ID '{}'", vertexOffset, vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to get vertex from invalid vertex buffer with ID '{}'", vbufferID);
	}

	return vertex;
}

void GraphicsManager::copyVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t sourceOffset,
                                   wosC_gfx_vertexBuffer_size_t destOffset,
                                   wosC_gfx_vertexBuffer_size_t vertexCount) noexcept
{
	if (!isVertexBufferIDValid(vbufferID))
	{
		logger.warn("Attempt to perform vertex copy within invalid vertex buffer with ID '{}'", vbufferID);
	}
	else if (sourceOffset < 0 && destOffset < 0)
	{
		logger.warn("Attempt to copy invalid range of vertices (source '{}', dest '{}') in vertex buffer with ID '{}'",
		            sourceOffset, destOffset, vbufferID);
	}
	else if (vertexCount >= 0)
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) sourceOffset + vertexCount <= buffer.size()
		    && (std::size_t) destOffset + vertexCount <= buffer.size())
		{
			if (sourceOffset < destOffset)
			{
				std::copy_backward(buffer.begin() + sourceOffset, buffer.begin() + sourceOffset + vertexCount,
				                   buffer.begin() + destOffset + vertexCount);
			}
			else if (sourceOffset > destOffset)
			{
				std::copy(buffer.begin() + sourceOffset, buffer.begin() + sourceOffset + vertexCount,
				          buffer.begin() + destOffset);
			}
		}
		else
		{
			logger.warn("Out-of-range copy of {} vertices from {} to {} in vertex buffer with ID '{}' and size {}",
			            vertexCount, sourceOffset, destOffset, vbufferID, buffer.size());
		}
	}
}

void GraphicsManager::writeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                    wosC_gfx_vertexBuffer_size_t vertexCount,
                                    const wosC_gfx_vertex_t * vertices) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset <= buffer.size())
		{
			// Increase buffer size to accomodate extra vertices.
			buffer.resize(std::max<int64_t>(buffer.size(), vertexOffset + vertexCount));

			// We assume it's safe to memcpy vertex data directly, as sf::Vertex's layout must be well-defined for
			// the data to be usable by OpenGL.
			static_assert(sizeof(wosC_gfx_vertex_t) == sizeof(sf::Vertex), "Vertex size mismatch");
			std::memcpy(buffer.data() + vertexOffset, vertices, vertexCount * sizeof(wosC_gfx_vertex_t));
		}
		else
		{
			logger.warn("Out-of-range write of {} vertices to {} in vertex buffer with ID '{}' and size {}",
			            vertexCount, vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to write vertices to invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::readVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                   wosC_gfx_vertexBuffer_size_t vertexCount,
                                   wosC_gfx_vertex_t * vertices) const noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		const auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset + vertexCount <= buffer.size())
		{
			static_assert(sizeof(wosC_gfx_vertex_t) == sizeof(sf::Vertex), "Vertex size mismatch");
			std::memcpy(vertices, buffer.data() + vertexOffset, vertexCount * sizeof(wosC_gfx_vertex_t));
		}
		else
		{
			logger.warn("Out-of-range read of {} vertices from {} in vertex buffer with ID '{}' and size {}",
			            vertexCount, vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to read vertices from invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::writeVertexTextureIDs(wosC_gfx_vertexBuffer_t vbufferID,
                                            wosC_gfx_vertexBuffer_size_t vertexOffset,
                                            wosC_gfx_vertexBuffer_size_t textureIDCount,
                                            const wosC_gfx_textureID_t * textureIDs) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].textureIDs;
		if ((std::size_t) vertexOffset <= buffer.size())
		{
			// Increase buffer size to accomodate extra entries.
			buffer.resize(std::max<int64_t>(buffer.size(), vertexOffset + textureIDCount));
			std::memcpy(buffer.data() + vertexOffset, textureIDs, textureIDCount * sizeof(wosC_gfx_textureID_t));
		}
		else
		{
			logger.warn("Out-of-range write of {} textureIDs to {} in vertex buffer with ID '{}' and size {}",
			            textureIDCount, vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to write textureIDs to invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::writeVertexZOrders(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                         wosC_gfx_vertexBuffer_size_t zOrderCount,
                                         const wosC_gfx_zOrder_t * zOrders) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].zOrderBuffer;
		if ((std::size_t) vertexOffset <= buffer.size())
		{
			// Increase buffer size to accomodate extra entries.
			buffer.resize(std::max<int64_t>(buffer.size(), vertexOffset + zOrderCount));
			std::memcpy(buffer.data() + vertexOffset, zOrders, zOrderCount * sizeof(wosC_gfx_zOrder_t));
		}
		else
		{
			logger.warn("Out-of-range write of {} zOrders to {} in vertex buffer with ID '{}' and size {}", zOrderCount,
			            vertexOffset, vbufferID, buffer.size());
		}
	}
	else
	{
		logger.warn("Attempt to write zOrders to invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::setVertexPosition(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                        double x, double y) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset < buffer.size())
		{
			auto & bufferVertex = buffer[vertexOffset];
			bufferVertex.position = sf::Vector2f((float) x, (float) y);
		}
		else
		{
			logger.warn("Attempt to set position of invalid vertex at {} in vertex buffer with ID '{}'", vertexOffset,
			            vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to set vertex position in invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::setVertexColor(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
                                     double color) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset < buffer.size())
		{
			auto & bufferVertex = buffer[vertexOffset];
			bufferVertex.color = sf::Color(sf::Uint32(color));
		}
		else
		{
			logger.warn("Attempt to set color of invalid vertex at {} in vertex buffer with ID '{}'", vertexOffset,
			            vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to set vertex color in invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::setVertexTextureCoordinate(wosC_gfx_vertexBuffer_t vbufferID,
                                                 wosC_gfx_vertexBuffer_size_t vertexOffset, double tx,
                                                 double ty) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID].vertices;
		if ((std::size_t) vertexOffset < buffer.size())
		{
			auto & bufferVertex = buffer[vertexOffset];
			bufferVertex.texCoords = {(float) tx, (float) ty};
		}
		else
		{
			logger.warn("Attempt to set texture coordinates of invalid vertex at {} in vertex buffer with ID '{}'",
			            vertexOffset, vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to set vertex texture coordinates in invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::setTransform(wosC_gfx_vertexBuffer_t vbufferID, const wosC_gfx_transform_t * transform) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		vertexBuffers[vbufferID].drawState.transform =
		    sf::Transform(transform->matrix[0], transform->matrix[3], transform->matrix[6], transform->matrix[1],
		                  transform->matrix[4], transform->matrix[7], transform->matrix[2], transform->matrix[5],
		                  transform->matrix[8]);
	}
	else
	{
		logger.warn("Attempt to set transform of invalid vertex buffer with ID '{}'", vbufferID);
	}
}

const wosC_gfx_transform_t * GraphicsManager::getTransform(wosC_gfx_vertexBuffer_t vbufferID) const noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		const float * matrix = vertexBuffers[vbufferID].drawState.transform.getMatrix();
		transformReturnValue = {
		    {matrix[0], matrix[4], matrix[12], matrix[1], matrix[5], matrix[13], matrix[3], matrix[7], matrix[15]}};
	}
	else
	{
		logger.warn("Attempt to get transform of invalid vertex buffer with ID '{}'", vbufferID);
		transformReturnValue = TRANSFORM_IDENTITY;
	}

	return &transformReturnValue;
}

void GraphicsManager::setClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID,
                                           const wosC_gfx_rectangle_t * rect) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		vertexBuffers[vbufferID].drawState.clipRect = sf::FloatRect(rect->x, rect->y, rect->w, rect->h);
	}
	else
	{
		logger.warn("Attempt to set clipping rectangle of invalid vertex buffer with ID '{}'", vbufferID);
	}
}

const wosC_gfx_rectangle_t * GraphicsManager::getClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID) const noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & rect = vertexBuffers[vbufferID].drawState.clipRect;
		clipRectReturnValue = {rect.left, rect.top, rect.width, rect.height};
	}
	else
	{
		logger.warn("Attempt to get clipping rectangle of invalid vertex buffer with ID '{}'", vbufferID);
		clipRectReturnValue = RECTANGLE_NULL;
	}

	return &clipRectReturnValue;
}

void GraphicsManager::sortBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept
{
	using Index = VertexBuffer::Index;

	if (isVertexBufferIDValid(vbufferID))
	{
		auto & vbuffer = vertexBuffers[vbufferID];
		std::size_t quadCount = vbuffer.vertices.size() / QUAD_SIZE;
		if (vbuffer.zOrderBuffer.size() >= quadCount)
		{
			// Shrink Z-order buffer to fit
			vbuffer.zOrderBuffer.resize(quadCount);

			// Shrink texture page buffer to fit (TODO: do this elsewhere?)
			vbuffer.textureIDs.resize(std::min(vbuffer.textureIDs.size(), quadCount));

			auto & indexBuffer = vbuffer.sortedIndexBuffer;

			// Adjust index buffer (should contain each integer exactly once, up to its size)
			if (indexBuffer.size() > quadCount)
			{
				// Index buffer too big: remove superfluous entries
				ContainerUtils::removeIf(indexBuffer, [quadCount](Index entry) {
					return entry >= quadCount;
				});
			}
			else
			{
				// Index buffer too small: add extra entries
				while (indexBuffer.size() < quadCount)
				{
					indexBuffer.push_back(indexBuffer.size());
				}
			}

			// Check if index buffer already satisfies Z-ordering constraints
			if (!vbuffer.isEffectivelySorted())
			{
				// Sort index buffer to satisfy Z-ordering constraints
				std::sort(indexBuffer.begin(), indexBuffer.end(), [&vbuffer](Index i1, Index i2) {
					return vbuffer.compareBufferEntries(i1, i2);
				});

				// Index buffer now contains target vertex order, but vertices themselves are not sorted yet.
				vbuffer.needVertexPermutation = true;
			}
		}
		else
		{
			logger.warn("Attempt to sort vertex buffer with insufficient Z-order data (ID '{}')", vbufferID);
		}
	}
	else
	{
		logger.warn("Attempt to sort invalid vertex buffer with ID '{}'", vbufferID);
	}
}

void GraphicsManager::mergeSortedBuffers(const wosC_gfx_vertexBuffer_t * sourceIDs,
                                         wosC_gfx_vertexBuffer_size_t sourceCount,
                                         wosC_gfx_vertexBuffer_t targetID) noexcept
{
	using Index = VertexBuffer::Index;

	if (isVertexBufferIDValid(targetID))
	{
		auto & target = vertexBuffers[targetID];

		std::vector<MergeRegion> mergeRegions;
		std::vector<const VertexBuffer *> sourceBuffers;
		std::size_t quadCount = 0;

		for (wosC_gfx_vertexBuffer_size_t i = 0; i < sourceCount; ++i)
		{
			if (!isVertexBufferIDValid(sourceIDs[i]) || sourceIDs[i] == targetID)
			{
				logger.warn("Attempt to merge into invalid vertex buffer with ID '{}'", sourceIDs[i]);
				continue;
			}

			auto & source = vertexBuffers[sourceIDs[i]];
			auto bufferQuadCount = std::min(source.vertices.size() / QUAD_SIZE, source.sortedIndexBuffer.size());

			if (bufferQuadCount == 0)
			{
				continue;
			}

			mergeRegions.emplace_back(quadCount, bufferQuadCount);
			sourceBuffers.push_back(&source);
			quadCount += bufferQuadCount;
		}

		target.vertices.resize(quadCount * QUAD_SIZE);
		target.textureIDs.resize(quadCount);
		target.zOrderBuffer.resize(quadCount);
		target.sortedIndexBuffer.resize(quadCount);

		for (std::size_t i = 0; i < sourceBuffers.size(); ++i)
		{
			auto & source = *sourceBuffers[i];
			auto & region = mergeRegions[i];

			auto copyAppend = [&region](auto & src, auto & dst, std::size_t elemCount) {
				std::copy(src.begin(), src.begin() + std::min(src.size(), region.size * elemCount),
				          dst.begin() + region.offset * elemCount);
			};

			copyAppend(source.vertices, target.vertices, QUAD_SIZE);
			copyAppend(source.textureIDs, target.textureIDs, 1);
			copyAppend(source.zOrderBuffer, target.zOrderBuffer, 1);

			for (std::size_t j = 0; j < std::min(region.size, source.sortedIndexBuffer.size()); ++j)
			{
				target.sortedIndexBuffer[region.offset + j] = source.sortedIndexBuffer[j] + region.offset;
			}
		}

		// Merge sorted regions pairwise
		while (mergeRegions.size() > 1)
		{
			std::vector<MergeRegion> newMergeRegions;
			for (std::size_t i = 1; i < mergeRegions.size(); i += 2)
			{
				auto & left = mergeRegions[i - 1];
				auto & right = mergeRegions[i];
				auto it = target.sortedIndexBuffer.begin();
				std::inplace_merge(it + left.offset, it + left.offset + left.size, it + right.offset + right.size,
				                   [&target](Index i1, Index i2) {
					                   return target.compareBufferEntries(i1, i2);
				                   });
				newMergeRegions.emplace_back(left.offset, left.size + right.size);
			}

			// Add possible leftover region to the merge list
			if (mergeRegions.size() % 2 == 1)
			{
				newMergeRegions.push_back(mergeRegions.back());
			}

			mergeRegions = newMergeRegions;
		}

		// Index buffer now contains target vertex order, but vertices themselves are not sorted yet.
		target.needVertexPermutation = true;

		// TODO merge smallest ranges first
		// TODO apply transformation matrix if necessary
	}
	else
	{
		logger.warn("Attempt to merge into invalid vertex buffer with ID '{}'", targetID);
	}
}

void GraphicsManager::drawVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept
{
	if (isVertexBufferIDValid(vbufferID))
	{
		auto & buffer = vertexBuffers[vbufferID];

		// Permute buffers according to index buffer (if necessary)
		buffer.applyIndexBuffer();

		VertexBufferDrawEntry entry;
		entry.bufferID = vbufferID;
		entry.state = buffer.drawState;
		drawOrder.push_back(entry);
	}
}

void GraphicsManager::resetDrawState()
{
	drawOrder.clear();

	for (auto & buffer : vertexBuffers)
	{
		buffer.injectionFuncs.clear();
	}

	// Clean up text cache if necessary
	if (textCacheTimer.getElapsedTime() > sf::seconds(5))
	{
		textCacheTimer.restart();
		cleanUpTextCache();
	}
}

void GraphicsManager::inject(wosC_gfx_vertexBuffer_t vbufferID, InjectionFunc function)
{
	if (isVertexBufferIDValid(vbufferID))
	{
		vertexBuffers[vbufferID].injectionFuncs.push_back(function);
	}
	else
	{
		logger.warn("Attempt to inject custom draw call into invalid vertex buffer with ID '{}'", vbufferID);
	}
}

std::shared_ptr<text::AbstractFont> GraphicsManager::acquireFont(const std::string & resourceName, unsigned int size)
{
	if (!getApplication())
	{
		return nullptr;
	}

	// TODO add font resource to list
	auto font = getApplication()->getResourceManager().acquireFont(resourceName);

	return font ? font->getFont(size) : nullptr;
}

void GraphicsManager::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
	for (const auto & entry : drawOrder)
	{
		if (isVertexBufferIDValid(entry.bufferID))
		{
			auto & vbuffer = vertexBuffers[entry.bufferID];

			if (entry.state.isClipped())
			{
				auto clipRect = states.transform.transformRect(sf::FloatRect(
				    entry.state.clipRect.left * game.getSize().x, entry.state.clipRect.top * game.getSize().y,
				    entry.state.clipRect.width * game.getSize().x, entry.state.clipRect.height * game.getSize().y));

				drawClipped(DrawableWrapper([&](sf::RenderTarget & target, sf::RenderStates vbufferStates) {
					            vbufferStates.transform *= entry.state.transform;
					            drawBuffer(vbuffer, target, vbufferStates);
				            }),
				            target, states, clipRect);
			}
			else
			{
				sf::RenderStates vbufferStates = states;
				vbufferStates.transform *= entry.state.transform;
				drawBuffer(vbuffer, target, vbufferStates);
				for (const InjectionFunc & injectionFunc : vbuffer.injectionFuncs)
				{
					injectionFunc(target, states);
				}
			}
		}
		else
		{
			logger.warn("Attempt to draw invalid vertex buffer with ID '{}'", entry.bufferID);
		}
	}
}

void GraphicsManager::drawBuffer(const VertexBuffer & buffer, sf::RenderTarget & target, sf::RenderStates states) const
{
	std::size_t startIndex = 0;
	std::size_t quadCount = buffer.vertices.size() / QUAD_SIZE;

	if (getApplication() != nullptr)
	{
		if (buffer.textureIDs.size() >= quadCount && quadCount > 0)
		{
			// (Possibly) multiple texture pages: need to iterate and find texture differences
			auto texturePageID = buffer.textureIDs[0];
			states.texture = getApplication()->getTexture(texturePageID);

			for (std::size_t i = 0; i < quadCount; ++i)
			{
				if (texturePageID != buffer.textureIDs[i])
				{
					target.draw(buffer.vertices.data() + startIndex * QUAD_SIZE, (i - startIndex) * QUAD_SIZE,
					            sf::Triangles, states);
					texturePageID = buffer.textureIDs[i];
					startIndex = i;
					states.texture = getApplication()->getTexture(texturePageID);
				}
			}
		}
		else
		{
			// Single uniform texture
			states.texture = getApplication()->getTexture(buffer.texturePage);
		}
	}

	// Draw final set of vertices
	target.draw(
	    buffer.vertices.data() + startIndex * QUAD_SIZE, (quadCount - startIndex) * QUAD_SIZE, sf::Triangles, states);
}

GraphicsManager::TextCacheKey GraphicsManager::getTextCacheKey(const TextSettings & settings) const
{
	// TODO do this more elegantly
	return hash::dataHash64((const char *) settings.text.getData(), settings.text.getSize() * 4)
	       ^ hash::dataHash64(settings.font.data(), settings.font.size());
}

void GraphicsManager::cleanUpTextCache()
{
	for (auto it = textCache.begin(); it != textCache.end();)
	{
		if (it->second.timeout.getElapsedTime() > sf::seconds(5))
		{
			it = textCache.erase(it);
		}
		else
		{
			++it;
		}
	}
}

gui3::Application * GraphicsManager::getApplication() const
{
	return game.getParentApplication();
}

const text::Text & GraphicsManager::drawText(TextSettings & settings)
{
	auto & textCacheEntry = textCache[settings.useCache ? getTextCacheKey(settings) : 0];
	textCacheEntry.timeout.restart();

	auto & text = *textCacheEntry.text;

	auto font = acquireFont(settings.font, settings.characterSize);
	if (font == nullptr)
	{
		return text;
	}

	if (settings.maxLines > 0)
	{
		settings.maxSize.y = std::max(settings.maxSize.y, settings.maxLines * (settings.size + settings.spacing.y));
	}

	text.setFont(font);
	text.setString(settings.text);
	text.setPosition(settings.position);
	text.setScale(settings.size / font->getSize(), settings.size / font->getSize());
	text.setSizeCorrection(settings.sizeCorrection);
	text.setMaximumSize(settings.maxSize / text.getScale());
	text.setWordWrapEnabled(settings.wordWrap);
	text.setSpacing(settings.spacing);
	text.setAlignment(settings.align);
	// TODO reimplement fixed width mode (possibly as a font modifier)
	// text.setFixedWidth(settings.fixedWidth);

	if (!settings.noDraw && !settings.text.isEmpty() && isVertexBufferIDValid(settings.vertexBuffer))
	{
		auto & bufferVertices = vertexBuffers[settings.vertexBuffer].vertices;
		auto & bufferTextureIDs = vertexBuffers[settings.vertexBuffer].textureIDs;

		auto addVertices = [&]() {
			const auto & vertices = text.getVertices();
			const auto & textureIDs = text.getTextureIDs();

			bufferVertices.resize(std::max<int64_t>(bufferVertices.size(), settings.vertexOffset + vertices.size()));
			std::memcpy(bufferVertices.data() + settings.vertexOffset, vertices.data(),
			            vertices.size() * sizeof(sf::Vertex));

			bufferTextureIDs.resize(
			    std::max<int64_t>(bufferTextureIDs.size(), textureIDs.size() + settings.vertexOffset / QUAD_SIZE));
			std::memcpy(bufferTextureIDs.data() + settings.vertexOffset / QUAD_SIZE, textureIDs.data(),
			            textureIDs.size() * sizeof(wosC_gfx_textureID_t));

			settings.vertexOffset += vertices.size();
		};

		if (settings.shadowColor.a > 0)
		{
			text.setIconDisplayEnabled(true);
			text.setColorModifierEnabled(false);
			text.setColor(settings.shadowColor);
			text.move(settings.outlineThickness * text.getMaximumSizeScaleFactor());
			addVertices();
		}

		if (settings.outlineColor.a > 0 && settings.outlineThickness > 0)
		{
			text.setColorModifierEnabled(false);
			text.setIconDisplayEnabled(false);
			text.setColor(settings.outlineColor);
			for (int i = 0; i < 9; ++i)
			{
				if (i == 4)
					continue;
				auto offset = sf::Vector2f(i % 3 - 1, i / 3 - 1) * settings.outlineThickness;
				text.setPosition(settings.position + text.getMaximumSizeScaleFactor() * offset);
				addVertices();
			}
		}

		text.setPosition(settings.position);
		text.setIconDisplayEnabled(true);
		text.setColorModifierEnabled(true);
		text.setColor(settings.fillColor);
		addVertices();
	}

	return text;
}

GraphicsManager::ImageLoadResult GraphicsManager::getImagePixels(const std::string & resourceName) const
{
	ImageLoadResult result;

	if (!arrayContext)
	{
		logger.warn("Attempt to get image pixels without valid array context");
		return result;
	}

	auto data = getApplication()->getResourceManager().acquireData(resourceName);

	if (data)
	{
		sf::Image image;

		if (::loadImage((const sf::Uint8 *) data->getData(), data->getDataSize(), image))
		{
			auto size = image.getSize().x * image.getSize().y * 4;

			result.success = true;
			result.width = image.getSize().x;
			result.height = image.getSize().y;
			result.pixels = arrayContext->newArray(size);

			std::memcpy(arrayContext->getArrayInfo(result.pixels).data, image.getPixelsPtr(), size);

			return result;
		}
	}

	return result;
}

void GraphicsManager::takeScreenshot(
    wosC_gfx_vertexBuffer_t vertexBuffer, const std::string & targetFile, sf::FloatRect rect, sf::Vector2i size)
{
	if (size.x <= 0 || size.y <= 0)
	{
		logger.warn("Attempt to take screenshot with invalid size {}x{}", size.x, size.y);
		return;
	}

	inject(vertexBuffer,
	    [this, targetFile, rect, size](sf::RenderTarget & target, sf::RenderStates states)
	    {
		    sf::Image image;

		    // Correct for absolute position of game widget on render target
		    sf::FloatRect inputRect = states.transform.transformRect(rect);

		    if (sf::RenderWindow * renderWindow = dynamic_cast<sf::RenderWindow *>(&target))
		    {
			    sf::Texture screenTexture;
			    if (screenTexture.create(target.getSize().x, target.getSize().y))
			    {
				    screenTexture.update(*renderWindow);
				    image = screenTexture.copyToImage();
			    }
			    else
			    {
				    logger.warn("Failed to take screenshot (could not create texture)");
				    return;
			    }
		    }
		    else if (sf::RenderTexture * renderTexture = dynamic_cast<sf::RenderTexture *>(&target))
		    {
			    image = renderTexture->getTexture().copyToImage();
		    }
		    else
		    {
			    logger.warn("Failed to take screenshot (unsupported RenderTarget)");
			    return;
		    }

		    sf::Image cropped;
		    cropped.create(size.x, size.y);

		    int maxX = image.getSize().x - 1;
		    int maxY = image.getSize().y - 1;

		    for (int y = 0; y < size.y; ++y)
		    {
			    for (int x = 0; x < size.x; ++x)
			    {
				    int srcX = std::floor((double(x) / size.x) * inputRect.width + inputRect.left + 0.5);
				    int srcY = std::floor((double(y) / size.y) * inputRect.height + inputRect.top + 0.5);
				    cropped.setPixel(x, y, image.getPixel(clamp(0, srcX, maxX), clamp(0, srcY, maxY)));
			    }
		    }

		    if (!cropped.saveToFile(targetFile))
		    {
			    logger.warn("Failed to take screenshot (failed to save file '{}')", targetFile);
		    }
	    });
}

wosC_gfx_imageID_t GraphicsManager::createFramebuffer(sf::Vector2u size)
{
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&game.getParentApplication()->getResourceManager()))
	{
		auto framebuffer = resourceManager->createFramebuffer(size);

		if (framebuffer != nullptr)
		{
			for (std::size_t i = 0; i < images.size(); ++i)
			{
				if (images[i] == nullptr)
				{
					images[i] = std::move(framebuffer);
					return i;
				}
			}
			images.push_back(std::move(framebuffer));
			return images.size() - 1;
		}
	}

	logger.error("Failed to create framebuffer");
	return INVALID_IMAGE_ID;
}

bool GraphicsManager::updateFramebuffer(wosC_gfx_imageID_t image, sf::IntRect rect, wosc::ArrayContext::ArrayID pixels,
                                        std::size_t offset)
{
	if (!arrayContext)
	{
		logger.warn("Attempt to update framebuffer without valid array context");
		return false;
	}

	auto arrayInfo = arrayContext->getArrayInfo(pixels);

	if (arrayInfo.data == nullptr)
	{
		logger.warn("Invalid array specified for framebuffer update");
		return false;
	}

	if (arrayInfo.size - offset < rect.width * rect.height * 4)
	{
		logger.warn("Pixel array size ({}) is too small for framebuffer update region ({}x{})", arrayInfo.size - offset,
		            rect.width, rect.height);
		return false;
	}

	return updateFramebuffer(image, rect, arrayInfo.data + offset);
}

bool GraphicsManager::updateFramebuffer(wosC_gfx_imageID_t image, sf::IntRect rect, const sf::Uint8 * data)
{
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&game.getParentApplication()->getResourceManager()))
	{
		if (isImageLoaded(image))
		{
			return resourceManager->updateFramebuffer(images[image], rect, data);
		}
		else
		{
			logger.warn("Attempt to update non-existent framebuffer");
		}
	}
	return false;
}

void GraphicsManager::displayCurrentFrame()
{
	if (auto interface = game.getParentInterface())
	{
		// Update all transformation matrices
		for (auto & drawEntry : drawOrder)
		{
			if (isVertexBufferIDValid(drawEntry.bufferID))
			{
				auto & buffer = vertexBuffers[drawEntry.bufferID];
				drawEntry.state.transform = buffer.drawState.transform;
			}
		}

		interface->display();
	}
}

sf::Time GraphicsManager::getTimeSinceFrameStart() const
{
	if (auto application = game.getParentApplication())
	{
		return application->getTimeSinceFrameStart();
	}
	else
	{
		return sf::Time::Zero;
	}
}

void GraphicsManager::preloadImage(const std::string & resourceName)
{
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&game.getParentApplication()->getResourceManager()))
	{
		resourceManager->preloadData(resourceName, game.getThreadPool());
	}
}

int GraphicsManager::getAvailablePreloadCapacity() const
{
	if (auto resourceManager = dynamic_cast<WOSResourceManager *>(&game.getParentApplication()->getResourceManager()))
	{
		return static_cast<int>(game.getThreadPool().getThreadCount()) - resourceManager->getPendingAsyncLoads();
	}
	return 0;
}

bool GraphicsManager::VertexBuffer::compareBufferEntries(Index i1, Index i2) const
{
	if (i1 >= zOrderBuffer.size())
	{
		return false;
	}
	else if (i2 >= zOrderBuffer.size())
	{
		return true;
	}
	else if (zOrderBuffer[i1] != zOrderBuffer[i2])
	{
		return zOrderBuffer[i1] < zOrderBuffer[i2];
	}
	else
	{
		return i1 < i2;
	}
}

bool GraphicsManager::VertexBuffer::isEffectivelySorted() const
{
	for (std::size_t i = 1; i < sortedIndexBuffer.size(); ++i)
	{
		if (compareBufferEntries(sortedIndexBuffer[i], sortedIndexBuffer[i - 1]))
		{
			return false;
		}
	}

	return true;
}

void GraphicsManager::VertexBuffer::applyIndexBuffer()
{
	if (!needVertexPermutation)
	{
		return;
	}

	auto permute = [&](auto & data, std::size_t elemCount) {
		auto dataSource = data;
		auto & ibuf = sortedIndexBuffer;
		std::size_t limit = std::min(ibuf.size(), dataSource.size() / elemCount);
		for (std::size_t i = 0; i < limit; ++i)
		{
			auto index = ibuf[i];
			if ((index + 1) * elemCount <= dataSource.size())
			{
				for (std::size_t j = 0; j < elemCount; ++j)
				{
					data[i * elemCount + j] = dataSource[index * elemCount + j];
				}
			}
		}
	};

	permute(vertices, QUAD_SIZE);
	permute(textureIDs, 1);
	permute(zOrderBuffer, 1);

	// Clear the index buffer to force it to be rebuilt as a sequence upon recursive merge.
	sortedIndexBuffer.clear();

	needVertexPermutation = false;
}

}
