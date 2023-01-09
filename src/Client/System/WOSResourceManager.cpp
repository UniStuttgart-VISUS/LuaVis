#include <Client/Graphics/UtilitiesSf.hpp>
#include <Client/System/WOSResourceManager.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/InputStream.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/ThreadPool.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>

const std::string WOSResourceManager::IMAGE_NAME_WHITE_PIXEL = "$white_pixel";
const std::string WOSResourceManager::IMAGE_PREFIX_FRAMEBUFFER = "$framebuffer:";

WOSResourceManager::WOSResourceManager() :
	logger("WOSResourceManager")
{
	// Does not hold any data - only used for atomic refcounting
	asyncLoadCounter = std::make_shared<int>(0);
}

WOSResourceManager::~WOSResourceManager()
{
}

void WOSResourceManager::setSource(gui3::Ptr<res::AbstractSource> source)
{
	sourceCallbackHandle.remove();

	this->source = source;

	if (source)
	{
		sourceCallbackHandle = source->addCallback(
		    [=](res::ResourceEvent event) {
			    handleResourceEvent(event);
		    },
		    res::ResourceEvent::Any);

		handleResourceEvent(res::ResourceEvent::MultipleResourcesChanged);
	}
}

gui3::Ptr<res::AbstractSource> WOSResourceManager::getSource() const
{
	return source;
}

gui3::Ptr<gui3::res::Data> WOSResourceManager::acquireData(std::string dataName)
{
	dataName = res::normalizeResourceName(dataName);

	auto cached = datas.get(dataName);
	if (cached)
	{
		return cached;
	}

	std::vector<char> data;

	if (source && source->loadResource(dataName, data))
	{
		gui3::Ptr<gui3::res::Data> resource = gui3::make<Data>(dataName, std::move(data));
		datas.add(dataName, resource);
		return resource;
	}

	return nullptr;
}

gui3::Ptr<gui3::res::Image> WOSResourceManager::acquireImage(std::string imageName)
{
	imageName = res::normalizeResourceName(imageName);

	auto cached = images.get(imageName);
	if (cached)
	{
		return cached;
	}

	sf::Image image;

	if (imageName == WOSResourceManager::IMAGE_NAME_WHITE_PIXEL)
	{
		// Special white pixel resource
		image.create(1, 1, sf::Color::White);
	}
	else if (imageName.empty() || imageName[0] == '$')
	{
		// Unrecognized special resource name
		return nullptr;
	}
	else
	{
		// Regular resource file
		auto imageData = acquireData(imageName);
		if (!imageData || !loadImage((const sf::Uint8 *) imageData->getData(), imageData->getDataSize(), image))
		{
			return nullptr;
		}
	}

	return addImageResource(imageName, image);
}

