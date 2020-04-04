#pragma once

#include "math.hpp"
#include "resource/resource.hpp"
#include "resource/texture.hpp"
#include <memory>

namespace PT
{

struct Material : public Resource
{
    glm::vec3 albedo = glm::vec3( 0.0f );
    glm::vec3 Ks     = glm::vec3( 0.0f );
    glm::vec3 Ke     = glm::vec3( 0.0f );
    float Ns         = 0.0f;
    glm::vec3 Tr     = glm::vec3( 0 );
    float ior        = 1.0f;
    std::shared_ptr< Texture > albedoTexture;

    glm::vec3 GetAlbedo( float u, float v ) const
    {
        glm::vec3 color = albedo;
        if ( albedoTexture )
        {
            color *= glm::vec3( albedoTexture->GetPixel( u, v ) );
        }

        return color;
    }
};

} // namespace PT