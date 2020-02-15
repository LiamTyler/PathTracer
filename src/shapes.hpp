#pragma once

#include "glm/glm.hpp"
#include "resource/material.hpp"

namespace PT
{

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