#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>
#include <Shared/Utils/Filesystem/FileObserver.hpp>
#include <Shared/Utils/Filesystem/LocalStorage.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/ThreadPool.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Version.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>

#ifdef WOS_WINDOWS
#	include <cppfs/windows/FileNameConversions.h>
#	include <shlobj.h>
#	include <windows.h>
#elif defined(WOS_LINUX) || defined(WOS_OSX)
#	include <pwd.h>
#	include <sys/types.h>
#	include <unistd.h>
#endif

using namespace fs;

LocalStorage & LocalStorage::getInstance(Path path)
{
	// External asset path must be lazy-initialized separately, as it is not immediately available at startup
	if (path == Path::ExternalAssets)
	{
		static std::unique_ptr<LocalStorage> external;
		if (!external)
		{
			// Private c'tor - can't use make_unique here
			external = std::unique_ptr<LocalStorage>(new LocalStorage(Path::ExternalAssets));
		}
		return *external;
	}

	static std::vector<LocalStorage> instances = {Path::WorkingDirectory, Path::Home, Path::Configuration,
	    Path::UserData, Path::Cache, Path::WorkingDirectoryParent};

	std::size_t index = static_cast<std::size_t>(path);
	return index < instances.size() ? instances[index] : instances[static_cast<std::size_t>(Path::UserData)];
}

LocalStorage::LocalStorage(std::string basePath) :
	basePath(basePath)
{
}

std::string LocalStorage::resolve(std::string path) const
{
	return checkPath(path) ? path : basePath;
}

std::vector<std::string> LocalStorage::list(std::string path, ListFlags flags) const
{
	std::vector<std::string> result;

	if (!checkPath(path))
	{
		return result;
	}

	if (flags & ListFiles)
	{
		::listFiles(path, result, flags & ListRecursive, flags & ListSorted);
	}

	if (flags & ListDirectories)
	{
		::listDirectories(path, result, flags & ListRecursive, flags & ListSorted);
	}

	if ((flags & ListFullPath) && !path.empty())
	{
		std::for_each(result.begin(), result.end(), [=](std::string & name) {
			name = path + "/" + name;
		});
	}

	return result;
}