gui3::Ptr<gui3::res::Font> WOSResourceManager::acquireFont(std::string fontName)
{
	fontName = res::normalizeResourceName(fontName);

	auto cached = fonts.get(fontName);
	if (cached)
	{
		return cached;
	}

	if (fontName.find(';') != std::string::npos)
	{
		// Multi-font
		std::vector<gui3::Ptr<gui3::res::Font>> subFonts;
		for (auto subFontName : splitString(fontName, ";", true))
		{
			auto subFont = acquireFont(subFontName);
			if (subFont)
			{
				subFonts.push_back(subFont);
			}
		}

		gui3::Ptr<gui3::res::Font> resource = gui3::make<MultiFont>(fontName, subFonts);
		fonts.add(fontName, resource);

		return resource;
	}
	else if (stringEndsWith(toLowercase(fontName), ".png"))
	{
		// Bitmap font
		logger.debug("Loading bitmap font '{}'...", fontName);

		sf::Image image;
		auto data = acquireData(fontName);
		if (data == nullptr || !image.loadFromMemory(data->getData(), data->getDataSize()))
		{
			logger.error("Error loading bitmap font '{}': missing or invalid data", fontName);
			return nullptr;
		}

		auto font = std::make_shared<wos::text::BitmapFont>(image);

		std::string fontImageName = "$font_image:" + fontName;
		gui3::Ptr<gui3::res::Image> imageResource = addImageResource(fontImageName, image);
		if (imageResource == nullptr)
		{
			logger.error("Error loading bitmap font '{}': image does not fit into texture atlas", fontName);
			return nullptr;
		}

		gui3::Ptr<gui3::res::Font> resource = gui3::make<BitmapFont>(fontName, font, imageResource);
		fonts.add(fontName, resource);

		logger.debug("Loaded bitmap font '{}'.", fontName);

		return resource;
	}
	else
	{
		// Vector font
		logger.debug("Loading vector font '{}'...", fontName);

		auto sfmlFont = std::make_unique<sf::Font>();
		auto data = acquireData(fontName);
		if (data == nullptr || !sfmlFont->loadFromMemory(data->getData(), data->getDataSize()))
		{
			logger.error("Error loading vector font '{}': missing or invalid data", fontName);
			return nullptr;
		}

		if (stringEndsWith(fontName, "Silver.ttf"))
		{
			sfmlFont->setSmooth(false);
		}

		gui3::Ptr<gui3::res::Font> resource = std::make_shared<VectorFont>(*this, fontName, std::move(sfmlFont), data);
		fonts.add(fontName, resource);

		logger.debug("Loaded vector font '{}'.", fontName);

		return resource;
	}
}

std::vector<std::string> WOSResourceManager::getResourceList(std::string prefix) const
{
	if (source)
	{
		return source->getResourceList(prefix, fs::ListFiles | fs::ListRecursive | fs::ListSorted);
	}
	else
	{
		return {};
	}
}

WOSResourceManager::Data::Data(std::string name, std::vector<char> data) :
	gui3::res::Data(std::move(name)),
	data(data)
{
}

WOSResourceManager::Data::~Data()
{
}

const char * WOSResourceManager::Data::getData() const
{
	return data.data();
}

std::size_t WOSResourceManager::Data::getDataSize() const
{
	return data.size();
}

WOSResourceManager::AsyncData::AsyncData(std::string name, ThreadPool & threadPool, ProviderFunc provider) :
    gui3::res::Data(std::move(name))
{
	threadPool.submit(
	    [this, provider = std::move(provider)]()
	    {
		    auto data = provider();
		    this->data = std::move(data);
		    this->done = true;
		    conVar.notify_all();
	    });
}

WOSResourceManager::AsyncData::~AsyncData()
{
	await();
}

bool WOSResourceManager::AsyncData::await() const
{
	while (!done)
	{
		std::unique_lock<std::mutex> lock(mutex);
		conVar.wait_for(lock, std::chrono::milliseconds(10));
	}
	return data != nullptr;
}

const char * WOSResourceManager::AsyncData::getData() const
{
	return await() ? data->data() : nullptr;
}

std::size_t WOSResourceManager::AsyncData::getDataSize() const
{
	return await() ? data->size() : 0;
}

std::size_t WOSResourceManager::AsyncData::getMemoryUsage() const
{
	return (done && data != nullptr) ? data->size() : 0;
}

WOSResourceManager::Image::Image(std::string name, TexturePacker::Handle handle, std::size_t page) :
	gui3::res::Image(std::move(name)),
	handle(handle),
	texturePage(page)
{
}

WOSResourceManager::Image::~Image()
{
}

sf::FloatRect WOSResourceManager::Image::getTextureRect() const
{
	return sf::FloatRect(handle.getImageRect());
}

std::size_t WOSResourceManager::Image::getTexturePage() const
{
	return texturePage;
}

TexturePacker::NodeID WOSResourceManager::Image::getTexturePackerNodeID() const
{
	return handle.getNodeID();
}

