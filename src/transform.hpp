#pragma once

#include "math.hpp"

namespace PT
{

struct Transform
{
    Transform();
    Transform( const glm::mat4& mat );
    Transform( const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale );

    Transform Inverse() const;
    Ray operator*( const Ray& ray ) const;

    glm::mat4 matrix;
};

} // namespace PT