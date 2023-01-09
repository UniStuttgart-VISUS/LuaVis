#include <Shared/Game/PerformanceCounter.hpp>

namespace wos
{

PerformanceCounter::PerformanceCounter()
{
}

PerformanceCounter::~PerformanceCounter()
{
}

void PerformanceCounter::tick()
{
	fps.tick();
}

float PerformanceCounter::getFPS() const
{
	return fps.getFPS();
}

void PerformanceCounter::setTickTime(sf::Time time)
{
	tickTime = time;
}

sf::Time PerformanceCounter::getTickTime() const
{
	return tickTime;
}

void PerformanceCounter::setRenderTime(sf::Time time)
{
	renderTime = time;
}

sf::Time PerformanceCounter::getRenderTime() const
{
	return renderTime;
}

void PerformanceCounter::setSleepTime(sf::Time time)
{
	sleepTime = time;
}

sf::Time PerformanceCounter::getSleepTime() const
{
	return sleepTime;
}

void PerformanceCounter::setTotalFrameTime(sf::Time time)
{
	totalFrameTime = time;
}

sf::Time PerformanceCounter::getTotalFrameTime() const
{
	return totalFrameTime;
}

void PerformanceCounter::setTargetTime(sf::Time time)
{
	targetTime = time;
}

sf::Time PerformanceCounter::getTargetTime() const
{
	return targetTime;
}

void PerformanceCounter::registerMemoryUsageProvider(std::string name, MemoryUsageProvider provider)
{
	memoryUsageProviders.emplace(name, provider);
}

void PerformanceCounter::unregisterMemoryUsageProvider(std::string name)
{
	memoryUsageProviders.erase(name);
}

std::vector<PerformanceCounter::MemoryUsageEntry> PerformanceCounter::getMemoryUsage(const std::string & name,
                                                                                     std::size_t numberOfEntries) const
{
	auto provider = memoryUsageProviders.find(name);
	if (provider != memoryUsageProviders.end())
	{
		return provider->second(numberOfEntries);
	}
	else
	{
		return {};
	}
}

}
