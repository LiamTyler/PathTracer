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
    Transform Transpose() const;
    glm::vec3 TransformPoint( const glm::vec3& p ) const;
    glm::vec3 TransformVector( const glm::vec3& v ) const;
    Ray operator*( const Ray& ray ) const;
    glm::vec4 operator*( const glm::vec4& v ) const;

    glm::mat4 matrix;
};

} // namespace PT