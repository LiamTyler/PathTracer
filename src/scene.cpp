#include "scene.hpp"
#include "intersection_tests.hpp"

namespace PT
{

bool Scene::Load( const std::string& filename )
{
    PointLight p{ glm::vec3( 0, 10, -10 ), glm::vec3( 1, 1, 1 ) };
    pointLights.push_back( p );

    Sphere s{ glm::vec3( 0, 0, -10 ), 2 };
    spheres.push_back( s );

    backgroundColor = glm::vec3( 0.1, 0.1, 0.1 );

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    float closestTime         = FLT_MAX;
    size_t closestSphereIndex = ~0;
    float t;
    for ( size_t i = 0; i < spheres.size(); ++i )
    {
        const Sphere& s = spheres[i];
        if ( intersect::RaySphere( ray.position, ray.direction, s.position, s.radius, t ) )
        {
            if ( t < closestTime )
            {
                closestTime        = t;
                closestSphereIndex = i;
            }
        }
    }

    if ( closestSphereIndex != ~0 )
    {
        hitData.t        = closestTime;
        hitData.sphere   = &spheres[closestSphereIndex];
        hitData.color    = glm::vec3( 1, 0, 0 );
        hitData.position = ray.Evaluate( hitData.t );
        hitData.normal   = spheres[closestSphereIndex].GetNormal( hitData.position );
        return true;
    }

    return false;
}

} // namespace PT