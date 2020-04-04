#pragma once

#include "glm/glm.hpp"
#include <memory>

namespace PT
{

struct LightIlluminationInfo
{
    float attenuation;
    float distanceToLight;
    glm::vec3 dirToLight;
};

struct Light
{
    virtual ~Light() = default;

    glm::vec3 color = glm::vec3( 1, 1, 1 );
    int nSamples    = 1;

    virtual LightIlluminationInfo GetLightIlluminationInfo( const glm::vec3& pos ) const = 0;
};

struct PointLight : public Light
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );

    LightIlluminationInfo GetLightIlluminationInfo( const glm::vec3& pos ) const override;
};

struct DirectionalLight : public Light
{
    glm::vec3 direction = glm::vec3( 0, -1, 0 );

    LightIlluminationInfo GetLightIlluminationInfo( const glm::vec3& pos ) const override;
};

struct Shape;
// Area lights are generated whenever the shape's material is emissive. One area light per
// shape. Only used if you want to loop directly over a list of all lights / lit surfaces.
// Otherwise during rendering, the materials emissive value is just used.
struct AreaLight : public Light
{
    std::shared_ptr< Shape > shape;

    LightIlluminationInfo GetLightIlluminationInfo( const glm::vec3& pos ) const override;
};

} // namespace PT
