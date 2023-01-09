#ifndef SRC_SHARED_UTILS_FILESYSTEM_DIRECTORYOBSERVER_HPP_
#define SRC_SHARED_UTILS_FILESYSTEM_DIRECTORYOBSERVER_HPP_

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <memory>
#include <queue>
#include <string>

namespace cppfs
{
class FileHandle;
}

namespace fs
{

class WatcherImpl;

class DirectoryObserver
{
public:
	DirectoryObserver();
	~DirectoryObserver();

	DirectoryObserver(DirectoryObserver &&) = default;
	DirectoryObserver & operator=(DirectoryObserver &&) = default;

	struct Event
	{
		enum Type
		{
			Unknown = 0,

			Added = 1 << 0,
			Removed = 1 << 1,
			Modified = 1 << 2,
			MovedFrom = 1 << 3,
			MovedTo = 1 << 4,
			Attributes = 1 << 5,
			ModifiedCoalesced = 1 << 6,

			Default = Added | Removed | Modified | MovedFrom | MovedTo | ModifiedCoalesced,
			All = (1 << 7) - 1,
		};

		Type type;
		std::string filename;
		std::shared_ptr<cppfs::FileHandle> handle;

		bool operator==(const Event & event) const;
	};

	struct CoalescenceSettings
	{
		bool enabled = false;
		bool force = false;
		sf::Time delay = sf::Time::Zero;
		sf::Time limit = sf::Time::Zero;
	};

	void process();
	bool pollEvent(Event & event);
	void clearQueue();

	void setEventMask(int eventMask);
	int getEventMask() const;

	void setWatchedDirectory(std::string watchedDirectory);
	std::string getWatchedDirectory() const;

	void setRecursive(bool recursive);
	bool isRecursive() const;

	void setEventCoalescence(CoalescenceSettings coalescence);
	const CoalescenceSettings & getEventCoalescence() const;

	void startWatching();
	void stopWatching();
	bool isWatching() const;

private:
	struct EventCoalescenceData
	{
		sf::Clock firstEvent;
		sf::Clock lastEvent;
		std::shared_ptr<cppfs::FileHandle> handle;
	};

	struct EventHash
	{
		std::size_t operator()(const Event & event) const;
	};

	void updateSettings();
	bool isCoalescedEventExpired(const EventCoalescenceData & data) const;
	void pushEvent(Event event, bool coalesce);

	bool watching = false;
	bool recursive = false;
	std::unique_ptr<WatcherImpl> impl;

	std::queue<Event> events;
	CoalescenceSettings coalescence;
	HashMap<std::string, EventCoalescenceData> coalescedEvents;

	std::string watchedDirectory;
	int eventMask = Event::Default;

	Logger logger;

	friend class WatcherImpl;
};

}

#endif
