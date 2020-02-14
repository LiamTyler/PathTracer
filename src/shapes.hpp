#pragma once

#include "glm/glm.hpp"

namespace PT
{

struct Ray
{
    Ray() = default;
    Ray( const glm::vec3& pos, const glm::vec3& dir ) :
        position( pos ),
        direction( dir )
    {
    }

    glm::vec3 Evaluate( float t ) const
    {
        return position + t * direction;
    }

    glm::vec3 position;
    glm::vec3 direction;
};

struct Sphere
{
    glm::vec3 position;
    float radius;

    glm::vec3 GetNormal( const glm::vec3& p ) const
    {
        return glm::normalize( p - position );
    }
};

struct IntersectionData
{
    Sphere* sphere;
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    float t;
};

} // namespace PT