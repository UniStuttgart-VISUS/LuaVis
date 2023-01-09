#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/OperatingSystem.hpp>
#include <Shared/Utils/Utilities.hpp>

#ifdef WOS_WINDOWS
#	include <cppfs/windows/FileNameConversions.h>
#	include <windows.h>
#elif defined(WOS_LINUX) || defined(WOS_OSX)
#	include <unistd.h>
#	if defined(WOS_LINUX)
#		include <wait.h>
#	endif
#endif

namespace os
{

bool openWithSystemHandler(const std::string & path)
{
#ifdef WOS_WINDOWS
	static const int successValue = 32;
	std::wstring widePath = cppfs::convert::utf8ToWideString(path);
	return reinterpret_cast<intptr_t>(
	           ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT))
	       > successValue;
#elif defined(WOS_LINUX)
	static const char * xdgOpen = "xdg-open";
	const char * pathStr = path.c_str();

	if (vfork())
	{
		return true;
	}
	else
	{
		execlp(xdgOpen, xdgOpen, pathStr, (const char *) nullptr);
		_exit(1);
	}
#elif defined(WOS_OSX)
	static const char * open = "open";
	const char * pathStr = path.c_str();

	if (vfork())
	{
		return true;
	}
	else
	{
		execlp(open, open, pathStr, (const char *) nullptr);
		_exit(1);
	}
#endif

	return false;
}

intptr_t spawnChildProcess(const std::vector<std::string> & args)
{
#ifdef WOS_WINDOWS
	std::wstring commandLine = cppfs::convert::utf8ToWideString(joinString(args, " "));
	WCHAR fileName[4096];
	if (GetModuleFileNameW(NULL, fileName, sizeof(fileName) / sizeof(WCHAR)) == 0)
	{
		return 0;
	}

	STARTUPINFOW startInfo;
	PROCESS_INFORMATION procInfo;

	ZeroMemory(&startInfo, sizeof(startInfo));
	ZeroMemory(&procInfo, sizeof(procInfo));
	startInfo.cb = sizeof(startInfo);

	if (CreateProcessW(fileName, &commandLine[0], NULL, NULL, false, 0, NULL, NULL, &startInfo, &procInfo) != 0)
	{
		return reinterpret_cast<intptr_t>(procInfo.hProcess);
	}

#elif defined(WOS_LINUX)
	std::vector<char> exePath(4096);
	auto result = readlink("/proc/self/exe", exePath.data(), exePath.size());
	if (result <= 0)
	{
		return 0;
	}
	exePath.resize(result);
	exePath.push_back('\0');

	std::vector<char *> argv;
	argv.push_back(exePath.data());
	for (std::size_t i = 0; i < args.size(); ++i)
	{
		argv.push_back(const_cast<char *>(args[i].c_str()));
	}
	argv.push_back(nullptr);

	pid_t pid = vfork();
	if (pid)
	{
		return pid;
	}
	else
	{
		execv(exePath.data(), argv.data());
		_exit(1);
	}
#elif defined(WOS_OSX)
	// TODO implement spawnChildProcess for macOS
#endif

	return 0;
}

bool isChildProcessRunning(intptr_t process)
{
#ifdef WOS_WINDOWS
	if (WaitForSingleObject(reinterpret_cast<HANDLE>(process), 0) == WAIT_TIMEOUT)
	{
		return true;
	}
#elif defined(WOS_LINUX)
	if (waitpid(process, nullptr, WNOHANG) == 0)
	{
		return true;
	}
#elif defined(WOS_OSX)
	// TODO implement isChildProcessRunning for macOS
#endif
	return false;
}
}
