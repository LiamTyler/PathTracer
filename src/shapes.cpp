#include "shapes.hpp"
#include "intersection_tests.hpp"
#include "resource/model.hpp"

namespace PT
{

Material* Sphere::GetMaterial() const
{
    return material.get();
}

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

    hitData->tangent   = glm::vec3( -sin( theta ), 0, cos( theta ) );
    hitData->bitangent = glm::cross( hitData->normal, hitData->tangent );

    return true;
}

AABB Sphere::WorldSpaceAABB() const
{
    const glm::vec3 extent = glm::vec3( radius );
    return AABB( position - extent, position + extent );
}


Material* Triangle::GetMaterial() const
{
    return model->materials[materialIndex].get();
}

bool Triangle::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t, u, v;
    if ( intersect::RayTriangle( ray.position, ray.direction, model->vertices[i0], model->vertices[i1], model->vertices[i2], t, u, v, hitData->t ) )
    {
        hitData->t         = t;
        hitData->material  = model->materials[materialIndex].get();
        hitData->position  = ray.Evaluate( t );
        hitData->normal    = glm::normalize( ( 1 - u - v ) * model->normals[i0] + u * model->normals[i1] + v * model->normals[i2] );
        hitData->tangent   = glm::normalize( ( 1 - u - v ) * model->tangents[i0] + u * model->tangents[i1] + v * model->tangents[i2] );
        hitData->bitangent = glm::cross( hitData->normal, hitData->tangent );
        hitData->texCoords = ( 1 - u - v ) * model->uvs[i0] + u * model->uvs[i1] + v * model->uvs[i2];
        return true;
    }
    return false;
}

AABB Triangle::WorldSpaceAABB() const
{
    AABB aabb;
    aabb.Union( model->vertices[i0] );
    aabb.Union( model->vertices[i1] );
    aabb.Union( model->vertices[i2] );
    return aabb;
}

} // namespace PT