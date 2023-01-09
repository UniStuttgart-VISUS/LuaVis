#include <Client/GUI3/Events/KeyNative.hpp>
#include <Shared/Utils/OSDetect.hpp>

#include <array>

#ifdef WOS_WINDOWS
#	include <cppfs/windows/FileNameConversions.h>
#	include <windows.h>
#endif

#ifdef WOS_LINUX
#	include <X11/Xlib.h>
#endif

namespace gui3
{
namespace detail
{

std::string getNativeKeyName(std::int32_t code)
{
#ifdef WOS_WINDOWS
	std::array<wchar_t, 256> name;
	if (int length = GetKeyNameTextW(MapVirtualKeyW(code, MAPVK_VK_TO_VSC) << 16, name.data(), name.size()))
	{
		return cppfs::convert::wideToUtf8String(std::wstring(name.data(), name.data() + length));
	}
#endif

#ifdef WOS_LINUX
	const char * keySym = XKeysymToString(code);
	if (keySym != nullptr)
	{
		return keySym;
	}
#endif

	return "";
}

}
}