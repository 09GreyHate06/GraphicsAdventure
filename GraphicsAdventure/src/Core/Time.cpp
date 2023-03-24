#include "Time.h"

using namespace std::chrono;

namespace GA
{
	Time::Time()
		: m_deltaTime(0.0f)
	{
		m_startTime = high_resolution_clock::now();
		m_lastFrame = steady_clock::now();
	}

	float Time::GetTime()
	{
		auto currentTime = high_resolution_clock::now();
		return duration<float>(currentTime - m_startTime).count();
	}

	void Time::UpdateDeltaTime()
	{
		const auto old = m_lastFrame;
		m_lastFrame = steady_clock::now();
		const duration<float> frametime = m_lastFrame - old;
		m_deltaTime = frametime.count();
	}

	Timer::Timer()
	{
		m_last = steady_clock::now();
	}

	float Timer::Mark()
	{
		const auto old = m_last;
		m_last = steady_clock::now();
		const duration<float> frametime = m_last - old;
		return frametime.count();
	}

	float Timer::Peek() const
	{
		return duration<float>(steady_clock::now() - m_last).count();
	}

}