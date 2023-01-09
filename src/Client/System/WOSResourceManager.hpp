#ifndef SRC_CLIENT_SYSTEM_WOSRESOURCEMANAGER_HPP_
#define SRC_CLIENT_SYSTEM_WOSRESOURCEMANAGER_HPP_

#include <Client/GUI3/ResourceManager.hpp>
#include <Client/GUI3/Types.hpp>
#include <Client/Graphics/Text/BitmapFont.hpp>
#include <Client/Graphics/Text/MultiFont.hpp>
#include <Client/Graphics/Text/VectorFont.hpp>
#include <Client/Graphics/TexturePacker.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/ResourceHolder.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fs
{
class DirectoryObserver;
}

struct BitmapFont;
class ThreadPool;

namespace sf
{
class Texture;
class Image;
}

class WOSResourceManager : public gui3::res::ResourceManager
{
public:
	static const std::string IMAGE_NAME_WHITE_PIXEL;
	static const std::string IMAGE_PREFIX_FRAMEBUFFER;

	struct TextureAllocation
	{
		// TODO actually use page hint
		std::size_t pageHint = 0;
		bool useSingleTexture = false;
	};

	using TextureAllocator = std::function<TextureAllocation(const std::string & name, const sf::Image & image)>;

	WOSResourceManager();
	virtual ~WOSResourceManager();

	/**
	 * Specifies the source from which to acquire resources.
	 */
	void setSource(gui3::Ptr<res::AbstractSource> source);

	/**
	 * Returns the source from which resources are acquired.
	 */
	virtual gui3::Ptr<res::AbstractSource> getSource() const override;

	/**
	 * Gets a handle to the specified data file. Returns a null pointer if no such data was found.
	 */
	virtual gui3::Ptr<gui3::res::Data> acquireData(std::string dataName) override;

	/**
	 * Gets a handle to the specified image. Returns a null pointer if the image failed to load.
	 */
	virtual gui3::Ptr<gui3::res::Image> acquireImage(std::string imageName) override;

	/**
	 * Gets a handle to the specified font. Returns a null pointer if the font failed to load.
	 */
	virtual gui3::Ptr<gui3::res::Font> acquireFont(std::string fontName) override;

	/**
	 * Returns a list of all resources with the specified prefix.
	 */
	virtual std::vector<std::string> getResourceList(std::string prefix) const override;

	/**
	 * Registers a callback function to be executed when the specified resource is reloaded.
	 */
	virtual CallbackHandle<gui3::ResourceEvent> addResourceCallback(std::function<void(gui3::ResourceEvent)> callback,
	                                                                gui3::ResourceEvent::Type typeFilter,
	                                                                std::string resourceFilter, int order) override;

	/**
	 * Returns one of the textures containing loaded images.
	 */
	const sf::Texture * getTexture(std::size_t page) const;

	/**
	 * Enables/disables bilinear filtering for all textures.
	 */
	void setTextureFilteringEnabled(bool enabled);

	/**
	 * Checks if bilinear filtering is enabled.
	 */
	bool isTextureFilteringEnabled() const;

	/**
	 * Creates a blank framebuffer image with the specified size, which can be updated dynamically.
	 */
	gui3::Ptr<gui3::res::Image> createFramebuffer(sf::Vector2u size);

	/**
	 * Updates a rectangular section of the specified framebuffer with new pixel data.
	 */
	bool updateFramebuffer(gui3::Ptr<gui3::res::Image> framebuffer, sf::IntRect rect, const sf::Uint8 * pixels);

	/**
	 * Specifies how texture pages should be allocated
	 */
	void setTextureAllocator(TextureAllocator textureAllocator);

	/**
	 * Frees all currently unused resources.
	 */
	void collectGarbage();

	/**
	 * Frees all currently unused data resources.
	 */
	void collectDataGarbage();

	/**
	 * Asynchronously loads a resource in the background.
	 */
	void preloadData(std::string resourceName, ThreadPool & threadPool);

	/**
	 * Returns how many asynchronous loads are currently being performed.
	 */
	int getPendingAsyncLoads() const;

	/**
	 * Polls all resource file change events. Should be called once per tick.
	 */
	void pollChangeEvents();

	/**
	 * Safely deallocates all resources immediately before the last window is closed.
	 */
	void cleanUpBeforeExit();

	/**
	 * Returns the total memory usage of data resources inside the resource manager.
	 */
	std::size_t getDataMemoryUsage() const;

	/**
	 * Returns the total memory usage of images inside the resource manager.
	 */
	std::size_t getImageMemoryUsage() const;

	void setTexturePackerPageSize(unsigned int size);

private:
	class Data : public gui3::res::Data
	{
	public:
		Data(std::string name, std::vector<char> data);
		virtual ~Data();

