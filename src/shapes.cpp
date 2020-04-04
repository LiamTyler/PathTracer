#include "shapes.hpp"
#include "intersection_tests.hpp"
#include "utils/random.hpp"
#include "resource/model.hpp"

namespace PT
{

Material* Sphere::GetMaterial() const
{
    return material.get();
}

SurfaceInfo Sphere::Sample() const
{
    SurfaceInfo info;
    info.position = position + radius * glm::normalize( glm::vec3( Random::Rand(), Random::Rand(), Random::Rand() ) );
    info.normal   = glm::normalize( info.position - position );
    return info;
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
    return mesh->data.material.get();
}

SurfaceInfo Triangle::Sample() const
{
    SurfaceInfo info;
    float u = Random::Rand();
    float v = Random::Rand();
    const auto& obj = mesh->data;
    info.position   = ( 1 - u - v ) * obj.vertices[i0]  + u * obj.vertices[i1] + v * obj.vertices[i2];
    info.normal     = glm::normalize( ( 1 - u - v ) * obj.normals[i0] + u * obj.normals[i1] + v * obj.normals[i2] );
    return info;
}

bool Triangle::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t, u, v;
    const auto& obj = mesh->data;
    if ( intersect::RayTriangle( ray.position, ray.direction, obj.vertices[i0], obj.vertices[i1], obj.vertices[i2], t, u, v, hitData->t ) )
    {
        hitData->t         = t;
        hitData->material  = obj.material.get();
        hitData->position  = ray.Evaluate( t );
        hitData->normal    = glm::normalize( ( 1 - u - v ) * obj.normals[i0]  + u * obj.normals[i1]  + v * obj.normals[i2] );
        hitData->tangent   = glm::normalize( ( 1 - u - v ) * obj.tangents[i0] + u * obj.tangents[i1] + v * obj.tangents[i2] );
        hitData->bitangent = glm::cross( hitData->normal, hitData->tangent );
        hitData->texCoords = ( 1 - u - v ) * obj.uvs[i0] + u * obj.uvs[i1] + v * obj.uvs[i2];
        return true;
    }
    return false;
}

AABB Triangle::WorldSpaceAABB() const
{
    AABB aabb;
    aabb.Union( mesh->data.vertices[i0] );
    aabb.Union( mesh->data.vertices[i1] );
    aabb.Union( mesh->data.vertices[i2] );
    return aabb;
}

} // namespace PT