WOSResourceManager::BitmapFont::BitmapFont(std::string name, std::shared_ptr<wos::text::BitmapFont> font,
                                           gui3::Ptr<gui3::res::Image> image) :
	gui3::res::Font(name),
	font(font),
	image(image)
{
	font->setTextureID(image->getTexturePage());
	font->setTextureRect(sf::IntRect(image->getTextureRect()));
}

WOSResourceManager::BitmapFont::~BitmapFont()
{
}

std::shared_ptr<wos::text::AbstractFont> WOSResourceManager::BitmapFont::getFont(unsigned int characterSize) const
{
	return font;
}

bool WOSResourceManager::BitmapFont::isOnMainTexture() const
{
	return image->isOnMainTexture();
}

std::size_t WOSResourceManager::BitmapFont::getMemoryUsage() const
{
	return image->getMemoryUsage();
}

WOSResourceManager::VectorFont::VectorFont(WOSResourceManager & parent, std::string name,
    std::shared_ptr<sf::Font> font, std::shared_ptr<gui3::res::Data> data) :
    gui3::res::Font(name),
    data(data),
    parent(parent),
    font(std::move(font))
{
}

WOSResourceManager::VectorFont::~VectorFont()
{
}

std::shared_ptr<wos::text::AbstractFont> WOSResourceManager::VectorFont::getFont(unsigned int characterSize) const
{
	auto it = sizes.find(characterSize);
	if (it != sizes.end())
	{
		return it->second;
	}
	else
	{
		// TODO support bold fonts
		auto variant = std::make_shared<wos::text::VectorFont>(font, characterSize, false);
		variant->setTextureID(parent.addTexturePage(TexturePage(shared_from_this(), characterSize)));
		return sizes[characterSize] = variant;
	}
}

bool WOSResourceManager::VectorFont::isOnMainTexture() const
{
	return false;
}

std::size_t WOSResourceManager::VectorFont::getMemoryUsage() const
{
	std::size_t pixelCount = 0;
	for (auto it = sizes.begin(); it != sizes.end(); ++it)
	{
		if (const sf::Texture * texture = getTexture(it->second->getTextureID()))
		{
			pixelCount += texture->getSize().x * texture->getSize().y;
		}
	}
	return pixelCount * 4;
}

const sf::Texture * WOSResourceManager::VectorFont::getTexture(unsigned int characterSize) const
{
	return &font->getTexture(characterSize);
}

WOSResourceManager::MultiFont::MultiFont(std::string name, std::vector<gui3::Ptr<gui3::res::Font>> fonts) :
	gui3::res::Font(name),
	fonts(fonts)
{
}

WOSResourceManager::MultiFont::~MultiFont()
{
}

std::shared_ptr<wos::text::AbstractFont> WOSResourceManager::MultiFont::getFont(unsigned int characterSize) const
{
	auto it = sizes.find(characterSize);
	if (it != sizes.end())
	{
		return it->second;
	}
	else
	{
		std::vector<std::shared_ptr<wos::text::AbstractFont>> fontList;
		for (auto & font : fonts)
		{
			fontList.push_back(font->getFont(characterSize));
		}
		auto variant = std::make_shared<wos::text::MultiFont>(fontList);
		return sizes[characterSize] = variant;
	}
}

bool WOSResourceManager::MultiFont::isOnMainTexture() const
{
	return false;
}

std::size_t WOSResourceManager::MultiFont::getMemoryUsage() const
{
	std::size_t usage = 0;
	for (auto & font : fonts)
	{
		usage += font->getMemoryUsage();
	}
	return usage;
}

const sf::Texture * WOSResourceManager::getTexture(std::size_t page) const
{
	if (page < texturePages.size())
	{
		return texturePages[page].getTexture();
	}
	else
	{
		return nullptr;
	}
}

gui3::Ptr<gui3::res::Image> WOSResourceManager::createFramebuffer(sf::Vector2u size)
{
	sf::Image image;
	image.create(size.x, size.y, sf::Color::Transparent);
	return addImageResource(IMAGE_PREFIX_FRAMEBUFFER + std::to_string(nextFramebufferID++), image);
}

