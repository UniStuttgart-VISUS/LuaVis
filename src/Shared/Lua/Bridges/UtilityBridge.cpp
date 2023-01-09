#include <Shared/Content/ZipCreator.hpp>
#include <Shared/Lua/Bridges/UtilityBridge.hpp>
#include <Shared/Lua/LuaStringBuffer.hpp>
#include <Shared/Lua/LuaUtils.hpp>
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/OperatingSystem.hpp>
#include <Shared/Utils/Zlib.hpp>
#include <functional>
#include <string>
#include <vector>

#ifdef WOS_WINDOWS
#	include <direct.h>
#	include <cppfs/windows/FileNameConversions.h>
#elif defined(WOS_LINUX)
#	include <unistd.h>
#endif

namespace lua
{

UtilityBridge::UtilityBridge() :
	logger("UtilityBridge")
{
}

UtilityBridge::~UtilityBridge()
{
}

void UtilityBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("util.compress", std::function<sol::object(std::string, int, sol::this_state)>(
	                                 [=](std::string data, int level, sol::this_state state) -> sol::object {
		                                 if (zu::compress(data, level))
		                                 {
			                                 return sol::make_object(state, data);
		                                 }
		                                 else
		                                 {
			                                 return sol::make_object(state, sol::lua_nil);
		                                 }
	                                 }));

	loader.bind("util.decompress", std::function<sol::object(std::string, sol::this_state)>(
	                                   [=](std::string data, sol::this_state state) -> sol::object {
		                                   if (zu::decompress(data))
		                                   {
			                                   return sol::make_object(state, data);
		                                   }
		                                   else
		                                   {
			                                   return sol::make_object(state, sol::lua_nil);
		                                   }
	                                   }));

	loader.bind("util.zip", std::function<sol::object(sol::table, sol::this_state)>(
	                            [=](sol::table input, sol::this_state state) -> sol::object
	                            {
		                            if (!input.valid())
		                            {

			                            return sol::make_object(state, sol::lua_nil);
		                            }

		                            std::shared_ptr<DataStream> stream = std::make_shared<DataStream>();
		                            std::string result;
		                            res::ZipCreator zipper(stream);
		                            for (int i = 1; i <= input.size(); ++i)
		                            {
			                            sol::table entry = input[i];
			                            if (!entry.valid())
			                            {
				                            continue;
			                            }

			                            std::string path = entry["path"].get_or<std::string>("");
			                            if (path.empty())
			                            {
				                            continue;
			                            }

			                            if (entry["file"].get_type() == sol::type::string)
			                            {
				                            std::string file = entry["file"].get_or<std::string>("");
				                            zipper.addFromFile(path, file);
			                            }
			                            else if (entry["data"].get_type() == sol::type::string)
			                            {
				                            std::string data = entry["data"].get_or<std::string>("");
				                            zipper.addFromMemory(path, data.data(), data.size());
			                            }
			                            else if (entry["directory"].get_or(false))
			                            {
				                            zipper.addDirectory(path);
			                            }
		                            }
		                            zipper.finalize();

		                            stream->exportToString(result);
		                            return sol::make_object(state, result);
	                            }));

	loader.bind("util.openWithSystemHandler", std::function<bool(std::string)>([=](std::string path) {
		            return os::openWithSystemHandler(path);
	            }));

	loader.bind("util.getWorkingDirectory", //
	    std::function<std::string()>(
	        [=]() -> std::string
	        {
#ifdef WOS_WINDOWS
		        std::vector<wchar_t> buffer(4096);
		        if (_wgetcwd(buffer.data(), buffer.size() - 1))
		        {
			        return cppfs::convert::wideToUtf8String(std::wstring(buffer.data()));
		        }
#else
		        std::vector<char> buffer(4096);
		        if (getcwd(buffer.data(), buffer.size() - 1))
		        {
			        return std::string(buffer.data());
		        }
#endif
		        return "";
	        }));

	loader.bind("util.spawnChildProcess", //
	    std::function<std::string(sol::table)>(
	        [=](sol::table args)
	        {
		        return std::to_string(os::spawnChildProcess(lua::tableToVector<std::string>(args, "")));
	        }));

	loader.bind("util.isChildProcessRunning", //
	    std::function<bool(std::string)>(
	        [=](std::string pid)
	        {
		        return os::isChildProcessRunning(static_cast<intptr_t>(std::stoll(pid)));
	        }));

	loader.bind("util.determinizeStringBuffer",
	            std::function<sol::object(std::string, sol::this_state)>([=](std::string data, sol::this_state state) {
		            switch (lua::StringBufferParser::determinizeString(data))
		            {
		            case lua::StringBufferParser::DeterminizationResult::Success:
			            // String buffer determinized successfully: return modified string
			            return sol::make_object(state, data);

		            case lua::StringBufferParser::DeterminizationResult::Identical:
			            // String buffer already determinized: return nil to avoid re-interning
			            return sol::make_object(state, sol::lua_nil);

		            case lua::StringBufferParser::DeterminizationResult::Error:
		            default:
			            // Error occurred: return false
			            return sol::make_object(state, false);
		            }
	            }));
}

}
