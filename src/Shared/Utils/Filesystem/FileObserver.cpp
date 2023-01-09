#include <Shared/Utils/Filesystem/FileObserver.hpp>

#include <cppfs/FilePath.h>

namespace fs
{

FileObserver::FileObserver(std::string filename)
{
	setTargetFile(filename);
}

FileObserver::~FileObserver()
{
}

void FileObserver::setTargetFile(std::string filename)
{
	directoryObserver.stopWatching();

	if (!filename.empty())
	{
		cppfs::FilePath filePath(filename);

		// Assign full path.
		target = filePath.fullPath();

		// Compute effective watch directory (convert relative path into dot-prefixed path)
		std::string directory = filePath.directoryPath();
		if (directory == "")
		{
			directory = ".";
			target = "./" + target;
		}

		// Set event mask and target directory.
		directoryObserver.setEventMask(DirectoryObserver::Event::Default);
		directoryObserver.setWatchedDirectory(directory);

		// Observe directory that contains the file.
		directoryObserver.startWatching();
	}
}

std::string FileObserver::getTargetFile() const
{
	return target;
}

bool FileObserver::poll()
{
	DirectoryObserver::Event event;

	directoryObserver.process();

	while (directoryObserver.pollEvent(event))
	{
		if (event.filename == target)
		{
			directoryObserver.clearQueue();
			return true;
		}
	}
	return false;
}

}
