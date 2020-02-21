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
        hitData->position = ray.Evaluate( t );
        auto localPos     = localRay.Evaluate( t );
        float a           = transform.scale.x;
        float b           = transform.scale.y;
        float c           = transform.scale.z;
        hitData->normal   = glm::normalize( 2.0f * glm::vec3( localPos.x / (a*a), localPos.y / (b*b), localPos.z / (c*c) ) );
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