bool WOSResourceManager::updateFramebuffer(gui3::Ptr<gui3::res::Image> framebuffer, sf::IntRect rect,
                                           const sf::Uint8 * pixels)
{
	if (framebuffer && framebuffer->getTexturePage() < texturePages.size()
	    && texturePages[framebuffer->getTexturePage()].packer)
	{
		if (auto image = std::dynamic_pointer_cast<Image>(framebuffer))
		{
			auto nodeID = image->getTexturePackerNodeID();
			return texturePages[image->getTexturePage()].packer->update(nodeID, rect, pixels);
		}
	}
	return false;
}

void WOSResourceManager::setTextureAllocator(TextureAllocator textureAllocator)
{
	this->textureAllocator = textureAllocator;
}

void WOSResourceManager::collectGarbage()
{
	for (auto it = callbacks.begin(); it != callbacks.end();)
	{
		if (it->second.isEmpty())
		{
			it = callbacks.erase(it);
		}
		else
		{
			++it;
		}
	}

	fonts.runGC();
	images.runGC();
	datas.runGC();
}

void WOSResourceManager::collectDataGarbage()
{
	datas.runGC();
}

void WOSResourceManager::preloadData(std::string resourceName, ThreadPool & threadPool)
{
	if (datas.get(resourceName))
	{
		return;
	}

	if (source)
	{
		if (std::shared_ptr<res::Stream> stream = source->openStream(resourceName))
		{
			datas.add(resourceName, //
			    std::make_shared<AsyncData>(resourceName, threadPool,
			        [stream, loadCounter = asyncLoadCounter]() -> std::unique_ptr<std::vector<char>>
			        {
				        auto size = stream->getSize();
				        if (size >= 0)
				        {
					        auto result = std::make_unique<std::vector<char>>();
					        result->resize(size);
					        stream->read(result->data(), size);
					        return result;
				        }
				        else
				        {
					        return nullptr;
				        }
			        }));
		}
	}
}

int WOSResourceManager::getPendingAsyncLoads() const
{
	return asyncLoadCounter.use_count() - 1;
}

void WOSResourceManager::pollChangeEvents()
{
	if (source)
	{
		source->pollChanges();
	}
}

void WOSResourceManager::cleanUpBeforeExit()
{
	texturePages.clear();

	fonts.invalidateResources();
	images.invalidateResources();
	datas.invalidateResources();
}

std::size_t WOSResourceManager::getDataMemoryUsage() const
{
	return datas.getTotalMemoryUsage();
}

std::size_t WOSResourceManager::getImageMemoryUsage() const
{
	return images.getTotalMemoryUsage();
}

void WOSResourceManager::setTexturePackerPageSize(unsigned int size)
{
	if (size >= 1024 && size <= 8192)
	{
		texturePackerPageSize = size;
	}
}

CallbackHandle<gui3::ResourceEvent>
WOSResourceManager::addResourceCallback(std::function<void(gui3::ResourceEvent)> callback,
                                        gui3::ResourceEvent::Type typeFilter, std::string resourceFilter, int order)
{
	return callbacks[res::normalizeResourceName(resourceFilter)].addCallback(callback, typeFilter, order);
}

