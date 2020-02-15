#pragma once

#include "math.hpp"

namespace PT
{

class Camera
{
public:
    Camera() = default;

    void UpdateOrientationVectors();

    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    float vfov         = glm::radians( 45.0f );
    float aspectRatio  = 16.0f / 9.0f;
    float exposure     = 1.0f;
    float gamma        = 1.0f;

    glm::vec3 GetViewDir() const;
    glm::vec3 GetUpDir() const;
    glm::vec3 GetRightDir() const;

private:
    glm::vec3 m_viewDir   = glm::vec3( 0, 0, -1 );
    glm::vec3 m_upDir     = glm::vec3( 0, 1, 0 );
    glm::vec3 m_rightDir  = glm::vec3( 1, 0, 0 );
};

} // namespace PT