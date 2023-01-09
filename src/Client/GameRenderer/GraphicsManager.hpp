#ifndef SRC_CLIENT_GAMERENDERER_GRAPHICSMANAGER_HPP_
#define SRC_CLIENT_GAMERENDERER_GRAPHICSMANAGER_HPP_

#include <Client/GUI3/ResourceManager.hpp>
#include <Client/GUI3/Types.hpp>
#include <Client/Graphics/Text/AbstractFont.hpp>
#include <Client/Graphics/Text/Text.hpp>
#include <Client/Lua/Bindings/GraphicsBinding.h>
#include <Client/Lua/Bindings/GraphicsBinding.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gui3
{
class Widget;
class Application;
}

namespace sf
{
class RenderTarget;
class Font;
class Text;
}

namespace wos
{

class LocalGame;

class GraphicsManager : public wosc::Graphics, public sf::Drawable
{
public:
	using InjectionFunc = std::function<void(sf::RenderTarget &, sf::RenderStates)>;

	GraphicsManager(LocalGame & game);
	virtual ~GraphicsManager();

	GraphicsManager(const GraphicsManager &) = delete;
	GraphicsManager(GraphicsManager &&) = delete;
	GraphicsManager & operator=(const GraphicsManager &) = delete;
	GraphicsManager & operator=(GraphicsManager &&) = delete;

	void reset();

	void setArrayContext(wosc::ArrayContext * arrayContext);
	wosc::ArrayContext * getArrayContext() const;

	wosC_gfx_t getID() const;

	virtual wosC_gfx_imageID_t loadImage(const char * name) noexcept override;
	virtual void unloadImage(wosC_gfx_imageID_t imageID) noexcept override;
	virtual bool isImageLoaded(wosC_gfx_imageID_t imageID) const noexcept override;
	virtual wosC_gfx_imageGeometry_t getImageGeometry(wosC_gfx_imageID_t imageID) const noexcept override;

	virtual wosC_gfx_vertexBuffer_t newVertexBuffer() noexcept override;
	virtual void deleteVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept override;
	virtual bool isVertexBufferIDValid(wosC_gfx_vertexBuffer_t vbufferID) const noexcept override;

	virtual wosC_gfx_vertexBuffer_size_t getVertexBufferSize(wosC_gfx_vertexBuffer_t vbufferID) noexcept override;
	virtual void clearVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept override;

	virtual void setVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID,
	                                    wosC_gfx_textureID_t textureID) noexcept override;
	virtual wosC_gfx_textureID_t getVertexBufferTexture(wosC_gfx_vertexBuffer_t vbufferID) const noexcept override;

