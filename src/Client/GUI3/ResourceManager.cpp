/*
 * ResourceManager.cpp
 *
 *  Created on: Jul 9, 2015
 *      Author: marukyu
 */

#include <Client/GUI3/ResourceManager.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <algorithm>

namespace gui3
{

namespace res
{

const std::string ResourceManager::ANY_RESOURCE = "$any";

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

Ptr<::res::AbstractSource> gui3::res::ResourceManager::getSource() const
{
	return nullptr;
}

Ptr<Data> ResourceManager::acquireData(std::string dataName)
{
	return nullptr;
}

Ptr<Image> ResourceManager::acquireImage(std::string imageName)
{
	return nullptr;
}

Ptr<Font> ResourceManager::acquireFont(std::string fontName)
{
	return nullptr;
}

std::vector<std::string> ResourceManager::getResourceList() const
{
	return getResourceList("");
}

std::vector<std::string> ResourceManager::getResourceList(std::string prefix) const
{
	return std::vector<std::string>();
}

CallbackHandle<ResourceEvent> ResourceManager::addResourceCallback(std::function<void(ResourceEvent)> callback,
                                                                   ResourceEvent::Type typeFilter,
                                                                   std::string resourceFilter)
{
	return addResourceCallback(callback, typeFilter, resourceFilter, 0);
}

CallbackHandle<ResourceEvent> ResourceManager::addResourceCallback(std::function<void(ResourceEvent)> callback,
                                                                   ResourceEvent::Type typeFilter,
                                                                   std::string resourceFilter, int order)
{
	return CallbackHandle<ResourceEvent>();
}

Data::Data(std::string name) :
	Resource(std::move(name))
{
}

std::size_t Data::getMemoryUsage() const
{
	return getDataSize();
}

Image::Image(std::string name) :
	Resource(std::move(name))
{
}

bool Image::isOnMainTexture() const
{
	return getTexturePage() == 0;
}

std::size_t Image::getMemoryUsage() const
{
	static constexpr std::size_t bytesPerPixel = 4;
	sf::IntRect rect(getTextureRect());
	return std::max<std::int64_t>(0, rect.width * rect.height) * bytesPerPixel;
}

Font::Font(std::string name) :
	Resource(std::move(name))
{
}

std::size_t Font::getMemoryUsage() const
{
	return 0;
}

}
}
