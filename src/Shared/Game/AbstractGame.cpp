#include <Shared/Game/AbstractGame.hpp>

namespace wos
{

AbstractGame::AbstractGame()
{
}

AbstractGame::~AbstractGame()
{
}

const std::vector<std::string> & AbstractGame::getCommandLineArguments() const
{
	static std::vector<std::string> args;
	return args;
}

void AbstractGame::resetLater()
{
}

ThreadPool & AbstractGame::getThreadPool()
{
	return threadPool;
}

void AbstractGame::deallocateUnusedResources()
{
}
}
