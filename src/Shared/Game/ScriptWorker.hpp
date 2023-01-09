#ifndef SRC_SHARED_GAME_SCRIPTWORKER_HPP_
#define SRC_SHARED_GAME_SCRIPTWORKER_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace wos
{

class ScriptWorker
{
public:
	ScriptWorker();
	~ScriptWorker();

	void start(std::string scriptSource);
	void requestStop();

	bool isRunning() const;
	bool isStopRequested() const;

	void join();

	void clearIO();
	void pushInput(std::string data);
	bool hasOutput() const;
	std::string peekOutput();
	std::string popOutput();

private:
	std::queue<std::string> input;
	std::queue<std::string> output;

	std::thread workerThread;
	std::atomic_bool interrupted;
	std::atomic_bool running;
	mutable std::mutex mutex;
	mutable std::condition_variable condition;
};

}

#endif
