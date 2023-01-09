#ifndef SRC_SHARED_UTILS_THREADPOOL_HPP_
#define SRC_SHARED_UTILS_THREADPOOL_HPP_

#include <Shared/Utils/Debug/Logger.hpp>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
	using WorkItem = std::function<void()>;

	ThreadPool();
	ThreadPool(std::size_t threadCount);
	~ThreadPool();

	ThreadPool(const ThreadPool &) = delete;
	ThreadPool & operator=(const ThreadPool &) = delete;

	void submit(WorkItem item);

	std::size_t getThreadCount() const;

	static std::size_t getDefaultThreadCount();

private:
	void processWorkItems();

	bool running = true;
	std::vector<std::thread> threads;

	std::queue<WorkItem> workQueue;
	std::mutex queueMutex;
	std::condition_variable queueCondition;

	Logger logger;
};

#endif
