#include <Client/GUI3/ResourceManager.hpp>
#include <SFML/System/InputStream.hpp>
#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Content/ZipSource.hpp>
#include <Shared/Game/AsyncPackageCompiler.hpp>
#include <Shared/Game/ResourceLoader.hpp>
#include <Shared/Lua/Bridges/ResourceBridge.hpp>
#include <Shared/Lua/LuaUtils.hpp>
#include <Shared/Utils/FileChooser.hpp>
#include <Shared/Utils/Filesystem/LocalStorage.hpp>
#include <Shared/Utils/Hash.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Sol2/sol.hpp>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lua
{

ResourceBridge::ResourceBridge(wos::ResourceLoader & resourceLoader) :
	resourceLoader(resourceLoader),
	logger("ResourceBridge")
{
	resourceCallback = resourceLoader.getCombinedSource().addCallback(
	    [=](res::ResourceEvent event) {
		    handleResourceEvent(event);
	    },
	    res::ResourceEvent::Any, 0);
}

ResourceBridge::ResourceBridge(
    wos::ResourceLoader & aggregator, wos::AsyncPackageCompiler & packageCompiler, ThreadPool & threadPool) :
    ResourceBridge(aggregator)
{
	this->packageCompiler = &packageCompiler;
	this->threadPool = &threadPool;
}

ResourceBridge::~ResourceBridge()
{
}

void ResourceBridge::handleResourceEvent(res::ResourceEvent event)
{
	logger.trace("Resource '{}' changed.", event.resourceName);
	changedResources[event.resourceName] |= event.type;
}

struct StoragePath
{
	StoragePath(fs::LocalStorage & localStorage, std::string path);
	static StoragePath getPath(const sol::object & object);

	fs::LocalStorage & localStorage;
	std::string path;
};

StoragePath::StoragePath(fs::LocalStorage & localStorage, std::string path) :
	localStorage(localStorage),
	path(path)
{
}

StoragePath StoragePath::getPath(const sol::object & object)
{
	if (object.is<sol::table>())
	{
		auto table = object.as<sol::table>();
		return StoragePath(
		    fs::LocalStorage::getInstance(static_cast<fs::LocalStorage::Path>(lua::getOr<int>(table[1], -1))),
		    lua::getOr<std::string>(table[2], ""));
	}
	else
	{
		return StoragePath(fs::LocalStorage::getInstance(fs::LocalStorage::Path::UserData),
		                   object.is<std::string>() ? object.as<std::string>() : "");
	}
}


void ResourceBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("res.pollChanges", std::function<sol::optional<std::tuple<std::string, int>>()>(
	                                   [=]() -> sol::optional<std::tuple<std::string, int>> {
		                                   if (changedResources.empty())
		                                   {
			                                   return sol::nullopt;
		                                   }
		                                   else
		                                   {
			                                   auto resIter = changedResources.begin();
			                                   auto resData = *resIter;
			                                   changedResources.erase(resIter);
			                                   return std::make_tuple(resData.first, int(resData.second));
		                                   }
	                                   }));

	loader.bind("res.readFileToString", //
	    std::function<sol::object(sol::this_state, std::string, std::int32_t)>(
	        [=](sol::this_state state, std::string fileName, std::int32_t maxBytes) -> sol::object
	        {
		        std::vector<char> data;
		        if (maxBytes < 0 || maxBytes > 65536)
		        {
			        if (resourceLoader.getCombinedSource().loadResource(fileName, data))
			        {
				        return sol::make_object(sol::state_view(state),
				            std::string(data.data(),
				                        maxBytes < 0 ? data.size() : std::min<std::size_t>(data.size(), maxBytes)));
			        }
		        }
		        else
		        {
			        data.resize(maxBytes);
			        if (resourceLoader.getCombinedSource().loadResourceLimited(fileName, data))
			        {
				        return sol::make_object(sol::state_view(state), std::string(data.data(), data.size()));
			        }
		        }

		        return sol::lua_nil;
	        }));

	loader.bind("res.getHash", std::function<sol::optional<std::string>(sol::this_state, std::string)>(
	                               [=](sol::this_state state, std::string fileName) -> sol::optional<std::string> {
		                               if (auto stream = resourceLoader.getCombinedSource().openStream(fileName))
		                               {
			                               hash::Blake256 hashValue = hash::computeBlake256(*stream);
			                               return std::string(hashValue.begin(), hashValue.end());
		                               }
		                               else
		                               {
			                               return sol::nullopt;
		                               }
	                               }));

	loader.bind("res.resourceExists",
	            std::function<bool(sol::this_state, std::string)>([=](sol::this_state state, std::string fileName) {
		            return resourceLoader.getCombinedSource().resourceExists(fileName);
	            }));

	loader.bind("res.fireEvent", //
	    std::function<void(std::string, int eventType)>(
	        [=](std::string fileName, int eventType)
	        {
		        resourceLoader.getCombinedSource().fireEvent(
		            res::ResourceEvent(res::ResourceEvent::Type(eventType), fileName));
	        }));

	loader.bind("res.listResources",
	            std::function<sol::object(sol::this_state, std::string, int)>(
	                [=](sol::this_state state, std::string path, int flags) -> sol::object {
		                return lua::listToTable(
		                    resourceLoader.getCombinedSource().getResourceList(path, fs::ListFlags(flags)), state);
	                }));

	loader.bind("res.getListFlags",
	            std::function<sol::table(sol::this_state)>([=](sol::this_state state) -> sol::table {
		            sol::table flags = sol::state_view(state).create_table();
		            flags["FILES"] = int(fs::ListFiles);
		            flags["DIRECTORIES"] = int(fs::ListDirectories);
		            flags["RECURSIVE"] = int(fs::ListRecursive);
		            flags["SORTED"] = int(fs::ListSorted);
		            flags["FULL_PATH"] = int(fs::ListFullPath);
		            return flags;
	            }));

	loader.bind("res.addDirectorySource", std::function<void(std::string, std::string, int, std::string, bool, bool)>(
	                                          [=](std::string label, std::string mountPoint, int order,
	                                              std::string path, bool external, bool autoReload) {
		                                          resourceLoader.addDirectorySource(label, mountPoint, order, path,
		                                                                            external, autoReload);
	                                          }));

	auto isZipFile = [](const std::string & name)
	{
		// Check for the presence of the PKZip header
		return readFileToString(name, 4) == "PK\x03\x04";
	};

	loader.bind("res.addPackageSource", std::function<void(std::string, std::string, int, std::string)>(
	                                        [=](std::string label, std::string mountPoint, int order, std::string path)
	                                        {
		                                        if (isZipFile(path))
		                                        {
			                                        resourceLoader.addZipSource(label, mountPoint, order, path);
		                                        }
		                                        else
		                                        {
			                                        resourceLoader.addPackageSource(label, mountPoint, order, path);
		                                        }
	                                        }));

	loader.bind("res.addFunctionSource",
	            std::function<void(std::string, std::string, int, sol::function)>(
	                [=](std::string label, std::string mountPoint, int order, sol::function function) {
		                resourceLoader.addLuaFunctionSource(label, mountPoint, order, function);
	                }));

	loader.bind("res.removeSource", std::function<void(std::string)>([=](std::string label) {
		            resourceLoader.removeSource(label);
	            }));

	loader.bind("res.setSourceOrder", std::function<void(std::string, int)>([=](std::string label, int order) {
		            resourceLoader.setSourceOrder(label, order);
	            }));

	loader.bind("res.clearSources", std::function<void()>([=]() {
		            resourceLoader.removeAllSources();
	            }));

	loader.bind("res.storage.resolve", std::function<std::string(sol::object)>([=](sol::object path) {
		            auto storage(StoragePath::getPath(path));
		            return storage.localStorage.resolve(storage.path);
	            }));

	loader.bind(
	    "res.storage.readFile",
	    std::function<sol::object(sol::object, std::int32_t maxBytes, sol::this_state)>(
	        [=](sol::object path, std::int32_t maxBytes, sol::this_state state) -> sol::object {
		        auto storage(StoragePath::getPath(path));
		        auto stream = storage.localStorage.openInputStream(storage.path);
		        if (stream.isOpen())
		        {
			        std::string data;
			        if (stream.exportToString(data, maxBytes < 0 ? std::numeric_limits<std::size_t>::max() : maxBytes))
			        {
				        return sol::make_object(state, data);
			        }
		        }
		        return sol::lua_nil;
	        }));

	loader.bind("res.storage.writeFile",
	            std::function<bool(sol::object, std::string)>([=](sol::object path, std::string data) {
		            auto storage(StoragePath::getPath(path));
		            auto stream = storage.localStorage.openOutputStream(storage.path);
		            if (stream.isOpen())
		            {
			            stream.addData(data.data(), data.size());
		            }
		            return stream.isOpen();
	            }));

	loader.bind("res.storage.writeFileAsync", //
	    std::function<bool(sol::object, std::string)>(
	        [=](sol::object path, std::string data) -> bool
	        {
		        if (threadPool)
		        {
			        auto storage(StoragePath::getPath(path));
			        return storage.localStorage.writeAsync(storage.path, data, *threadPool);
		        }
		        else
		        {
			        return false;
		        }
	        }));

	loader.bind("res.storage.createDirectory",
	            std::function<bool(sol::object, bool)>([=](sol::object path, bool recursive) {
		            auto storage(StoragePath::getPath(path));
		            return storage.localStorage.createDirectory(storage.path, recursive);
	            }));

	loader.bind("res.storage.delete", std::function<bool(sol::object, bool)>([=](sol::object path, bool recursive) {
		            auto storage(StoragePath::getPath(path));
		            return storage.localStorage.isRegularFile(storage.path)
		                       ? storage.localStorage.deleteFile(storage.path)
		                       : storage.localStorage.deleteDirectory(storage.path, recursive);
	            }));

	loader.bind("res.storage.move",
	            std::function<bool(sol::object, std::string)>([=](sol::object source, std::string target) {
		            auto storage(StoragePath::getPath(source));
		            return storage.localStorage.moveFile(storage.path, target);
	            }));

	loader.bind("res.storage.getInfo",
	            std::function<sol::object(sol::object, int, sol::this_state)>(
	                [=](sol::object path, int flags, sol::this_state state) -> sol::object {
		                auto storage(StoragePath::getPath(path));
		                if (storage.localStorage.fileExists(storage.path))
		                {
			                auto table = sol::state_view(state).create_table();

			                if (storage.localStorage.isRegularFile(storage.path))
			                {
				                table["file"] = true;
			                }
			                else if (storage.localStorage.isDirectory(storage.path))
			                {
				                table["directory"] = true;
				                table["contents"] = lua::listToTable(
				                    storage.localStorage.list(storage.path, fs::ListFlags(flags)), state);
			                }

			                return table;
		                }
		                else
		                {
			                return sol::lua_nil;
		                }
	                }));

	loader.bind("res.storage.peekPackageFile",
	            std::function<sol::optional<std::string>(sol::object, std::string)>(
	                [=](sol::object packageName, std::string resourceName) -> sol::optional<std::string> {
		                auto storage(StoragePath::getPath(packageName));
		                res::Package package;
		                std::string fileName = storage.localStorage.resolve(storage.path);
		                if (isZipFile(fileName))
		                {
			                res::ZipSource source(fileName);
			                std::vector<char> data;
			                if (source.loadResource(resourceName, data))
			                {
				                return std::string(data.begin(), data.end());
			                }
		                }
		                else if (package.openFile(fileName) && package.select(resourceName))
		                {
			                auto content = package.getContentData();
			                return std::string(content.begin(), content.end());
		                }
		                return sol::nullopt;
	                }));

	// clang-format off
	loader.registerType<FileChooser>("FileChooser",
		"setTitle", &FileChooser::setTitle,
		"setDefaultPath", &FileChooser::setDefaultPath,
		"setFilterDescription", &FileChooser::setFilterDescription,
		"setFilterPatterns", [](FileChooser & chooser, std::string patterns) {
			chooser.setFilterPatterns(splitString(patterns, std::string(1, '\0'), true));
		},
		"setMultiSelectAllowed", &FileChooser::setMultiSelectAllowed,
		"setAutoSetDefault", &FileChooser::setAutoSetDefault,
		"isOpen", &FileChooser::isOpen,
		"isDone", &FileChooser::isDone,
		"isCancelled", &FileChooser::isCancelled,
		"clear", &FileChooser::clear,
		"getSelectedFile", [](FileChooser & chooser, int index) -> std::string {
			auto files = chooser.getSelectedFiles();
			return index >= 0 && index < files.size() ? files[index] : "";
		},
		"getSelectedFileCount", [](FileChooser & chooser) -> int {
			return chooser.getSelectedFiles().size();
		},
		"showDialog", &FileChooser::showDialog);
	// clang-format on

	loader.bind("res.fileChooser.new", //
	    std::function<std::shared_ptr<FileChooser>()>(
	        [=]()
	        {
		        return std::make_shared<FileChooser>();
	        }));

	loader.bind("res.readAbsolutePathFileToString", //
	    std::function<std::string(std::string, std::int32_t)>(
	        [=](std::string filename, std::int32_t maxBytes)
	        {
		        return readFileToString(filename, maxBytes < 0 ? std::numeric_limits<std::size_t>::max() : maxBytes);
	        }));

	if (packageCompiler)
	{
		loader.bind("res.compiler.createPackage",
		            std::function<bool(std::string, std::string, sol::object)>(
		                [=](std::string sourceDirectory, std::string packageFile, sol::object fileList) {
			                return packageCompiler->compile(sourceDirectory, lua::tableToVector<std::string>(fileList),
			                                                packageFile);
		                }));

		loader.bind("res.compiler.getStatus", std::function<int(std::string)>([=](std::string packageFile) {
			            return packageCompiler->getStatus(packageFile);
		            }));

		loader.bind("res.compiler.getProgress", std::function<float(std::string)>([=](std::string packageFile) {
			            return packageCompiler->getProgress(packageFile);
		            }));

		loader.bind("res.compiler.pollLogOutput",
		            std::function<sol::optional<std::tuple<std::string, std::string>>(std::string)>(
		                [=](std::string packageFile) -> sol::optional<std::tuple<std::string, std::string>> {
			                Logger::Level level;
			                std::string message;

			                if (packageCompiler->pollLogOutput(packageFile, level, message))
			                {
				                return std::make_tuple(std::string(Logger::getLevelName(level)), message);
			                }
			                else
			                {
				                return sol::nullopt;
			                }
		                }));

		loader.bind("res.compiler.release", std::function<void(std::string)>([=](std::string packageFile) {
			            packageCompiler->release(packageFile);
		            }));
	}
}

}
