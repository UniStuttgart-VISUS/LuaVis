#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Debug/CrashHandler.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <cstdio>
#include <iostream>

namespace
{

std::string dumpFileName;

}

#if defined(WOS_WINDOWS) && !defined(__MINGW32__)

#	include <cppfs/windows/FileNameConversions.h>

bool exitRequested = false;

void CrashHandler::init(std::string dumpfile)
{
}

void CrashHandler::setMetadata(MetaKey key, std::string value)
{
}

void CrashHandler::initThread()
{
}

#else

#	include <Shared/Utils/Debug/StackTrace.hpp>
#	include <csignal>

namespace
{

volatile sig_atomic_t exitRequested = 0;

void handleCrashSignal(int sig)
{
	std::signal(sig, SIG_DFL);

	DataStream stream;
	stream.openOutFile(dumpFileName);

	auto trace = StackTrace::generate(0).toString();

	stream.close();

	std::raise(SIGABRT);
}

void handleExitSignal(int sig)
{
	std::signal(sig, SIG_DFL);
	exitRequested = 1;
}

}
void CrashHandler::init(std::string dumpfile)
{
#	ifndef WOS_OSX
	dumpFileName = dumpfile;
	std::signal(SIGSEGV, &handleCrashSignal);
	std::signal(SIGABRT, &handleCrashSignal);
	std::signal(SIGINT, &handleExitSignal);
	std::signal(SIGTERM, &handleExitSignal);
#	endif
}

void CrashHandler::setMetadata(MetaKey key, std::string value)
{
	// NYI on MacOS/Linux
}

void CrashHandler::initThread()
{
	// Not necessary on MacOS/Linux
}

#endif

bool CrashHandler::dumpExists()
{
	return !dumpFileName.empty() && fileExists(dumpFileName);
}

std::string CrashHandler::readErrorFromDump()
{
	return readFileToString(dumpFileName);
}

void CrashHandler::removeDumpFile()
{
	std::remove(dumpFileName.c_str());
}

bool CrashHandler::isExitRequested()
{
	return exitRequested;
}

CrashHandler::CrashHandler()
{
}
