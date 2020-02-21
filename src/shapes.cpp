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
        hitData->t        = t;
        hitData->material = material.get();
        hitData->position = localRay.Evaluate( t );
        hitData->normal   = glm::normalize( hitData->position );
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

    return true;
}

} // namespace PT