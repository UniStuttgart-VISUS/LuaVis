/*
 * World of Sand C++ source code, Copyright Marukyu 2011-2020
 */

#include <Client/System/WOSClient.hpp>
#include <string>
#include <vector>

#ifdef _MSC_VER
#	include <windows.h>
#endif

int main(int argc, char * argv[])
{
	std::vector<std::string> args(argv, argv + argc);
	WOSClient client;
	return client.run(args);
}

#ifdef _MSC_VER
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
