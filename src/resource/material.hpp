#pragma once

#include "math.hpp"
#include "resource/resource.hpp"
#include "resource/texture.hpp"
#include <memory>

namespace PT
{

struct Material : public Resource
{
    glm::vec3 albedo = glm::vec3( 1.0f, 0.1f, 0.6f );
    glm::vec3 Ks     = glm::vec3( 0.7f );
    float Ns         = 40.0f;
    glm::vec3 Tr     = glm::vec3( 0 );
    float ior        = 1.0f;
    std::shared_ptr< Texture > albedoTexture;
};

} // namespace PT