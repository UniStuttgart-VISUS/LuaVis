#include <Shared/Content/Package.hpp>
#include <Shared/Content/PackageCompiler.hpp>
#include <Shared/Game/AsyncPackageCompiler.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/Utilities.hpp>

#include <functional>
#include <string>
#include <utility>

namespace wos
{

AsyncPackageCompiler::AsyncPackageCompiler() :
	logger("AsyncPackageCompiler")
{
}

AsyncPackageCompiler::~AsyncPackageCompiler()
{
	clear();
}

bool AsyncPackageCompiler::compile(const std::string & sourceDirectory, std::vector<std::string> fileList,
                                   const std::string & packageFile)
{
	if (lookUpPackage(packageFile))
	{
		logger.error("Cannot compile package '{}': package is already being compiled", packageFile);
		return false;
	}

	if (!isDirectory(sourceDirectory))
	{
		logger.error("Cannot compile package '{}': source path '{}' is not a valid directory", packageFile,
		             sourceDirectory);
		return false;
	}

	logger.info("Compiling resources from directory '{}' into package '{}'", sourceDirectory, packageFile);

	auto package = makeUnique<Package>();
	package->packageFile = packageFile;
	package->thread = std::thread(
	    std::function<void()>([sourceDirectory, packageFile, fileList = std::move(fileList), &package = *package] {
		    Logger logger("AsyncPackageCompiler");
		    auto log = [&logger, &package](Logger::Level level, std::string message) {
			    logger.log(level, "{}", message);

			    std::lock_guard<std::mutex> lock(package.mutex);
			    package.pendingOutput.emplace(level, std::move(message));
		    };

		    try
		    {
			    res::PackageCompiler compiler;
			    compiler.setSourcePath(sourceDirectory);
			    compiler.setSourceFiles(std::move(fileList));
			    compiler.setPackageFile(packageFile);
			    compiler.setPermissive(false);
			    compiler.setLogCallback(log);
			    compiler.setProgressCallback([&package](int progress, int progressMax) {
				    package.progress = progress;
				    package.progressMax = progressMax;
			    });

			    bool result = compiler.compile();
			    if (result)
			    {
				    package.completed = true;
			    }
			    else
			    {
				    package.failed = true;
			    }
		    }
		    catch (std::exception & e)
		    {
			    log(Logger::Level::Error, e.what());
			    package.failed = true;
		    }
	    }));

	packages.emplace(packageFile, std::move(package));
	return true;
}

AsyncPackageCompiler::Status AsyncPackageCompiler::getStatus(const std::string & packageFile) const
{
	if (auto package = lookUpPackage(packageFile))
	{
		return package->completed ? Completed : (package->failed ? Failed : Running);
	}
	else
	{
		return Invalid;
	}
}

float AsyncPackageCompiler::getProgress(const std::string & packageFile) const
{
	if (auto package = lookUpPackage(packageFile))
	{
		int progressMax = package->progressMax;
		return progressMax == 0 ? 0.f : float(package->progress) / progressMax;
	}
	else
	{
		return 0.f;
	}
}

bool AsyncPackageCompiler::pollLogOutput(const std::string & packageFile, Logger::Level & level, std::string & message)
{
	if (auto package = lookUpPackage(packageFile))
	{
		std::lock_guard<std::mutex> lock(package->mutex);

		if (!package->pendingOutput.empty())
		{
			level = package->pendingOutput.front().first;
			message = package->pendingOutput.front().second;
			package->pendingOutput.pop();
			return true;
		}
	}

	return false;
}

void AsyncPackageCompiler::release(const std::string & packageFile)
{
	if (auto package = lookUpPackage(packageFile))
	{
		join(*package);
		packages.erase(packageFile);
	}
}

void AsyncPackageCompiler::clear()
{
	for (auto & package : packages)
	{
		join(*package.second);
	}

	packages.clear();
}

AsyncPackageCompiler::Package * AsyncPackageCompiler::lookUpPackage(const std::string & packageFile) const
{
	auto it = packages.find(packageFile);
	return it == packages.end() ? nullptr : it->second.get();
}

void AsyncPackageCompiler::join(Package & package) const
{
	if (package.thread.joinable())
	{
		if (!package.completed && !package.failed)
		{
			logger.warn("Package '{}' is still being compiled. Blocking until completion...", package.packageFile);
		}
		package.thread.join();
	}
}

}
