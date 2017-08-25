#pragma once

#include <chrono>
#include <functional>

typedef std::chrono::steady_clock Clock;
typedef Clock::duration Duration;
typedef Clock::time_point TimePoint;

typedef std::function<void()> Task;