	virtual void insertVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            wosC_gfx_vertexBuffer_size_t vertexCount) noexcept override;
	virtual void removeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            wosC_gfx_vertexBuffer_size_t vertexCount) noexcept override;

	virtual void setVertex(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                       wosC_gfx_vertex_t vertex) noexcept override;
	virtual wosC_gfx_vertex_t getVertex(wosC_gfx_vertexBuffer_t vbufferID,
	                                    wosC_gfx_vertexBuffer_size_t vertexOffset) const noexcept override;
	virtual void copyVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t sourceOffset,
	                          wosC_gfx_vertexBuffer_size_t destOffset,
	                          wosC_gfx_vertexBuffer_size_t vertexCount) noexcept override;

	virtual void writeVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                           wosC_gfx_vertexBuffer_size_t vertexCount,
	                           const wosC_gfx_vertex_t * vertices) noexcept override;
	virtual void readVertices(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                          wosC_gfx_vertexBuffer_size_t vertexCount,
	                          wosC_gfx_vertex_t * vertices) const noexcept override;

	virtual void writeVertexTextureIDs(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                   wosC_gfx_vertexBuffer_size_t textureIDCount,
	                                   const wosC_gfx_textureID_t * textureIDs) noexcept override;
	virtual void writeVertexZOrders(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                                wosC_gfx_vertexBuffer_size_t zOrderCount,
	                                const wosC_gfx_zOrder_t * zOrders) noexcept override;

	virtual void setVertexPosition(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                               double x, double y) noexcept override;
	virtual void setVertexColor(wosC_gfx_vertexBuffer_t vbufferID, wosC_gfx_vertexBuffer_size_t vertexOffset,
	                            double color) noexcept override;
	virtual void setVertexTextureCoordinate(wosC_gfx_vertexBuffer_t vbufferID,
	                                        wosC_gfx_vertexBuffer_size_t vertexOffset, double tx,
	                                        double ty) noexcept override;

	virtual void setTransform(wosC_gfx_vertexBuffer_t vbufferID,
	                          const wosC_gfx_transform_t * transform) noexcept override;
	virtual const wosC_gfx_transform_t * getTransform(wosC_gfx_vertexBuffer_t vbufferID) const noexcept override;

	virtual void setClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID,
	                                  const wosC_gfx_rectangle_t * rect) noexcept override;
	virtual const wosC_gfx_rectangle_t *
	getClippingRectangle(wosC_gfx_vertexBuffer_t vbufferID) const noexcept override;

	virtual void sortBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept override;
	virtual void mergeSortedBuffers(const wosC_gfx_vertexBuffer_t * sourceIDs, wosC_gfx_vertexBuffer_size_t sourceCount,
	                                wosC_gfx_vertexBuffer_t targetID) noexcept override;

	virtual void drawVertexBuffer(wosC_gfx_vertexBuffer_t vbufferID) noexcept override;

	/**
	 * Needs to be called before "drawVertexBuffer" calls are made, to reset the drawing order buffer.
	 */
	void resetDrawState();

	/**
	 * Injects a one-off custom draw call to be made after a specific vertex buffer is rendered.
	 *
	 * TODO: This is a quick hack that only exists to allow rendering sf::Text. It will be replaced later.
	 */
	void inject(wosC_gfx_vertexBuffer_t vbufferID, InjectionFunc function);

	/**
	 * Loads a vector or bitmap font. If the font is a vector font, the `size` parameter determines the character size
	 * to load.
	 *
	 * If an error occurred, a null pointer is returned.
	 */
	std::shared_ptr<text::AbstractFont> acquireFont(const std::string & resourceName, unsigned int size);

	/**
	 * Settings structure containing data on how to render text.
	 */
	struct TextSettings
	{
		std::string font;
		sf::Vector2f position;
		sf::String text;
		unsigned int characterSize = 1;
		float size = 12;
		float sizeCorrection = 1.f;
		sf::Vector2f align;
		sf::Color fillColor;
		sf::Color outlineColor;
		float outlineThickness = 0;
		wosC_gfx_vertexBuffer_t vertexBuffer = 0;
		wosC_gfx_vertexBuffer_size_t vertexOffset = 0;
		sf::Color shadowColor;
		sf::Vector2f spacing;
		sf::Vector2f maxSize;
		bool clip = false;
		bool wordWrap = false;
		int maxLines = 0;
		bool noDraw = false;
		bool fixedWidth = false;
		bool useCache = false;
	};

	/**
	 * Renders text using the specified settings and returns the bounding box.
	 */
	const text::Text & drawText(TextSettings & settings);

	struct ImageLoadResult
	{
		bool success = false;
		wosc::ArrayContext::ArrayID pixels = -1;
		int width = 0;
		int height = 0;
	};

	/**
	 * Loads the specified image and returns a structure containing information on the loaded image.
	 *
	 * The pixels are stored inside of the array context associated with this graphics manager instance.
	 */
	ImageLoadResult getImagePixels(const std::string & resourceName) const;

	/**
	 * Takes a screenshot at the current render state and saves it to the file with the specified name.
	 */
	void takeScreenshot(
	    wosC_gfx_vertexBuffer_t vertexBuffer, const std::string & targetFile, sf::FloatRect rect, sf::Vector2i size);

	/**
	 * Creates a blank framebuffer with the specified size.
	 */
	wosC_gfx_imageID_t createFramebuffer(sf::Vector2u size);

	/**
	 * Updates a section of the specified framebuffer with pixels from the array context.
	 */
	bool updateFramebuffer(wosC_gfx_imageID_t image, sf::IntRect rect, wosc::ArrayContext::ArrayID pixels,
	                       std::size_t offset = 0);

	/**
	 * Updates a section of the specified framebuffer with pixels from the specified memory region.
	 */
	bool updateFramebuffer(wosC_gfx_imageID_t image, sf::IntRect rect, const sf::Uint8 * data);

	/**
	 * Renders the current intermediate frame to the screen.
	 */
	void displayCurrentFrame();

	/**
	 * Returns the time elapsed since the last frame was rendered to the screen.
	 */
	sf::Time getTimeSinceFrameStart() const;

	void preloadImage(const std::string & resourceName);
	int getAvailablePreloadCapacity() const;

