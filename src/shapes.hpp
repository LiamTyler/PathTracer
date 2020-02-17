#pragma once

#include "glm/glm.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "transform.hpp"
#include <memory>

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

struct WorldObject
{
    Transform transform;
    std::shared_ptr< Model > model;
    std::vector< std::shared_ptr< Material > > materials;
};

} // namespace PT
