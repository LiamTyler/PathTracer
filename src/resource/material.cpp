#include "resource/material.hpp"
#include "intersection_tests.hpp"
#include "sampling.hpp"
#include "utils/random.hpp"

namespace PT
{

glm::vec3 BRDF::F( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const
{
    return albedo / M_PI;
}

glm::vec3 BRDF::Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, float& pdf ) const
{
    glm::vec3 localWi = CosineSampleHemisphere( Random::Rand(), Random::Rand() );
    worldSpace_wi     = T * localWi.x + B * localWi.y + N * localWi.z;
    pdf               = Pdf( worldSpace_wo, worldSpace_wi );
    return F( worldSpace_wo, worldSpace_wi );
}

float BRDF::Pdf( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const
{
    bool sameHemisphere = glm::dot( worldSpace_wo, N ) * glm::dot( worldSpace_wi, N ) > 0;
    return sameHemisphere ? AbsDot( worldSpace_wi, N ) / M_PI : 0;
}

glm::vec3 Material::GetAlbedo( const glm::vec2& texCoords ) const
{
    glm::vec3 color = albedo;
    if ( albedoTexture )
    {
        color *= glm::vec3( albedoTexture->GetPixel( texCoords.x, texCoords.y ) );
    }

    return color;
}

BRDF Material::ComputeBRDF( IntersectionData* surfaceInfo ) const
{
    BRDF brdf;
    brdf.albedo = GetAlbedo( surfaceInfo->texCoords );
    brdf.T      = surfaceInfo->tangent;
    brdf.B      = surfaceInfo->bitangent;
    brdf.N      = surfaceInfo->normal;

    return brdf;
}

} // namespace PT