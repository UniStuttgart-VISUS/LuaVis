#ifndef SRC_SHARED_LUA_BRIDGES_RESOURCEBRIDGE_HPP_
#define SRC_SHARED_LUA_BRIDGES_RESOURCEBRIDGE_HPP_

#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Lua/Bridges/AbstractBridge.hpp>
#include <Shared/Lua/Bridges/BridgeLoader.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <string>

namespace fs
{
class LocalStorage;
}

namespace res
{
class AbstractSource;
class SourceAggregator;
}

namespace wos
{
class AsyncPackageCompiler;
class ResourceLoader;
}

class ThreadPool;

namespace lua
{

class ResourceBridge : public AbstractBridge
{
public:
	ResourceBridge(wos::ResourceLoader & aggregator);
	ResourceBridge(
	    wos::ResourceLoader & aggregator, wos::AsyncPackageCompiler & packageCompiler, ThreadPool & threadPool);
	virtual ~ResourceBridge();

protected:
	virtual void onLoad(BridgeLoader & loader) override;

private:
	void handleResourceEvent(res::ResourceEvent event);

	HashMap<std::string, res::ResourceEvent::Type> changedResources;
	Callback<res::ResourceEvent> resourceCallback;

	wos::ResourceLoader & resourceLoader;
	wos::AsyncPackageCompiler * packageCompiler = nullptr;
	ThreadPool * threadPool = nullptr;

	Logger logger;
};

}

#endif
