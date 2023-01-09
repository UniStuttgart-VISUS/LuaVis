#include <Shared/Utils/ThreadPool.hpp>

#include <Shared/Utils/Debug/CrashHandler.hpp>
#include <Shared/Utils/MiscMath.hpp>

ThreadPool::ThreadPool() :
	ThreadPool(getDefaultThreadCount())
{
}

ThreadPool::ThreadPool(std::size_t threadCount) :
	logger("ThreadPool")
{
	for (std::size_t i = 0; i < threadCount; ++i)
	{
		threads.emplace_back(&ThreadPool::processWorkItems, this);
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::lock_guard<std::mutex> guard(queueMutex);
		running = false;
	}

	queueCondition.notify_all();

	for (auto & thread : threads)
	{
		thread.join();
	}
}

void ThreadPool::submit(WorkItem item)
{
	{
		std::lock_guard<std::mutex> guard(queueMutex);
		workQueue.push(std::move(item));
	}

	queueCondition.notify_one();
}

std::size_t ThreadPool::getThreadCount() const
{
	return threads.size();
}

std::size_t ThreadPool::getDefaultThreadCount()
{
	return clamp<std::size_t>(4, std::thread::hardware_concurrency(), 16);
}

void ThreadPool::processWorkItems()
{
	CrashHandler::initThread();

	std::unique_lock<std::mutex> lock(queueMutex);
	while (running)
	{
		queueCondition.wait_for(lock, std::chrono::seconds(10));
		while (!workQueue.empty())
		{
			auto item = std::move(workQueue.front());
			workQueue.pop();
			lock.unlock();

			try
			{
				item();
			}
			catch (std::exception & e)
			{
				logger.error("Error while processing thread pool queue item: {}", e.what());
			}

			lock.lock();
		}
	}
}
