#ifndef SRC_SHARED_UTILS_DEBUG_CRASHHANDLER_HPP_
#define SRC_SHARED_UTILS_DEBUG_CRASHHANDLER_HPP_

#include <string>

class StackTrace;

class CrashHandler
{
public:
	enum class MetaKey
	{
		Version,
		Notes,
		UserName,
		UserEmail,
		UserDescription,
	};

	static void init(std::string dumpfile);
	static void initThread();

	static bool dumpExists();
	static std::string readErrorFromDump();
	static void removeDumpFile();

	static bool isExitRequested();

	static void setMetadata(MetaKey key, std::string value);

private:
	CrashHandler();
};

#endif
