/*
 * ResourceManager.hpp
 *
 *  Created on: Jul 9, 2015
 *      Author: marukyu
 */

#ifndef SRC_CLIENT_GUI3_RESOURCEMANAGER_HPP_
#define SRC_CLIENT_GUI3_RESOURCEMANAGER_HPP_

#include <Client/GUI3/Types.hpp>
#include <Client/Graphics/Text/AbstractFont.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace res
{
class Resource;
}

namespace res
{
class ResourceEvent;
}

namespace gui3
{

using ResourceEvent = ::res::ResourceEvent;

namespace res
{

using Resource = ::res::Resource;

class Data : public Resource
{
public:
	Data(std::string name);
	virtual ~Data() = default;

	virtual const char * getData() const = 0;
	virtual std::size_t getDataSize() const = 0;
	std::size_t getMemoryUsage() const override;
};

class Image : public Resource
{
public:
	Image(std::string name);
	virtual ~Image() = default;

	virtual sf::FloatRect getTextureRect() const = 0;
	virtual std::size_t getTexturePage() const = 0;
	std::size_t getMemoryUsage() const override;
	bool isOnMainTexture() const;
};

class Font : public Resource
{
public:
	Font(std::string name);
	virtual ~Font() = default;

	virtual std::shared_ptr<wos::text::AbstractFont> getFont(unsigned int characterSize) const = 0;
	virtual bool isOnMainTexture() const = 0;
	std::size_t getMemoryUsage() const override;
};

class ResourceManager
{
public:
	static const std::string ANY_RESOURCE;

	ResourceManager();

	virtual ~ResourceManager();

	/**
	 * Returns a pointer to the underlying resource source. May return a null pointer if the operation is unsupported.
	 */
	virtual Ptr<::res::AbstractSource> getSource() const;

	/**
	 * Gets a handle to the specified data. Returns a null pointer if the data failed to load.
	 */
	virtual Ptr<Data> acquireData(std::string dataName);

	/**
	 * Gets a handle to the specified image. Returns a null pointer if the image failed to load.
	 */
	virtual Ptr<Image> acquireImage(std::string imageName);

	/**
	 * Gets a handle to the specified font. Returns a null pointer if the font failed to load.
	 */
	virtual Ptr<Font> acquireFont(std::string fontName);

	/**
	 * Returns a list of all resources with the specified prefix.
	 */
	std::vector<std::string> getResourceList() const;
	virtual std::vector<std::string> getResourceList(std::string prefix) const;

	/**
	 * Registers a callback function to be executed when the specified resource is reloaded.
	 */
	CallbackHandle<ResourceEvent> addResourceCallback(std::function<void(ResourceEvent)> callback,
	                                                  ResourceEvent::Type typeFilter, std::string resourceFilter);
	virtual CallbackHandle<ResourceEvent> addResourceCallback(std::function<void(ResourceEvent)> callback,
	                                                          ResourceEvent::Type typeFilter,
	                                                          std::string resourceFilter, int order);
};

}

}

#endif
