#include "lights.hpp"

namespace PT
{

glm::vec3 PointLight::GetLightDirAndAttenuation( const glm::vec3& pos, float& atten ) const
{
    glm::vec3 L = position - pos;
    float dist  = glm::length( L );
    atten       = 1.0f / ( dist * dist );
    return L / dist;
}

glm::vec3 DirectionalLight::GetLightDirAndAttenuation( const glm::vec3& pos, float& atten ) const
{
    atten = 1;
    return -direction;
}
} // namespace PT
