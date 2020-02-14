#pragma once

#include "math.hpp"

namespace PT
{
namespace intersect
{

bool RaySphere( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& spherePos, float sphereRadius, float& t );

} // namespace intersect 
} // namespace PT