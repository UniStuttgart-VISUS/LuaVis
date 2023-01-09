#ifndef SRC_SHARED_GAME_ASYNCPACKAGECOMPILER_HPP_
#define SRC_SHARED_GAME_ASYNCPACKAGECOMPILER_HPP_

#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/StringStream.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace wos
{

class AsyncPackageCompiler
{
public:
	enum Status
	{
		Invalid,
		Running,
		Completed,
		Failed,
	};

	AsyncPackageCompiler();
	virtual ~AsyncPackageCompiler();

	bool compile(const std::string & sourceDirectory, std::vector<std::string> fileList,
	             const std::string & packageFile);

	Status getStatus(const std::string & packageFile) const;
	float getProgress(const std::string & packageFile) const;
	bool pollLogOutput(const std::string & packageFile, Logger::Level & level, std::string & message);

	void release(const std::string & packageFile);

	void clear();

private:
	struct Package
	{
		std::atomic_bool completed {false};
		std::atomic_bool failed {false};

		std::atomic_int progress {0};
		std::atomic_int progressMax {0};

		std::queue<std::pair<Logger::Level, std::string>> pendingOutput;
		std::string packageFile;

		std::thread thread;
		std::mutex mutex;
	};

	Package * lookUpPackage(const std::string & packageFile) const;

	void join(Package & package) const;

	std::map<std::string, std::unique_ptr<Package>> packages;
	Logger logger;
};

}

#endif
