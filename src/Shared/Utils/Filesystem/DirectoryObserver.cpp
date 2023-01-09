#include <Shared/Utils/Filesystem/DirectoryObserver.hpp>

#include <SFML/System/Lock.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <cppfs/FileHandle.h>
#include <cppfs/FileWatcher.h>
#include <cppfs/fs.h>
#include <exception>

namespace fs
{

class WatcherImpl
{
public:
	WatcherImpl(DirectoryObserver & observer, std::string directory, int eventMask, bool recursive) :
		observer(observer),
		watchedDirectory(cppfs::fs::open(directory))
	{
		watcher.add(watchedDirectory, eventMaskToCppFS(eventMask), recursive ? cppfs::Recursive : cppfs::NonRecursive);
		watcher.addHandler([this](cppfs::FileHandle & file, cppfs::FileEvent event) {
			handleEvent(file, event);
		});
	}

	~WatcherImpl()
	{
	}

	void handleEvent(cppfs::FileHandle & file, cppfs::FileEvent eventType)
	{
		DirectoryObserver::Event event;
		event.filename = file.path();
		event.handle = std::make_shared<cppfs::FileHandle>(std::move(file));
		event.type = cppFSEventToType(eventType);

		// FileModified events can be fired intermittently during the write and should be coalesced
		observer.pushEvent(std::move(event), eventType == cppfs::FileModified);
	}

	void watch(sf::Time timeout)
	{
		watcher.watch(timeout.asMilliseconds());
	}

	static int eventMaskToCppFS(int eventMask)
	{
		int fsMask = 0;

		if (eventMask & DirectoryObserver::Event::Added)
			fsMask |= cppfs::FileCreated;
		if (eventMask & DirectoryObserver::Event::Removed)
			fsMask |= cppfs::FileRemoved;
		if (eventMask & DirectoryObserver::Event::Modified)
			fsMask |= cppfs::FileModified | cppfs::FileWritten;
		if (eventMask & DirectoryObserver::Event::MovedFrom)
			fsMask |= cppfs::FileMovedFrom;
		if (eventMask & DirectoryObserver::Event::MovedTo)
			fsMask |= cppfs::FileMovedTo;
		if (eventMask & DirectoryObserver::Event::Attributes)
			fsMask |= cppfs::FileAttrChanged;

		return fsMask;
	}

	static DirectoryObserver::Event::Type cppFSEventToType(int eventMask)
	{
		switch (eventMask)
		{
		case cppfs::FileCreated:
			return DirectoryObserver::Event::Added;
		case cppfs::FileRemoved:
			return DirectoryObserver::Event::Removed;
		case cppfs::FileModified:
			return DirectoryObserver::Event::Modified;
		case cppfs::FileWritten:
			return DirectoryObserver::Event::Modified;
		case cppfs::FileMovedFrom:
			return DirectoryObserver::Event::MovedFrom;
		case cppfs::FileMovedTo:
			return DirectoryObserver::Event::MovedTo;
		case cppfs::FileAttrChanged:
			return DirectoryObserver::Event::Attributes;
		default:
			return DirectoryObserver::Event::Unknown;
		}
	}

private:
	DirectoryObserver & observer;
	cppfs::FileWatcher watcher;
	cppfs::FileHandle watchedDirectory;
};

DirectoryObserver::DirectoryObserver() :
	logger("DirectoryObserver")
{
}

DirectoryObserver::~DirectoryObserver()
{
	stopWatching();
}

bool DirectoryObserver::Event::operator==(const Event & event) const
{
	return type == event.type && filename == event.filename;
}

void DirectoryObserver::process()
{
	if (impl)
	{
		try
		{
			impl->watch(sf::Time::Zero);
		}
		catch (std::exception & ex)
		{
			logger.error("Error occurred while observing directory '{}': {}", watchedDirectory, ex.what());
			stopWatching();
		}
	}

	if (!coalescedEvents.empty())
	{
		for (auto it = coalescedEvents.begin(); it != coalescedEvents.end();)
		{
			// Check if event has either "quieted" down sufficiently long, or is old enough to be queued
			if (isCoalescedEventExpired(it->second))
			{
				Event event;
				event.filename = it->first;
				event.handle = it->second.handle;
				event.type = Event::Type::ModifiedCoalesced;
				events.push(event);
				it = coalescedEvents.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}

bool DirectoryObserver::pollEvent(Event & event)
{
	if (events.empty())
	{
		return false;
	}
	event = std::move(events.front());
	events.pop();
	return true;
}

void DirectoryObserver::clearQueue()
{
	events = std::queue<Event>();
}

void DirectoryObserver::setEventMask(int eventMask)
{
	if (this->eventMask != eventMask)
	{
		this->eventMask = eventMask;
		updateSettings();
	}
}

int DirectoryObserver::getEventMask() const
{
	return eventMask;
}

void DirectoryObserver::setWatchedDirectory(std::string watchedDirectory)
{
	if (this->watchedDirectory != watchedDirectory)
	{
		this->watchedDirectory = std::move(watchedDirectory);
		updateSettings();
	}
}

std::string DirectoryObserver::getWatchedDirectory() const
{
	return watchedDirectory;
}

void DirectoryObserver::setRecursive(bool recursive)
{
	if (this->recursive != recursive)
	{
		this->recursive = recursive;
		updateSettings();
	}
}

bool DirectoryObserver::isRecursive() const
{
	return recursive;
}

void DirectoryObserver::setEventCoalescence(CoalescenceSettings coalescence)
{
	this->coalescence = std::move(coalescence);
}

const DirectoryObserver::CoalescenceSettings & DirectoryObserver::getEventCoalescence() const
{
	return coalescence;
}

void DirectoryObserver::startWatching()
{
	if (!isWatching())
	{
		try
		{
			if (watchedDirectory.empty() || eventMask == 0)
			{
				impl = nullptr;
			}
			else
			{
				impl = makeUnique<WatcherImpl>(*this, watchedDirectory, eventMask, recursive);
			}
			watching = true;
		}
		catch (std::exception & ex)
		{
			logger.error("Error occurred while creating directory observer for '{}': {}", watchedDirectory, ex.what());
			stopWatching();
		}
	}
}

void DirectoryObserver::stopWatching()
{
	impl = nullptr;
	watching = false;
}

bool DirectoryObserver::isWatching() const
{
	return watching;
}

std::size_t DirectoryObserver::EventHash::operator()(const Event & event) const
{
	return std::hash<std::string>()(event.filename) ^ std::hash<int>()(event.type);
}

void DirectoryObserver::updateSettings()
{
	if (isWatching())
	{
		stopWatching();
		startWatching();
	}
}

bool DirectoryObserver::isCoalescedEventExpired(const EventCoalescenceData & data) const
{
	return !coalescence.enabled
	       || (coalescence.limit > sf::Time::Zero && data.firstEvent.getElapsedTime() >= coalescence.limit)
	       || (coalescence.delay > sf::Time::Zero && data.lastEvent.getElapsedTime() >= coalescence.delay);
}

void DirectoryObserver::pushEvent(Event event, bool coalesce)
{
	if (coalescence.enabled && (coalesce || coalescence.force))
	{
		auto & coalescedEvent = coalescedEvents[event.filename];
		coalescedEvent.lastEvent.restart();
		coalescedEvent.handle = event.handle;
	}
	else
	{
		if (!coalescedEvents.empty())
		{
			coalescedEvents.erase(event.filename);
		}
		events.push(std::move(event));
	}
}

}
