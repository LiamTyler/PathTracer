#pragma once

#include "math.hpp"

namespace PT
{

struct Transform
{
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::mat4 ModelMatrix() const;
    Ray WorldToLocal( const Ray& ray ) const;
};

} // namespace PT