#ifndef SRC_SHARED_GAME_PERFORMANCECOUNTER_HPP_
#define SRC_SHARED_GAME_PERFORMANCECOUNTER_HPP_

#include <SFML/System/Time.hpp>
#include <Shared/Utils/FPS.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace wos
{

class PerformanceCounter
{
public:
	struct MemoryUsageEntry
	{
		std::string name;
		std::size_t memoryUsage = 0;
	};

	using MemoryUsageProvider = std::function<std::vector<MemoryUsageEntry>(std::size_t)>;

	PerformanceCounter();
	virtual ~PerformanceCounter();

	void tick();
	float getFPS() const;

	void setTickTime(sf::Time time);
	sf::Time getTickTime() const;

	void setRenderTime(sf::Time time);
	sf::Time getRenderTime() const;

	void setSleepTime(sf::Time time);
	sf::Time getSleepTime() const;

	void setTotalFrameTime(sf::Time time);
	sf::Time getTotalFrameTime() const;

	void setTargetTime(sf::Time time);
	sf::Time getTargetTime() const;

	void registerMemoryUsageProvider(std::string name, MemoryUsageProvider provider);
	void unregisterMemoryUsageProvider(std::string name);
	std::vector<MemoryUsageEntry> getMemoryUsage(const std::string & name, std::size_t numberOfEntries) const;

private:
	FpsCounter fps;
	sf::Time tickTime;
	sf::Time renderTime;
	sf::Time sleepTime;
	sf::Time totalFrameTime;
	sf::Time targetTime;
	HashMap<std::string, MemoryUsageProvider> memoryUsageProviders;
};

}

#endif
