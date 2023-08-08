#pragma once

#include <chrono>
#include <cstdint>

namespace OpenLoco::Core
{
    class Timer
    {
        using ClockType = std::chrono::high_resolution_clock;
        using TimePointType = std::chrono::time_point<ClockType>;

        TimePointType _start = ClockType::now();

    public:
        // Resets the timer
        void reset()
        {
            _start = ClockType::now();
        }

        // Returns elapsed time in milliseconds
        float elapsed() const
        {
            const auto now = ClockType::now();
            return std::chrono::duration<float, std::milli>(now - _start).count();
        }
    };
}
