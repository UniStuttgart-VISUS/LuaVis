#ifndef SRC_CLIENT_SYSTEM_ASSETS_LOCALSTORAGE_HPP_
#define SRC_CLIENT_SYSTEM_ASSETS_LOCALSTORAGE_HPP_

#include <Shared/Utils/Filesystem/Filesystem.hpp>
#include <memory>
#include <string>
#include <vector>

class ThreadPool;
class DataStream;

namespace fs
{
class DirectoryObserver;
class FileObserver;

class LocalStorage
{
public:
	enum class Path
	{
		WorkingDirectory,
		Home,
		Configuration,
		UserData,
		Cache,
		WorkingDirectoryParent,
		ExternalAssets,
	};

	static LocalStorage & getInstance(Path path);

	LocalStorage(std::string basePath);

	std::string resolve(std::string path = "") const;

	std::vector<std::string> list(std::string path, ListFlags flags = ListFiles | ListSorted) const;

	bool fileExists(std::string path) const;
	bool isRegularFile(std::string path) const;
	bool isDirectory(std::string path) const;

	bool createDirectory(std::string path, bool recursive = false);
	bool moveFile(std::string sourcePath, std::string targetPath);
	bool deleteFile(std::string path);
	bool deleteDirectory(std::string path, bool recursive = false);

	std::unique_ptr<DirectoryObserver> observeDirectory(std::string path) const;
	std::unique_ptr<DirectoryObserver> observeDirectory(std::string path, int eventMask, bool recursive = false) const;

	std::unique_ptr<FileObserver> observeFile(std::string path) const;

	DataStream openInputStream(std::string path) const;
	DataStream openOutputStream(std::string path);
	bool writeAsync(std::string path, std::string data, ThreadPool & threadPool);

private:
	LocalStorage(Path path);

	static std::string getSpecialPath(Path path);
	static std::string getApplicationIdentifier();

	bool checkPath(std::string & path) const;

	std::string basePath;
};

}

#endif
