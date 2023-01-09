#include <Shared/Game/PerformanceCounter.hpp>
#include <Shared/Lua/Bridges/PerformanceBridge.hpp>
#include <functional>

namespace lua
{

PerformanceBridge::PerformanceBridge(wos::PerformanceCounter & performance) :
	performance(performance)
{
}

PerformanceBridge::~PerformanceBridge()
{
}

void PerformanceBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("perf.getFramerate", std::function<double()>([=]() {
		            return performance.getFPS();
	            }));

	loader.bind("perf.getTickTime", std::function<double()>([=]() {
		            return performance.getTickTime().asMicroseconds() / 1000000.0;
	            }));

	loader.bind("perf.getRenderTime", std::function<double()>([=]() {
		            return performance.getRenderTime().asMicroseconds() / 1000000.0;
	            }));

	loader.bind("perf.getTargetTime", std::function<double()>([=]() {
		            return performance.getTargetTime().asMicroseconds() / 1000000.0;
	            }));

	loader.bind("perf.getSleepTime", std::function<double()>([=]() {
		            return performance.getSleepTime().asMicroseconds() / 1000000.0;
	            }));

	loader.bind("perf.getTotalFrameTime", std::function<double()>([=]() {
		            return performance.getTotalFrameTime().asMicroseconds() / 1000000.0;
	            }));

	loader.bind("perf.getMemoryUsage",
	            std::function<sol::optional<double>(std::string)>([=](std::string source) -> sol::optional<double> {
		            // TODO allow getting more detailed memory usage statistics
		            auto result = performance.getMemoryUsage(source, 0);
		            return result.empty() ? sol::nullopt : sol::optional<double>(result[0].memoryUsage);
	            }));
}

}