bool LocalStorage::fileExists(std::string path) const
{
	if (checkPath(path))
	{
		return ::fileExists(path);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::isRegularFile(std::string path) const
{
	if (checkPath(path))
	{
		return ::isRegularFile(path);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::isDirectory(std::string path) const
{
	if (checkPath(path))
	{
		return ::isDirectory(path);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::createDirectory(std::string path, bool recursive)
{
	if (checkPath(path))
	{
		return ::createDirectory(path, recursive);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::moveFile(std::string sourcePath, std::string targetPath)
{
	if (checkPath(sourcePath) && checkPath(targetPath))
	{
		return ::moveFile(sourcePath, targetPath);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::deleteFile(std::string path)
{
	if (checkPath(path))
	{
		return ::deleteFile(path);
	}
	else
	{
		return false;
	}
}

bool LocalStorage::deleteDirectory(std::string path, bool recursive)
{
	if (checkPath(path))
	{
		return ::deleteDirectory(path, recursive);
	}
	else
	{
		return false;
	}
}

std::unique_ptr<DirectoryObserver> LocalStorage::observeDirectory(std::string path) const
{
	return observeDirectory(path, DirectoryObserver::Event::Default);
}

std::unique_ptr<DirectoryObserver> LocalStorage::observeDirectory(std::string path, int eventMask, bool recursive) const
{
	if (checkPath(path) && ::isDirectory(path))
	{
		std::unique_ptr<DirectoryObserver> observer = makeUnique<DirectoryObserver>();
		observer->setWatchedDirectory(path);
		observer->setEventMask(eventMask);
		observer->setRecursive(recursive);
		return observer;
	}
	else
	{
		return nullptr;
	}
}

std::unique_ptr<FileObserver> LocalStorage::observeFile(std::string path) const
{
	if (checkPath(path) && ::isRegularFile(path))
	{
		return makeUnique<FileObserver>(std::move(path));
	}
	else
	{
		return nullptr;
	}
}

DataStream LocalStorage::openInputStream(std::string path) const
{
	DataStream stream;
	if (checkPath(path))
	{
		stream.openInFile(path);
	}
	return stream;
}

DataStream LocalStorage::openOutputStream(std::string path)
{
	DataStream stream;
	if (checkPath(path))
	{
		stream.openOutFile(path);
	}
	return stream;
}

bool LocalStorage::writeAsync(std::string path, std::string data, ThreadPool & threadPool)
{
	if (checkPath(path))
	{
		threadPool.submit(
		    [path, dataToWrite = std::move(data)]()
		    {
			    DataStream stream;
			    stream.openOutFile(path);
			    stream.addData(dataToWrite.data(), dataToWrite.size());
		    });
		return true;
	}
	else
	{
		return false;
	}
}

LocalStorage::LocalStorage(Path path) :
	basePath(getSpecialPath(path))
{
}

std::string LocalStorage::getSpecialPath(Path path)
{
#ifdef WOS_WINDOWS
	auto getSpecialFolderByID = [](int csidl, const char * infix = "") -> std::string {
		WCHAR folder[MAX_PATH];
		if (SHGetFolderPathW(NULL, csidl, NULL, 0, folder) == S_OK)
		{
			return cppfs::convert::wideToUtf8String(folder) + infix + "/" + getApplicationIdentifier();
		}
		return "";
	};
#elif defined(WOS_LINUX)
	auto getXDGDirectory = [](const char * xdgVar, const char * fallback) {
		const char * directory = getenv(xdgVar);
		return (directory ? directory : getSpecialPath(Path::Home) + fallback) + "/" + getApplicationIdentifier();
	};
#elif defined(WOS_OSX)
	auto getLibraryPath = [](const char * libraryPath) {
		return getSpecialPath(Path::Home) + "/Library/" + libraryPath + "/" + getApplicationIdentifier();
	};
#endif

	switch (path)
	{
	case Path::WorkingDirectory:
	default:
		return "";

	case Path::WorkingDirectoryParent:
		return "..";

	case Path::Home:
	{
#ifdef WOS_WINDOWS
		return getSpecialFolderByID(CSIDL_PROFILE);
#elif defined(WOS_LINUX) or defined(WOS_OSX)
		const char * homeDirectory = getenv("HOME");

		if (homeDirectory == nullptr)
		{
			homeDirectory = getpwuid(getuid())->pw_dir;
		}

		return homeDirectory != nullptr ? homeDirectory : "";
#else
		return "";
#endif
	}

	case Path::Configuration:
	{
#ifdef WOS_WINDOWS
		return getSpecialFolderByID(CSIDL_APPDATA);
#elif defined(WOS_LINUX)
		return getXDGDirectory("XDG_CONFIG_HOME", "/.config");
#elif defined(WOS_OSX)
		return getLibraryPath("Preferences");
#else
		return "";
#endif
	}

	case Path::UserData:
	{
#ifdef WOS_WINDOWS
		return getSpecialFolderByID(CSIDL_LOCAL_APPDATA);
#elif defined(WOS_LINUX)
		return getXDGDirectory("XDG_DATA_HOME", "/.local/share");
#elif defined(WOS_OSX)
		return getLibraryPath("Application Support");
#else
		return "";
#endif
	}

	case Path::Cache:
	{
#ifdef WOS_WINDOWS
		return getSpecialFolderByID(CSIDL_LOCAL_APPDATA, "/cache");
#elif defined(WOS_LINUX)
		return getXDGDirectory("XDG_CACHE_HOME", "/.cache");
#elif defined(WOS_OSX)
		return getLibraryPath("Caches");
#else
		return "";
#endif
	}

	case Path::ExternalAssets:
		return "";
	}
}

std::string LocalStorage::getApplicationIdentifier()
{
	return version::applicationName;
}

bool LocalStorage::checkPath(std::string & path) const
{
	// Skip all the complicated stuff if the path is empty
	if (path.empty())
	{
		path = basePath;
		return true;
	}

	// Convert slashes to unix-style
	std::replace(path.begin(), path.end(), '\\', '/');

	// Merge slashes
	path.erase(std::unique(path.begin(), path.end(),
	                       [](char a, char b) {
		                       return a == '/' && b == '/';
	                       }),
	           path.end());

	// Remove leading slash
	if (!path.empty() && path[0] == '/')
	{
		path.erase(path.begin());
	}

	// Remove null bytes (and anything following them)
	auto nullPos = path.find_first_of('\0');
	if (nullPos != std::string::npos)
	{
		path.erase(nullPos);
	}

	// Check for parent directory markers and remove them to prevent directory traversal
	if (path.find("..") != std::string::npos)
	{
		std::vector<std::string> segments;
		for (auto segment : splitString(path, "/", false))
		{
			if (segment != "..")
			{
				segments.push_back(segment);
			}
			else if (!segments.empty() && segments.back().empty())
			{
				segments.pop_back();
			}
		}
		path = joinString(segments, "/");
	}

	// Preprend base path
	if (!basePath.empty())
	{
		path = basePath + "/" + path;
	}

	return true;
}
