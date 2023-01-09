#include <Shared/Utils/Debug/Logger.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Logger::Level Logger::consoleLevel = Logger::Level::WOS_LOG_LEVEL;
Logger::Level Logger::fileLevel = Logger::Level::WOS_LOG_LEVEL;
std::string Logger::outputFileName = "";

static spdlog::level::level_enum getSpdLogLevel(Logger::Level level)
{
	switch (level)
	{
	case Logger::Level::Trace:
		return spdlog::level::trace;
	case Logger::Level::Debug:
		return spdlog::level::debug;
	case Logger::Level::Info:
	default:
		return spdlog::level::info;
	case Logger::Level::Warn:
		return spdlog::level::warn;
	case Logger::Level::Error:
		return spdlog::level::err;
	case Logger::Level::Critical:
		return spdlog::level::critical;
	case Logger::Level::Off:
		return spdlog::level::off;
	}
}

Logger::Logger(std::string name) :
	Logger(name, Logger::Level::Debug)
{
}

Logger::Logger(std::string name, Level minimumLevel)
{
	logger = spdlog::get(name);
	if (logger == nullptr)
	{
		logger = getRootLogger()->clone(name);
		logger->set_level(getSpdLogLevel(minimumLevel));
		spdlog::register_logger(logger);
	}
}

Logger::~Logger()
{
}

void Logger::setConsoleLoggingLevel(Level level)
{
	if (consoleLevel != level)
	{
		consoleLevel = level;
		consoleSink(getRootLogger())->set_level(getSpdLogLevel(level));
	}
}

Logger::Level Logger::getConsoleLoggingLevel()
{
	return consoleLevel;
}

void Logger::setFileLoggingLevel(Level level)
{
	if (fileLevel != level)
	{
		fileLevel = level;
		fileSink(getRootLogger())->set_level(getSpdLogLevel(level));
	}
}

Logger::Level Logger::getFileLoggingLevel()
{
	return fileLevel;
}

const char * Logger::getLevelName(Level level)
{
	return spdlog::level::to_c_str(getSpdLogLevel(level));
}

void Logger::flush()
{
	fileSink(getRootLogger())->flush();
}

spdlog::sink_ptr & Logger::consoleSink(std::shared_ptr<spdlog::logger> logger)
{
	return logger->sinks()[0];
}

spdlog::sink_ptr & Logger::fileSink(std::shared_ptr<spdlog::logger> logger)
{
	return logger->sinks()[1];
}

spdlog::sink_ptr Logger::createConsoleSink()
{
	auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	sink->set_level(getSpdLogLevel(consoleLevel));
	return sink;
}

spdlog::sink_ptr Logger::createFileSink()
{
	spdlog::sink_ptr sink;
	if (outputFileName.empty())
	{
		sink = std::make_shared<spdlog::sinks::null_sink_mt>();
	}
	else
	{
		sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(outputFileName, true);
	}
	sink->set_level(getSpdLogLevel(fileLevel));
	return sink;
}

std::shared_ptr<spdlog::logger> Logger::createRootLogger()
{
	static const std::string rootLoggerName = "Root";
	auto logger = spdlog::get(rootLoggerName);
	if (logger == nullptr)
	{
		logger = std::make_shared<spdlog::logger>(rootLoggerName,
		                                          spdlog::sinks_init_list {createConsoleSink(), createFileSink()});
		spdlog::register_logger(logger);
	}
	return logger;
}

std::shared_ptr<spdlog::logger> Logger::getRootLogger()
{
	static auto logger = createRootLogger();
	return logger;
}

void Logger::setLogFileName(std::string name)
{
	if (outputFileName != name)
	{
		outputFileName = name;
		auto newFileSink = createFileSink();
		spdlog::apply_all([&](std::shared_ptr<spdlog::logger> logger) {
			fileSink(logger) = newFileSink;
		});
	}
}

const std::string & Logger::getLogFileName()
{
	return outputFileName;
}
