/*
 * Application.cpp
 *
 *  Created on: May 22, 2015
 *      Author: marukyu
 */

#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/ResourceManager.hpp>
#include <SFML/System/Time.hpp>
#include <iterator>

namespace sf
{
class Texture;
}

namespace gui3
{

static constexpr int FpsLimitVSync = -1;

Application::Application()
{
	setFramerateLimit(60);
}

Application::~Application()
{
}

int Application::run(const std::vector<std::string> & args)
{
	int initRet = init(args);

	if (initRet != 0)
	{
		return initRet;
	}
	else
	{
		while (!myInterfaces.empty())
		{
			myLastFrameTime = myFrameClock.restart();

			for (auto it = myInterfaces.begin(); it != myInterfaces.end();)
			{
				if ((*it)->isWindowOpen())
				{
#ifdef WOS_DEBUG
					(*it)->process();
#else
					try
					{
						(*it)->process();
					}
					catch (std::exception & exception)
					{
						handleError(exception);
					}
#endif
					++it;
				}
				else
				{
					it = myInterfaces.erase(it);
				}
			}
#ifdef WOS_DEBUG
			runInvokeLaterFunctions();
#else
			try
			{
				runInvokeLaterFunctions();
			}
			catch (std::exception & exception)
			{
				handleError(exception);
			}
#endif

			// Sleep if necessary.
			if (getFramerateLimit() > 0)
			{
				myLastSleepTime = myFramerateTimer.tick();
			}
			else
			{
				myLastSleepTime = sf::Time::Zero;
			}
		}
	}

	return 0;
}

Interface * Application::open()
{
	Interface * newInterface = makeInterface();
	newInterface->openWindow();

	myInterfaces.push_back(std::unique_ptr<Interface>(newInterface));
	updateVerticalSync();
	return newInterface;
}

void Application::close(const Interface * interface)
{
	for (auto it = myInterfaces.begin(); it != myInterfaces.end(); ++it)
	{
		if (it->get() == interface)
		{
			(*it)->closeWindow();
			invokeLater([=]() {
				updateVerticalSync();
			});
			return;
		}
	}
}

std::vector<Interface *> Application::getOpenInterfaces() const
{
	std::vector<Interface *> interfaceList;

	for (auto it = myInterfaces.begin(); it != myInterfaces.end(); ++it)
	{
		interfaceList.push_back(it->get());
	}

	return interfaceList;
}

void Application::exit()
{
	for (auto it = myInterfaces.begin(); it != myInterfaces.end(); ++it)
		(*it)->closeWindow();

	myInterfaces.clear();
}

void Application::setFramerateLimit(int limit)
{
	myFramerateLimit = limit;

	if (limit > 0)
	{
		myFramerateTimer.setFrameTime(sf::microseconds(1000000 / limit));
	}

	updateVerticalSync();
}

int Application::getFramerateLimit() const
{
	return myFramerateLimit;
}

const sf::Texture * Application::getTexture(std::size_t pageIndex) const
{
	return nullptr;
}

const sf::Texture * Application::getMainTexture() const
{
	return getTexture(0);
}

int Application::init(const std::vector<std::string> & args)
{
	return 0;
}

void Application::handleError(std::exception & exception)
{
	throw;
}

Interface * Application::makeInterface()
{
	return new Interface(this);
}

void Application::cleanUpBeforeExit()
{
}

InvocationHandle::~InvocationHandle()
{
}

void InvocationHandle::invoke()
{
	if (!disabled)
	{
		function();
		disabled = true;
	}
}

void InvocationHandle::remove()
{
	disabled = true;
}

bool InvocationHandle::isActive() const
{
	return !disabled;
}

InvocationHandle::InvocationHandle(std::function<void()> function, int order) :
	disabled(false),
	function(function),
	order(order)
{
}

std::weak_ptr<InvocationHandle> Application::invokeLater(std::function<void()> function, int order)
{
	auto it = std::upper_bound(myInvocationHandles.begin(), myInvocationHandles.end(), order,
	                           [](int order, const std::shared_ptr<InvocationHandle> & handle) {
		                           return order < handle->order;
	                           });

	auto handle = std::make_shared<InvocationHandle>(function, order);
	myInvocationHandles.insert(it, handle);
	return handle;
}

sf::Time Application::getTimeSinceFrameStart() const
{
	return myFrameClock.getElapsedTime();
}

sf::Time Application::getLastFrameTime() const
{
	return myLastFrameTime;
}

sf::Time Application::getLastSleepTime() const
{
	return myLastSleepTime;
}

void Application::cleanUpWindowResources()
{
	std::size_t openInterfaceCount = 0;
	for (auto it = myInterfaces.begin(); it != myInterfaces.end(); ++it)
	{
		if ((**it).isWindowOpen())
		{
			openInterfaceCount++;
		}
	}

	if (openInterfaceCount == 1)
	{
		cleanUpBeforeExit();
	}
}

void Application::runInvokeLaterFunctions()
{
	for (const auto & handle : myInvocationHandles)
	{
		handle->invoke();
	}
	myInvocationHandles.clear();
}

void Application::updateVerticalSync()
{
	for (std::size_t i = 0; i < myInterfaces.size(); ++i)
	{
		// Only enable VSync for the first open interface, and only if the special VSync FPS limit value is set
		myInterfaces[i]->setVerticalSyncEnabled(myFramerateLimit == FpsLimitVSync && i == 0);
	}
}

}
