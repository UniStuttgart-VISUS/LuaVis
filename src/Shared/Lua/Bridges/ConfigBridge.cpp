#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Game/AbstractGame.hpp>
#include <Shared/Lua/Bridges/ConfigBridge.hpp>
#include <functional>
#include <string>

namespace lua
{

ConfigBridge::ConfigBridge(wos::AbstractGame & game) :
	game(game)
{
}

ConfigBridge::~ConfigBridge()
{
}

cfg::Config & ConfigBridge::getConfig() const
{
	return game.getConfig();
}

void ConfigBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("config.getString", std::function<std::string(std::string)>([=](std::string key) {
		            return getConfig().get(cfg::transient::String(key));
	            }));

	loader.bind("config.getFloat", std::function<double(std::string)>([=](std::string key) {
		            return getConfig().get(cfg::transient::Float(key));
	            }));

	loader.bind("config.getInt", std::function<int(std::string)>([=](std::string key) {
		            return static_cast<int>(getConfig().get(cfg::transient::Int(key)));
	            }));

	loader.bind("config.getBool", std::function<bool(std::string)>([=](std::string key) {
		            return getConfig().get(cfg::transient::Bool(key));
	            }));

	loader.bind("config.setString",
	            std::function<void(std::string, std::string)>([=](std::string key, std::string value) {
		            getConfig().set(cfg::transient::String(key), value);
	            }));

	loader.bind("config.setFloat", std::function<void(std::string, double)>([=](std::string key, double value) {
		            getConfig().set(cfg::transient::Float(key), value);
	            }));

	loader.bind("config.setInt", std::function<void(std::string, int)>([=](std::string key, int value) {
		            getConfig().set(cfg::transient::Int(key), value);
	            }));

	loader.bind("config.setBool", std::function<void(std::string, bool)>([=](std::string key, bool value) {
		            getConfig().set(cfg::transient::Bool(key), value);
	            }));

	loader.bind("config.unset", std::function<void(std::string)>([=](std::string key) {
		            getConfig().unset(key);
	            }));
}

}
