#include "lights.hpp"

namespace PT
{

LightIlluminationInfo PointLight::GetLightIlluminationInfo( const glm::vec3& pos ) const
{
    LightIlluminationInfo info;
    info.dirToLight      = position - pos;
    info.distanceToLight = glm::length( info.dirToLight );
    info.attenuation     = 1.0f / ( info.distanceToLight * info.distanceToLight );
    info.dirToLight      = info.dirToLight / info.distanceToLight;

    return info;
}

LightIlluminationInfo DirectionalLight::GetLightIlluminationInfo( const glm::vec3& pos ) const
{
    LightIlluminationInfo info;
    info.distanceToLight = FLT_MAX;
    info.attenuation     = 1.0f;
    info.dirToLight      = -direction;

    return info;
}

} // namespace PT
