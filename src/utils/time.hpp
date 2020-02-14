#pragma once

#include <chrono>

namespace PT
{
namespace Time
{

    std::chrono::high_resolution_clock::time_point GetTimePoint();

    // Returns the number of milliseconds elapsed since the given point in time
    float GetDuration( const std::chrono::high_resolution_clock::time_point& point );

} // namespace Time
} // namespace PT
