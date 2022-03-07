#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class Timer
{
public:
	Timer()
	{
		start = std::chrono::high_resolution_clock::now();
		lastUpdate = start;
		currentUpdate = start;
	}
	void Update()
	{
		lastUpdate = currentUpdate;
		currentUpdate = std::chrono::high_resolution_clock::now();
	}

	long long FrameElapsed()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(currentUpdate - lastUpdate).count();
	}

private:
	std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::duration<long int, std::ratio<1, 1000000000> > > start;
	std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::duration<long int, std::ratio<1, 1000000000> > > lastUpdate;
	std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::duration<long int, std::ratio<1, 1000000000> > > currentUpdate;
};



#endif
