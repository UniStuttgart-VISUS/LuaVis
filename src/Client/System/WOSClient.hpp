#ifndef SRC_CLIENT_SYSTEM_WOSCLIENT_HPP_
#define SRC_CLIENT_SYSTEM_WOSCLIENT_HPP_

#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/ResourceManager.hpp>
#include <Client/GUI3/Types.hpp>
#include <Client/Game/LocalGame.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/System/WOSApplication.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Filesystem/FileObserver.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace res
{
class PackageSource;
class SourceAggregator;
class DirectorySource;
}

namespace cfg
{
class ConfigAggregator;
class Config;
class JSONConfig;
}

class WOSResourceManager;

namespace gui3
{
class Interface;
namespace res
{
class ResourceManager;
}
}
namespace sf
{
class Texture;
}

namespace wos
{
namespace steam
{
class SteamAPI;
}

namespace gog
{
class GogAPI;
}
}

class WOSClient : public gui3::Application
{
public:
	WOSClient();
	virtual ~WOSClient();

	virtual const sf::Texture * getTexture(std::size_t pageIndex) const override;

	virtual gui3::res::ResourceManager & getResourceManager() const override;

	virtual sf::Vector2f getWhitePixel() const override;

	virtual cfg::Config & getConfig() const override;

private:
	enum LoadReturnCodes
	{
		LoadSuccess = 0,
		LoadErrorGeneric,
		LoadErrorAssetFile,
		LoadErrorFont,
	};

	void saveConfig();

	virtual int init(const std::vector<std::string> & args) override;

	void initApplication();
	void initPlatform();
	void initAssets();
	void initWhitePixel();
	void initWindow();

	void updateConfig();

	virtual gui3::Interface * makeInterface() override;

	virtual void cleanUpBeforeExit() override;

	template <typename CallbackType>
	void bind(CallbackType callback)
	{
		removeActions.push_back([=] {
			callback.remove();
		});
	}

	// World of Sand application instance
	std::unique_ptr<WOSApplication> app;

	// World of Sand game instance
	gui3::Ptr<wos::LocalGame> game;

	// Resources
	std::unique_ptr<WOSResourceManager> resourceManager;

	// Graphics
	gui3::Ptr<gui3::res::Image> whitePixel;
	sf::Vector2f whitePixelPosition;

#ifdef WOS_ENABLE_STEAM
	// Steam
	std::unique_ptr<wos::steam::SteamAPI> steamAPI;
#endif

#ifdef WOS_ENABLE_GALAXY
	// GoG Galaxy
	std::unique_ptr<wos::gog::GogAPI> gogAPI;
#endif

	// Command line arguments
	std::vector<std::string> args;

	// Deallocate callbacks when closing. TODO: do this more generally for all widgets.
	std::vector<std::function<void()>> removeActions;

	Logger logger;
};

#endif
