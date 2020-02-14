#include "intersection_tests.hpp"
#include <cmath>

namespace PT
{
namespace intersect
{

    bool RaySphere( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& spherePos, float radius, float& t )
    {
        glm::vec3 OC = rayPos - spherePos;
        float b      = glm::dot( OC, rayDir );
        float c      = glm::dot( OC, OC ) - radius * radius;

        // exit if ray is outside of sphere (c > 0) and ray is pointing away from sphere (b > 0)
        if ( c > 0 && b > 0 )
        {
            return false;
        }

        float disc = b*b - c;
        if ( disc < 0 )
        {
            return false;
        }

        float d = std::sqrt( disc );
        t = -b - d;
        if ( t < 0 )
        {
            t = -b + d;
        }

        return true;
    }

} // namespace intersect 
} // namespace PT
