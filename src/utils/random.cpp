#include "utils/random.hpp"
#include <random>

namespace PT
{
namespace Random
{

float Rand()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 generator( rd() );
    std::uniform_real_distribution< float > distribution( 0, 1 );
    return distribution( generator );
    //return rand() / (float) RAND_MAX;
}

float RandFloat( float l, float h )
{
    return Rand() * (h - l) + l;
}

} // namespace Random
} // namespace PT