#pragma once

#include "glm/glm.hpp"
#include "resource/material.hpp"

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
    std::shared_ptr< Material > material;
    glm::vec3 position;
    float radius;

    glm::vec3 GetNormal( const glm::vec3& p ) const
    {
        return glm::normalize( p - position );
    }
};

} // namespace PT