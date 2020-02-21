#include "path_tracer.hpp"
#include "core_defines.hpp"
#include "glm/ext.hpp"
#include "utils/random.hpp"
#include "utils/time.hpp"
#include <algorithm>

#define TONEMAP_AND_GAMMA NOT_IN_USE

namespace PT
{

void PathTracer::InitImage( int width, int height )
{
    Random::SetSeed( time( NULL ) );
    renderedImage = Image( width, height );
}

glm::vec3 Illuminate( Scene* scene, const Ray& ray, const IntersectionData& hitData )
{
    // ambient
    glm::vec3 color = hitData.material->albedo * scene->ambientLight;

    IntersectionData shadowHit;
    for ( const auto& light : scene->lights )
    {
        LightIlluminationInfo info = light->GetLightIlluminationInfo( hitData.position );
        const auto& L = info.dirToLight;
        const auto& N = hitData.normal;
        const auto  V = glm::normalize( ray.position - hitData.position );

        Ray shadowRay( hitData.position + 0.0001f * N, L );
        if ( scene->Intersect( shadowRay, shadowHit ) && shadowHit.t < info.distanceToLight )
        {
            continue;
        }

        const auto& mat = *hitData.material;
        const auto I    = info.attenuation * light->color;
        // diffuse
        color += I * mat.albedo * std::max( glm::dot( N, L ), 0.0f );
        // specular
        color += I * mat.Ks * std::pow( std::max( 0.0f, glm::dot( V, glm::reflect( -L, N ) ) ), mat.Ns );
    }

    // reflection

    // refraction

    return color;
}

glm::vec3 ShootRay( const Ray& ray, Scene* scene )
{
    glm::vec3 pixelColor;
    IntersectionData hitData;
    if ( scene->Intersect( ray, hitData ) )
    {
        pixelColor = Illuminate( scene, ray, hitData );
    }
    else
    {
        pixelColor = scene->backgroundColor;
    }

    return pixelColor;
}

void PathTracer::Render( Scene* scene )
{
    auto timeStart = Time::GetTimePoint();
    assert( renderedImage.GetPixels() );
    Camera& cam = scene->camera;

    float halfHeight = std::tan( cam.vfov / 2 );
    float halfWidth  = halfHeight * cam.aspectRatio;
    glm::vec3 UL     = cam.position + cam.GetViewDir() + halfHeight * cam.GetUpDir() - halfWidth * cam.GetRightDir();
    glm::vec3 dU     = cam.GetRightDir() * (2 * halfWidth  / renderedImage.GetWidth());
    glm::vec3 dV     = -cam.GetUpDir()   * (2 * halfHeight / renderedImage.GetHeight());
    UL              += 0.5f * (dU + dV); // move to center of pixel

    auto antiAliasAlg        = AntiAlias::GetAlgorithm( cam.aaAlgorithm );
    auto antiAliasIterations = AntiAlias::GetIterations( cam.aaAlgorithm );

    #pragma omp parallel for schedule( dynamic )
    for ( int row = 0; row < renderedImage.GetHeight(); ++row )
    {
        Ray ray;
        ray.position = cam.position;
        for ( int col = 0; col < renderedImage.GetWidth(); ++col )
        {
            glm::vec3 imagePlanePos = UL + dV * (float)row + dU * (float)col;

            // do anti-aliasing by shooting more than 1 ray through the pixel in various directions
            glm::vec3 totalColor = glm::vec3( 0 );
            for ( int rayCounter = 0; rayCounter < antiAliasIterations; ++rayCounter )
            {
                glm::vec3 antiAliasedPos = antiAliasAlg( rayCounter, imagePlanePos, dU, dV );
                ray.direction            = glm::normalize( antiAliasedPos - ray.position );
                totalColor              += ShootRay( ray, scene );
            }

            renderedImage.SetPixel( row, col, totalColor / (float)antiAliasIterations );
        }
    }

    std::cout << "Rendered scene in " << Time::GetDuration( timeStart ) / 1000 << " seconds" << std::endl;
    
#if USING( TONEMAP_AND_GAMMA )
    renderedImage.ForAllPixels( [&cam]( const glm::vec3& pixel )
        {
            glm::vec3 hdrColor       = cam.exposure * pixel;
            glm::vec3 tonemapped     = hdrColor / ( glm::vec3( 1 ) + hdrColor );
            glm::vec3 gammaCorrected = glm::pow( tonemapped, glm::vec3( 1.0f / cam.gamma ) );
            return gammaCorrected;
        }
    );
#endif // #if USING( TONEMAP_AND_GAMMA )
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    return renderedImage.Save( filename );
}

} // namespace PT
