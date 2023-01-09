#include <Shared/Utils/Debug/LogSaturator.hpp>

LogSaturator::LogSaturator(Logger & logger, const char * name, Logger::Level level, int maxLines) :
	logger(logger),
	name(name),
	level(level),
	maxLines(maxLines),
	lineCounter(maxLines)
{
}

LogSaturator::~LogSaturator()
{
	flush();
}

void LogSaturator::flush()
{
	if (lineCounter < 0)
	{
		logger.log(level, "... {} more {}", -lineCounter, name);
		lineCounter = maxLines;
	}
}
