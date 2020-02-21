#include "intersection_tests.hpp"
#include <algorithm>
#include <cmath>

namespace PT
{
namespace intersect
{

    bool RaySphere( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& spherePos, float radius, float& t )
    {
        glm::vec3 OC = rayPos - spherePos;
        float a      = glm::dot( rayDir, rayDir );
        float b      = 2 * glm::dot( OC, rayDir );
        float c      = glm::dot( OC, OC ) - radius * radius;

        // exit if ray is outside of sphere (c > 0) and ray is pointing away from sphere (b > 0)
        if ( c > 0 && b > 0 )
        {
            return false;
        }

        float disc = b*b - 4*a*c;
        if ( disc < 0 )
        {
            return false;
        }

        float d = std::sqrt( disc );
        t = (-b - d) / (2*a);
        if ( t < 0 )
        {
            t = (-b + d) / (2*a);
        }

        return true;
    }

    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
    bool RayTriangle( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& v0,
                      const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v )
    {
        glm::vec3 v0v1 = v1 - v0;
        glm::vec3 v0v2 = v2 - v0;
        glm::vec3 pvec = glm::cross( v0v2, rayDir );
        float det      = glm::dot( v0v1, pvec );
        // ray and triangle are parallel if det is close to 0
        if ( fabs( det ) < 0.00000001 )
        {
            return false;
        }

        float invDet = 1 / det;

        glm::vec3 tvec = rayPos - v0;
        u              = glm::dot( tvec, pvec ) * invDet;
        if ( u < 0 || u > 1 )
        {
            return false;
        }

        glm::vec3 qvec = glm::cross( v0v1, tvec );
        v              = glm::dot( rayDir, qvec ) * invDet;
        if ( v < 0 || u + v > 1 )
        {
            return false;
        }

        t = glm::dot( v0v2, qvec) * invDet;

        return true;
    }

    bool RayAABB( const glm::vec3& rayPos, const glm::vec3& invRayDir, const glm::vec3& aabbMin, const glm::vec3& aabbMax )
    {
        float tx1 = (aabbMin.x - rayPos.x) * invRayDir.x;
        float tx2 = (aabbMax.x - rayPos.x) * invRayDir.x;

        float tmin = std::min( tx1, tx2 );
        float tmax = std::max( tx1, tx2 );

        float ty1 = (aabbMin.y - rayPos.y) * invRayDir.y;
        float ty2 = (aabbMax.y - rayPos.y) * invRayDir.y;

        tmin = std::max( tmin, std::min( ty1, ty2 ) );
        tmax = std::min( tmax, std::max( ty1, ty2 ) );

        float tz1 = (aabbMin.z - rayPos.z) * invRayDir.z;
        float tz2 = (aabbMax.z - rayPos.z) * invRayDir.z;

        tmin = std::max( tmin, std::min( tz1, tz2 ) );
        tmax = std::min( tmax, std::max( tz1, tz2 ) );

        return tmax >= std::max( 0.0f, tmin );
    }

} // namespace intersect 
} // namespace PT
