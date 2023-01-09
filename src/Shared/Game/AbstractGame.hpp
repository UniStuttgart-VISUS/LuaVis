#ifndef SRC_SHARED_GAME_ABSTRACTGAME_HPP_
#define SRC_SHARED_GAME_ABSTRACTGAME_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/Resource.hpp>
#include <Shared/Utils/ThreadPool.hpp>

namespace cfg
{
class Config;
}

namespace wos
{

namespace leaderboards
{
class AbstractLeaderboardProvider;
}

class ResourceLoader;
class AbstractUserStatistics;

class AbstractGame
{
public:
	AbstractGame();
	virtual ~AbstractGame();

	virtual cfg::Config & getConfig() const = 0;
	virtual res::AbstractSource & getResources() const = 0;

	virtual const std::vector<std::string> & getCommandLineArguments() const;

	virtual CallbackHandle<> addTickCallback(CallbackFunction<> callback, int order = 0) = 0;

	virtual void showError(std::string errorText) = 0;

	virtual void resetLater();
	virtual void exit() = 0;

	ThreadPool & getThreadPool();

	virtual void deallocateUnusedResources();

private:
	ThreadPool threadPool;
};

}

#endif
