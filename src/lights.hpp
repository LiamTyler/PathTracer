#pragma once

#include "glm/glm.hpp"

namespace PT
{

struct Light
{
    virtual ~Light() = default;

    glm::vec3 color    = glm::vec3( 1, 1, 1 );

    virtual glm::vec3 GetLightDirAndAttenuation( const glm::vec3& pos, float& atten ) const = 0;
};

struct PointLight : public Light
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );

    glm::vec3 GetLightDirAndAttenuation( const glm::vec3& pos, float& atten ) const override;
};

struct DirectionalLight : public Light
{
    glm::vec3 direction = glm::vec3( 0, -1, 0 );

    glm::vec3 GetLightDirAndAttenuation( const glm::vec3& pos, float& atten ) const override;
};

} // namespace PT