private:
	using TextCacheKey = sf::Uint64;

	static constexpr std::size_t QUAD_SIZE = 6;

	template <typename T>
	struct TextCacheValue
	{
		std::unique_ptr<T> text = std::make_unique<T>();
		sf::Clock timeout;
	};

	struct VertexBufferDrawState
	{
		sf::Transform transform;
		sf::FloatRect clipRect = {0, 0, 1, 1};
		bool isClipped() const
		{
			return clipRect.left > 0 || clipRect.top > 0 || clipRect.width < 1 || clipRect.height < 1;
		}
	};

	struct VertexBufferDrawEntry
	{
		VertexBufferDrawState state;
		wosC_gfx_vertexBuffer_t bufferID;
	};

	struct VertexBuffer
	{
		using Index = uint32_t;

		VertexBuffer() = default;
		VertexBuffer(VertexBuffer &&) = default;
		VertexBuffer & operator=(VertexBuffer &&) = default;

		std::vector<sf::Vertex> vertices;
		VertexBufferDrawState drawState;

		// Texture pages can be specified uniformly for the whole buffer, or on a per-quad basis (6 vertices)
		wosC_gfx_textureID_t texturePage = INVALID_TEXTURE_ID;
		std::vector<wosC_gfx_textureID_t> textureIDs;

		// Z-order can be specified per-quad, and will be used for vertex sorting
		std::vector<wosC_gfx_zOrder_t> zOrderBuffer;
		std::vector<Index> sortedIndexBuffer;
		bool needVertexPermutation = false;

		bool valid = false;
		std::vector<InjectionFunc> injectionFuncs;

		bool compareBufferEntries(Index i1, Index i2) const;
		bool isEffectivelySorted() const;
		void applyIndexBuffer();
	};

	struct MergeRegion
	{
		MergeRegion() = default;
		MergeRegion(std::size_t offset, std::size_t size) :
			offset(offset),
			size(size)
		{
		}

		std::size_t offset = 0;
		std::size_t size = 0;
	};

	virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const override;
	void drawBuffer(const VertexBuffer & buffer, sf::RenderTarget & target, sf::RenderStates states) const;

	TextCacheKey getTextCacheKey(const TextSettings & settings) const;
	void cleanUpTextCache();

	gui3::Application * getApplication() const;

	const sf::Texture * getTexturePage(wosC_gfx_textureID_t pageID) const;

	LocalGame & game;
	wosC_gfx_t gfxID;
	wosc::ArrayContext * arrayContext = nullptr;
	std::vector<gui3::Ptr<gui3::res::Image>> images;
	std::vector<VertexBuffer> vertexBuffers;
	std::vector<VertexBufferDrawEntry> drawOrder;

	mutable wosC_gfx_transform_t transformReturnValue;
	mutable wosC_gfx_rectangle_t clipRectReturnValue;

	sf::Clock textCacheTimer;
	HashMap<TextCacheKey, TextCacheValue<text::Text>> textCache;

	Logger logger;
};

}

#endif
