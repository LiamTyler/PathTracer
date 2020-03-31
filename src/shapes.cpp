#include "shapes.hpp"
#include "intersection_tests.hpp"

namespace PT
{

bool Sphere::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t;
    Ray localRay = worldToLocal * ray;
    if ( !intersect::RaySphere( localRay.position, localRay.direction, glm::vec3( 0 ), 1, t ) )
    {
        return false;
    }

    hitData->t        = t;
    hitData->material = material.get();
    hitData->position = ray.Evaluate( t );
    hitData->normal   = glm::normalize( hitData->position - position );

    glm::vec3 localPos   = localRay.Evaluate( t );
    float theta          = atan2( localPos.z, localPos.x );
    float phi            = acosf( -localPos.y );
    hitData->texCoords.x = -0.5f * (theta / M_PI + 1); 
    hitData->texCoords.y = phi / M_PI;

    return true;
}

AABB Sphere::WorldSpaceAABB() const
{
    const glm::vec3 extent = glm::vec3( radius );
    return AABB( position - extent, position + extent );
}

} // namespace PT