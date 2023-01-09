#include <Shared/Utils/Debug/StackTrace.hpp>
#include <Shared/Utils/Debug/TypeInfo.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>

#if defined WOS_WINDOWS || defined WOS_OSX


#elif defined WOS_LINUX
#	include <cxxabi.h>
#	include <execinfo.h>
#	include <regex>

static std::string demangleSubstring(std::string symbolString)
{
	std::regex symbolRegex("(.*\\()([a-zA-Z0-9_]+)(\\)|\\+)?(.*)");
	std::smatch matches;

	if (std::regex_match(symbolString, matches, symbolRegex))
	{
		std::string demangled;
		for (std::size_t i = 1; i < matches.size(); ++i)
		{
			if (i == 2)
			{
				demangled += cxaDemangle(matches[i]);
			}
			else
			{
				demangled += matches[i];
			}
		}
		return demangled;
	}
	else
	{
		return symbolString;
	}
}

#endif

StackTrace StackTrace::generate(std::size_t ignoreFrames)
{
	StackTrace trace;

#if defined WOS_WINDOWS || defined WOS_OSX

#elif defined WOS_LINUX

	// Disabled for now - this can crash if the stack contains LuaJIT frames
	// constexpr std::size_t maxFrames = 256;
	// std::vector<void *> frames(maxFrames + ignoreFrames);
	// frames.resize(backtrace(frames.data(), frames.size()));
	// char ** symbols = backtrace_symbols(frames.data(), frames.size());
	// for (std::size_t i = ignoreFrames; i < frames.size(); ++i)
	// {
	// 	trace.frames.push_back(demangleSubstring(symbols[i]));
	// }
	// std::free(symbols);

#endif

	return trace;
}

StackTrace::Iterator StackTrace::begin() const
{
	return frames.begin();
}

StackTrace::Iterator StackTrace::end() const
{
	return frames.end();
}

std::size_t StackTrace::getFrameCount() const
{
	return frames.size();
}

const std::string & StackTrace::getFrame(std::size_t frameNumber) const
{
	static std::string emptyString;
	return frameNumber < frames.size() ? frames[frameNumber] : emptyString;
}

std::string StackTrace::toString() const
{
	std::ostringstream str;
	for (const std::string & frame : frames)
	{
		str << frame << '\n';
	}
	return str.str();
}
