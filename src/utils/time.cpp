#include "time.hpp"

using namespace std::chrono;

namespace PT
{
namespace Time
{

    high_resolution_clock::time_point GetTimePoint()
    {
        return std::chrono::high_resolution_clock::now();
    }

    float GetDuration( const high_resolution_clock::time_point& point )
    {
        auto now = GetTimePoint();
        return duration_cast< microseconds >( now - point ).count() / static_cast< float >( 1000 );
    }

} // namespace Time
} // namespace PT