gui3::Ptr<WOSResourceManager::Image> WOSResourceManager::addImageResource(std::string imageName,
                                                                          const sf::Image & image)
{
	if (image.getSize().x == 0 || image.getSize().y == 0)
	{
		return nullptr;
	}

	auto textureAllocation = textureAllocator ? textureAllocator(imageName, image) : TextureAllocation();

	gui3::Ptr<WOSResourceManager::Image> resource;

	if (textureAllocation.useSingleTexture)
	{
		// Append single-image texture page
		auto packer = makeUnique<TexturePacker>(image);
		packer->setSmooth(isTextureFilteringEnabled());

		auto handle = TexturePacker::Handle(*packer, TexturePacker::fullImage);
		resource = gui3::make<Image>(imageName, handle, addTexturePage(std::move(packer)));
	}
	else
	{
		TexturePacker::NodeID node = TexturePacker::packFailure;
		std::size_t page = 0;

		// Try adding image to existing pages first.
		for (; page < texturePages.size(); ++page)
		{
			auto & packer = texturePages[page].packer;
			if (packer && !packer->isSingleTexture())
			{
				node = packer->add(image, false);
				if (node != TexturePacker::packFailure)
				{
					break;
				}
			}
		}

		// None of the existing pages accepted the image?
		if (node == TexturePacker::packFailure)
		{
			// Try adding another page to the end.
			auto packer = makeUnique<TexturePacker>(texturePackerPageSize);
			packer->setSmooth(isTextureFilteringEnabled());
			node = packer->add(image, false);

			// New page rejected image as well? Remove page and give up. Image is probably too big.
			if (node == TexturePacker::packFailure)
			{
				return nullptr;
			}
			else
			{
				page = addTexturePage(std::move(packer));
			}
		}

		// Create smart pointer to resource.
		if (page < texturePages.size() && texturePages[page].packer)
		{
			resource = gui3::make<Image>(imageName, TexturePacker::Handle(*texturePages[page].packer, node), page);
		}
	}

	if (resource)
	{
		// Add weak pointer to resource cache.
		images.add(imageName, resource);
	}

	return resource;
}

void WOSResourceManager::handleResourceEvent(res::ResourceEvent event)
{
	switch (event.type)
	{
	case res::ResourceEvent::MultipleResourcesChanged:
		handleAllFilesEvent(event);
		break;

	case res::ResourceEvent::ResourceAdded:
	case res::ResourceEvent::ResourceChanged:
	case res::ResourceEvent::ResourceRemoved:
		handleSingleFileEvent(event);
		break;

	default:
		break;
	}
}

void WOSResourceManager::handleSingleFileEvent(res::ResourceEvent event)
{
	fonts.invalidateResource(event.resourceName);
	images.invalidateResource(event.resourceName);
	datas.invalidateResource(event.resourceName);

	auto foundCallback = callbacks.find(event.resourceName);
	if (foundCallback != callbacks.end())
	{
		foundCallback->second.fireCallback(event.type, event);
	}

	auto foundWildcardCallback = callbacks.find(ANY_RESOURCE);
	if (foundWildcardCallback != callbacks.end())
	{
		foundWildcardCallback->second.fireCallback(event.type, event);
	}
}

void WOSResourceManager::setTextureFilteringEnabled(bool enabled)
{
	if (textureFilteringEnabled != enabled)
	{
		textureFilteringEnabled = enabled;
		for (const auto & page : texturePages)
		{
			if (page.packer)
			{
				page.packer->setSmooth(enabled);
			}
		}
	}
}

bool WOSResourceManager::isTextureFilteringEnabled() const
{
	return textureFilteringEnabled;
}

void WOSResourceManager::handleAllFilesEvent(res::ResourceEvent event)
{
	fonts.invalidateResources(event.resourceName);
	images.invalidateResources(event.resourceName);
	datas.invalidateResources(event.resourceName);

	for (auto & callback : callbacks)
	{
		callback.second.fireCallback(event.type, event);
	}
}

WOSResourceManager::TexturePage::TexturePage(std::unique_ptr<TexturePacker> packer) :
	packer(std::move(packer))
{
}

WOSResourceManager::TexturePage::TexturePage(std::shared_ptr<const VectorFont> font, unsigned int characterSize) :
	vectorFont(font),
	characterSize(characterSize)
{
}

const sf::Texture * WOSResourceManager::TexturePage::getTexture() const
{
	if (packer)
	{
		return packer->getTexture();
	}
	else if (auto font = vectorFont.lock())
	{
		return font->getTexture(characterSize);
	}
	else
	{
		return nullptr;
	}
}

std::size_t WOSResourceManager::addTexturePage(TexturePage page)
{
	texturePages.push_back(std::move(page));
	return texturePages.size() - 1;
}
