#pragma once

#include "math.hpp"

namespace PT
{

struct Material;
struct IntersectionData
{
    glm::vec3 position;
    glm::vec3 normal;
    Material* material;
    float t = FLT_MAX;
};

namespace intersect
{

    bool RaySphere( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& spherePos, float sphereRadius, float& t );

    bool RayTriangle( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& v0,
                      const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v );

    bool RayAABB( const glm::vec3& rayPos, const glm::vec3& invRayDir, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float currentT = FLT_MAX );

    bool RayAABBFastest( const glm::vec3& rayPos, const glm::vec3& invRayDir, const int isDirNeg[3], const glm::vec3& aabbMin, const glm::vec3& aabbMax, float currentT = FLT_MAX );

} // namespace intersect 
} // namespace PT