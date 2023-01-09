#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <Shared/Utils/DebugLog.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/Timer.hpp>
#include <algorithm>

Timer::Timer()
{
	setTime(sf::milliseconds(1000));
}

Timer::Timer(sf::Time time)
{
	setTime(time);
}

bool Timer::tick()
{
	if (clock.getElapsedTime() - myResetTime >= myTime)
	{ // timer ticked.
		myResetTime += myTime;
		return true;
	}
	else
		// timer didn't tick.
		return false;
}

void Timer::reset()
{
	myResetTime = clock.getElapsedTime();
}

void Timer::resetHigh()
{
	myResetTime = clock.getElapsedTime() - myTime;
}

void Timer::setTime(sf::Time time)
{
	myTime = time;
	reset();
}

sf::Time Timer::getTime() const
{
	return myTime;
}

float Timer::getCurrentRatio() const
{
	return (clock.getElapsedTime() - myResetTime).asSeconds() / myTime.asSeconds();
}

sf::Time Timer::getGlobalTime()
{
	static sf::Clock clock;
	return clock.getElapsedTime();
}

FramerateTimer::FramerateTimer()
{
	setFrameTime(sf::microseconds(1000000 / 60));
}

FramerateTimer::FramerateTimer(sf::Time frameTime)
{
	setFrameTime(frameTime);
}

sf::Time FramerateTimer::tick()
{
	sf::Clock effectiveSleepDuration;

	sf::Time requestedSleepTime = mySleepTime - clock.getElapsedTime() - sf::milliseconds(1);
	if (requestedSleepTime < sf::Time::Zero)
	{
		reset();
	}
	else
	{
		sf::sleep(requestedSleepTime);
		mySleepTime += myFrameTime;
	}

	return effectiveSleepDuration.getElapsedTime();
}

void FramerateTimer::reset()
{
	mySleepTime = clock.getElapsedTime() + myFrameTime;
}

void FramerateTimer::setFrameTime(sf::Time frameTime)
{
	myFrameTime = frameTime;
	reset();
}

sf::Time FramerateTimer::getFrameTime() const
{
	return myFrameTime;
}

Countdown::Countdown()
{
	restart(sf::Time::Zero);
}

Countdown::Countdown(sf::Time limit)
{
	restart(limit);
}

void Countdown::restart(sf::Time limit)
{
	myTimerEnd = clock.getElapsedTime() + limit;
}

sf::Time Countdown::getRemainingTime() const
{
	return std::max(myTimerEnd - clock.getElapsedTime(), sf::Time::Zero);
}

sf::Time Countdown::getOvertime() const
{
	return clock.getElapsedTime() - myTimerEnd;
}

bool Countdown::expired() const
{
	return clock.getElapsedTime() >= myTimerEnd;
}

Stopwatch::Stopwatch()
{
	reset();
}

void Stopwatch::start()
{
	if (isRunning())
		return;

	myIsRunning = true;
	myClock.restart();
}

void Stopwatch::stop()
{
	if (!isRunning())
		return;

	myTotalTime += myClock.getElapsedTime();
	myIsRunning = false;
}

void Stopwatch::reset()
{
	myTotalTime = sf::Time::Zero;
	myIsRunning = false;
}

bool Stopwatch::isRunning() const
{
	return myIsRunning;
}

sf::Time Stopwatch::getTime() const
{
	if (isRunning())
		return myTotalTime + myClock.getElapsedTime();
	else
		return myTotalTime;
}
