#pragma once

namespace PT
{
namespace Random
{

void SetSeed( size_t seed );

int RandInt( int l, int h );

float RandFloat( float l, float h );

float Rand();

} // namespace Random
} // namespace PT