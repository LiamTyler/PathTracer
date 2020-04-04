#include "lights.hpp"
#include "shapes.hpp"

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

LightIlluminationInfo AreaLight::GetLightIlluminationInfo( const glm::vec3& pos ) const
{
    SurfaceInfo surfInfo = shape->Sample();
    LightIlluminationInfo info;
    info.distanceToLight = glm::length( surfInfo.position - pos );
    info.dirToLight      = glm::normalize( surfInfo.position - pos );
    info.attenuation     = glm::max( 0.0f, glm::dot( -info.dirToLight, surfInfo.normal ) ) / ( info.distanceToLight * info.distanceToLight );

    return info;
}

} // namespace PT
