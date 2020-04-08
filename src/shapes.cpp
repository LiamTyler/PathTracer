#include "shapes.hpp"
#include "intersection_tests.hpp"
#include "utils/random.hpp"
#include "resource/model.hpp"
#include "sampling.hpp"

namespace PT
{

Material* Sphere::GetMaterial() const
{
    return material.get();
}

float Sphere::Area() const
{
    return 4 * M_PI * radius * radius;
}

SurfaceInfo Sphere::Sample() const
{
    SurfaceInfo info;
    glm::vec3 randNormal = UniformSampleSphere( Random::Rand(), Random::Rand() );
    info.position        = position + radius * randNormal;
    info.normal          = randNormal;
    info.pdf             = 1.0f / Area();
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

float Triangle::Area() const
{
    const auto& v0 = mesh->data.vertices[i0];
    const auto& v1 = mesh->data.vertices[i1];
    const auto& v2 = mesh->data.vertices[i2];
    return 0.5f * glm::length( glm::cross( v1 - v0, v2 - v0 ) );
}

SurfaceInfo Triangle::Sample() const
{
    SurfaceInfo info;
    glm::vec2 sample = UniformSampleTriangle( Random::Rand(), Random::Rand() );
    float u = sample.x;
    float v = sample.y;
    const auto& obj = mesh->data;
    info.position   = u * obj.vertices[i0] + v * obj.vertices[i1] + ( 1 - u - v ) * obj.vertices[i2];
    info.normal     = glm::normalize( u * obj.normals[i0] + v * obj.normals[i1] + ( 1 - u - v ) * obj.normals[i2] );
    info.pdf        = 1.0f / Area();
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