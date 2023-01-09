#include <Shared/Game/ScriptWorker.hpp>
#include <Shared/Lua/LuaManager.hpp>
#include <Shared/Utils/Debug/Logger.hpp>

namespace wos
{

ScriptWorker::ScriptWorker()
{
	interrupted = false;
	running = false;
}

ScriptWorker::~ScriptWorker()
{
	requestStop();
	join();
}

void ScriptWorker::start(std::string scriptSource)
{
	requestStop();
	join();
	interrupted = false;
	running = true;

	workerThread = std::thread(
	    [=]()
	    {
		    try
		    {
			    lua::LuaManager luaManager;

			    luaManager.loadBaseLibraries();

			    luaManager.bindFunction("read", //
			        std::function<sol::object(sol::this_state, bool)>(
			            [&](sol::this_state thisState, bool peek) -> sol::object
			            {
				            std::unique_lock<std::mutex> lock(mutex);
				            if (!peek && input.empty())
				            {
					            condition.wait(lock);
				            }

				            sol::state_view state(thisState);
				            if (interrupted || input.empty())
				            {
					            return sol::make_object(state, sol::lua_nil);
				            }
				            else
				            {
					            std::string value = std::move(input.front());
					            input.pop();
					            return sol::make_object(state, value);
				            }
			            }));

			    luaManager.bindFunction("write", //
			        std::function<void(std::string)>(
			            [&](std::string data)
			            {
				            std::unique_lock<std::mutex> lock(mutex);
				            output.push(std::move(data));
			            }));

			    auto script = luaManager.loadScript(scriptSource.data(), scriptSource.size(), "Worker");
			    luaManager.execute(script);
		    }
		    catch (std::exception & ex)
		    {
			    // TODO check thread-safety
			    Logger logger("WorkerThread");
			    logger.error("Error in Lua worker thread: {}", ex.what());
		    }

		    running = false;
	    });
}

void ScriptWorker::requestStop()
{
	if (workerThread.joinable())
	{
		interrupted = true;
		condition.notify_all();
	}
}

bool ScriptWorker::isRunning() const
{
	return running;
}

bool ScriptWorker::isStopRequested() const
{
	return interrupted;
}

void ScriptWorker::join()
{
	if (workerThread.joinable())
	{
		workerThread.join();
	}
}

void ScriptWorker::clearIO()
{
	std::unique_lock<std::mutex> lock(mutex);
	input = std::queue<std::string>();
	output = std::queue<std::string>();
}

void ScriptWorker::pushInput(std::string data)
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		input.push(std::move(data));
	}
	condition.notify_all();
}

bool ScriptWorker::hasOutput() const
{
	std::unique_lock<std::mutex> lock(mutex);
	return !output.empty();
}

std::string ScriptWorker::peekOutput()
{
	std::unique_lock<std::mutex> lock(mutex);
	return !output.empty() ? output.front() : "";
}

std::string ScriptWorker::popOutput()
{
	std::unique_lock<std::mutex> lock(mutex);
	if (output.empty())
	{
		return "";
	}
	else
	{
		std::string result = std::move(output.front());
		output.pop();
		return std::move(result);
	}
}

}