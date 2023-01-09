#ifndef SRC_SHARED_CONTENT_PACKAGECOMPILER_HPP_
#define SRC_SHARED_CONTENT_PACKAGECOMPILER_HPP_

#include <Shared/Utils/Debug/Logger.hpp>
#include <functional>
#include <string>
#include <vector>

namespace res
{

class PackageCompiler
{
public:
	using ProgressCallback = std::function<void(int, int)>;
	using LogCallback = std::function<void(Logger::Level, std::string)>;

	PackageCompiler();

	void setPackageFile(std::string packageFile);
	std::string getPackageFile() const;

	void setSourcePath(std::string sourcePath);
	std::string getSourcePath() const;

	void setSourceFiles(std::vector<std::string> sourceFiles);
	const std::vector<std::string> & getSourceFiles() const;

	void setPermissive(bool permissive);
	bool isPermissive() const;

	void setProgressCallback(ProgressCallback progressCallback);
	const ProgressCallback & getProgressCallback() const;

	void setLogCallback(LogCallback logCallback);
	const LogCallback & getLogCallback() const;

	bool compile();

private:
	void log(Logger::Level level, std::string text);

	std::vector<std::string> sourceFiles;
	std::string sourcePath;
	std::string packageFile;

	ProgressCallback progressCallback;
	LogCallback logCallback;
	bool permissive = false;

	Logger logger;
};

}

#endif
