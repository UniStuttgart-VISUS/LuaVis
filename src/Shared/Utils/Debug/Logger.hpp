#ifndef SRC_SHARED_UTILS_DEBUG_LOGGER_HPP_
#define SRC_SHARED_UTILS_DEBUG_LOGGER_HPP_

#ifdef WOS_DEBUG
#	define WOS_LOG_LEVEL Debug
#else
#	define WOS_LOG_LEVEL Info
#endif

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

class Logger
{
public:
	enum class Level
	{
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Critical,
		Off,
	};

	Logger(std::string name);
	Logger(std::string name, Level minimumLevel);
	virtual ~Logger();

	template <typename... Args>
	void trace(const char * fmt, const Args &... args) const
	{
		logger->trace(fmt, args...);
	}

	template <typename... Args>
	void debug(const char * fmt, const Args &... args) const
	{
		logger->debug(fmt, args...);
	}

	template <typename... Args>
	void info(const char * fmt, const Args &... args) const
	{
		logger->info(fmt, args...);
	}

	template <typename... Args>
	void warn(const char * fmt, const Args &... args) const
	{
		logger->warn(fmt, args...);
	}

	template <typename... Args>
	void error(const char * fmt, const Args &... args) const
	{
		logger->error(fmt, args...);
	}

	template <typename... Args>
	void critical(const char * fmt, const Args &... args) const
	{
		logger->critical(fmt, args...);
	}

	template <typename... Args>
	void log(Level level, const char * fmt, const Args &... args) const
	{
		switch (level)
		{
		case Level::Trace:
			trace(fmt, args...);
			break;
		case Level::Debug:
			debug(fmt, args...);
			break;
		case Level::Info:
			info(fmt, args...);
			break;
		case Level::Warn:
			warn(fmt, args...);
			break;
		case Level::Error:
			error(fmt, args...);
			break;
		case Level::Critical:
			critical(fmt, args...);
			break;
		default:
			break;
		}
	}

	static void setConsoleLoggingLevel(Level level);
	static Level getConsoleLoggingLevel();

	static void setFileLoggingLevel(Level level);
	static Level getFileLoggingLevel();

	static void setLogFileName(std::string name);
	static const std::string & getLogFileName();

	static const char * getLevelName(Level level);

	static void flush();

private:
	static spdlog::sink_ptr & consoleSink(std::shared_ptr<spdlog::logger> logger);
	static spdlog::sink_ptr & fileSink(std::shared_ptr<spdlog::logger> logger);

	static spdlog::sink_ptr createConsoleSink();
	static spdlog::sink_ptr createFileSink();

	static std::shared_ptr<spdlog::logger> createRootLogger();
	static std::shared_ptr<spdlog::logger> getRootLogger();

	std::shared_ptr<spdlog::logger> logger;

	static Level consoleLevel;
	static Level fileLevel;
	static std::string outputFileName;
};

#endif
