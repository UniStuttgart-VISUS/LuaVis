#ifndef SRC_SHARED_UTILS_DEBUG_LOGSATURATOR_HPP_
#define SRC_SHARED_UTILS_DEBUG_LOGSATURATOR_HPP_

#include <Shared/Utils/Debug/Logger.hpp>

class LogSaturator
{
public:
	LogSaturator(Logger & logger, const char * name, Logger::Level level, int maxLines = 10);
	~LogSaturator();

	template <typename... Args>
	void log(const char * fmt, const Args &... args)
	{
		if ((lineCounter--) > 0)
		{
			logger.log(level, fmt, args...);
		}
	}

	void flush();

private:
	Logger logger;
	const char * name;
	Logger::Level level;
	int maxLines;
	int lineCounter;
};

#endif
