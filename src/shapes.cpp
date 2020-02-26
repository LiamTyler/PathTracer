#include "shapes.hpp"
#include "intersection_tests.hpp"

namespace PT
{

bool Sphere::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t;
    const auto localRay = transform.WorldToLocal( ray );
    if ( !intersect::RaySphere( localRay.position, localRay.direction, glm::vec3( 0 ), 1, t ) )
    {
        return false;
    }
    if ( t < hitData->t )
    {
        hitData->t           = t;
        hitData->material    = material.get();
        hitData->position    = ray.Evaluate( t );
        auto localPos        = localRay.Evaluate( t );
        hitData->normal      = glm::normalize( glm::vec3( glm::inverse( glm::transpose( transform.ModelMatrix() ) ) * glm::vec4( localPos, 0 ) ) );
        hitData->texCoords.x = -(1 + atan2( localPos.z, localPos.x ) / M_PI) * 0.5; 
        hitData->texCoords.y = acosf( -localPos.y ) / M_PI;
    }

    return true;
}

bool ModelInstance::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    int materialIndex;  
    const auto localRay = transform.WorldToLocal( ray );
    if ( !model->IntersectRay( localRay, *hitData, materialIndex ) )
    {
        return false;
    }
    hitData->material = materials[materialIndex].get();
    hitData->position = ray.Evaluate( hitData->t );
    hitData->normal   = glm::normalize( glm::vec3( glm::inverse( glm::transpose( transform.ModelMatrix() ) ) * glm::vec4( hitData->normal, 0 ) ) );

    return true;
}

} // namespace PT