		virtual const char * getData() const override;
		virtual std::size_t getDataSize() const override;

	private:
		std::vector<char> data;
	};

	class AsyncData : public gui3::res::Data
	{
	public:
		using ProviderFunc = std::function<std::unique_ptr<std::vector<char>>()>;

		AsyncData(std::string name, ThreadPool & threadPool, ProviderFunc provider);
		virtual ~AsyncData();

		virtual const char * getData() const override;
		virtual std::size_t getDataSize() const override;

		std::size_t getMemoryUsage() const override;

		bool await() const;

	private:
		std::unique_ptr<std::vector<char>> data;
		std::atomic_bool done = ATOMIC_VAR_INIT(false);
		mutable std::mutex mutex;
		mutable std::condition_variable conVar;
	};

	class Image : public gui3::res::Image
	{
	public:
		Image(std::string name, TexturePacker::Handle handle, std::size_t page);
		virtual ~Image();

		virtual sf::FloatRect getTextureRect() const override;
		virtual std::size_t getTexturePage() const override;
		TexturePacker::NodeID getTexturePackerNodeID() const;

	private:
		TexturePacker::Handle handle;
		std::size_t texturePage;
	};

	class BitmapFont : public gui3::res::Font
	{
	public:
		BitmapFont(std::string name, std::shared_ptr<wos::text::BitmapFont> font, gui3::Ptr<gui3::res::Image> image);
		virtual ~BitmapFont();

		std::shared_ptr<wos::text::AbstractFont> getFont(unsigned int characterSize) const override;
		bool isOnMainTexture() const override;
		std::size_t getMemoryUsage() const override;

	private:
		std::shared_ptr<wos::text::BitmapFont> font;
		gui3::Ptr<gui3::res::Image> image;
	};

	class VectorFont : public gui3::res::Font, public std::enable_shared_from_this<VectorFont>
	{
	public:
		VectorFont(WOSResourceManager & parent, std::string name, std::shared_ptr<sf::Font> font,
		    std::shared_ptr<gui3::res::Data> data);
		virtual ~VectorFont();

		std::shared_ptr<wos::text::AbstractFont> getFont(unsigned int characterSize) const override;
		bool isOnMainTexture() const override;
		std::size_t getMemoryUsage() const override;

		const sf::Texture * getTexture(unsigned int characterSize) const;

	private:
		WOSResourceManager & parent;
		std::shared_ptr<gui3::res::Data> data;
		std::shared_ptr<sf::Font> font;
		mutable HashMap<unsigned int, std::shared_ptr<wos::text::VectorFont>> sizes;
	};

	class MultiFont : public gui3::res::Font
	{
	public:
		MultiFont(std::string name, std::vector<gui3::Ptr<gui3::res::Font>> fonts);
		virtual ~MultiFont();

		std::shared_ptr<wos::text::AbstractFont> getFont(unsigned int characterSize) const override;
		bool isOnMainTexture() const override;
		std::size_t getMemoryUsage() const override;

	private:
		std::vector<gui3::Ptr<gui3::res::Font>> fonts;
		mutable HashMap<unsigned int, std::shared_ptr<wos::text::AbstractFont>> sizes;
	};

	gui3::Ptr<WOSResourceManager::Image> addImageResource(std::string imageName, const sf::Image & image);

	void handleResourceEvent(res::ResourceEvent event);
	void handleSingleFileEvent(res::ResourceEvent event);
	void handleAllFilesEvent(res::ResourceEvent event);

	gui3::Ptr<res::AbstractSource> source;

	res::ResourceHolder<gui3::res::Data> datas;
	res::ResourceHolder<gui3::res::Image> images;
	res::ResourceHolder<gui3::res::Font> fonts;

	bool textureFilteringEnabled = false;
	std::size_t nextFramebufferID = 0;

	std::map<std::string, CallbackManager<gui3::ResourceEvent>> callbacks;
	Callback<gui3::ResourceEvent> sourceCallbackHandle;

	std::shared_ptr<int> asyncLoadCounter;

	TextureAllocator textureAllocator;

	struct TexturePage
	{
		TexturePage() = default;
		TexturePage(std::unique_ptr<TexturePacker> packer);
		TexturePage(std::shared_ptr<const VectorFont> font, unsigned int characterSize);

		const sf::Texture * getTexture() const;

		std::unique_ptr<TexturePacker> packer;

		std::weak_ptr<const VectorFont> vectorFont;
		unsigned int characterSize = 0;
	};

	std::size_t addTexturePage(TexturePage page);

	std::vector<TexturePage> texturePages;
	unsigned int texturePackerPageSize = 4096;

	Logger logger;
};

#endif /* SRC_CLIENT_SYSTEM_WOSRESOURCEMANAGER_HPP_ */
