#include "path_tracer.hpp"
#include "core_defines.hpp"
#include "glm/ext.hpp"
#include "tonemap.hpp"
#include "utils/random.hpp"
#include "utils/time.hpp"
#include <algorithm>
#include <atomic>

#define TONEMAP_AND_GAMMA IN_USE
#define PROGRESS_BAR_STR "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
#define PROGRESS_BAR_WIDTH 60

namespace PT
{

glm::vec3 ShootRay( const Ray& ray, Scene* scene, int depth );

float Fresnel(const glm::vec3& I, const glm::vec3& N, const float &ior )
{
    float cosi = std::min( 1.0f, std::max( 1.0f, glm::dot( I, N ) ) );
    float etai = 1, etat = ior;
    if ( cosi > 0 )
    {
        std::swap(etai, etat);
    }

    float sint = etai / etat * sqrtf( std::max( 0.0f, 1 - cosi * cosi ) );

    float kr;
    if ( sint >= 1 )
    {
        kr = 1;
    }
    else
    {
        float cost = sqrtf( std::max( 0.0f, 1 - sint * sint ) );
        cosi = fabsf( cosi );
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }

    return kr;
}

glm::vec3 Refract(const glm::vec3& I, const glm::vec3& N, const float &ior ) 
{ 
    float cosi  = std::min( 1.0f, std::max( 1.0f, glm::dot( I, N ) ) );
    float etai  = 1, etat = ior;
    glm::vec3 n = N; 
    if ( cosi < 0 )
    {
        cosi = -cosi;
    }
    else
    {
        std::swap( etai, etat );
        n = -N;
    } 
    float eta = etai / etat; 
    float k   = 1 - eta * eta * (1 - cosi * cosi); 
    return k < 0 ? glm::vec3( 0 ) : eta * I + (eta * cosi - sqrtf( k )) * n; 
} 

glm::vec3 Illuminate( Scene* scene, const Ray& ray, const IntersectionData& hitData, int depth )
{
    auto N             = hitData.normal;
    const auto V       = -ray.direction;
    const auto& mat    = *hitData.material;
    glm::vec3 fixedPos = hitData.position + 0.0001f * N;
    glm::vec3 albedo   = mat.GetAlbedo( hitData.texCoords.x, hitData.texCoords.y );

    // ambient
    glm::vec3 color = glm::vec3( 0 );
    color += mat.Ke;
    color += albedo * scene->ambientLight;

    for ( const auto& light : scene->lights )
    {
        glm::vec3 sampledColor( 0 );
        for ( int i = 0; i < light->nSamples; ++i )
        {
            LightIlluminationInfo info;
            info = light->GetLightIlluminationInfo( fixedPos );
            const auto& L = info.dirToLight;
        
            IntersectionData shadowHit;
            Ray shadowRay( fixedPos, L );
            if ( scene->Intersect( shadowRay, shadowHit ) && shadowHit.t + 0.0001f < info.distanceToLight )
            {
                continue;
            }

            const auto I = info.attenuation * light->color;
            // diffuse
            sampledColor += I * albedo * std::max( glm::dot( N, L ), 0.0f );
            // specular
            sampledColor += I * mat.Ks * std::pow( std::max( 0.0f, glm::dot( V, glm::reflect( -L, N ) ) ), mat.Ns );
        }
        color += sampledColor / (float)light->nSamples;
    }

    // reflection & refraction
    glm::vec3 reflectColor( 0 );
    glm::vec3 refractColor( 0 );

    float iorCurrent      = 1;
    float iorEntering     = mat.ior;
    bool rayOutsideObject = glm::dot( N, ray.direction ) < 0;
    float kr              = Fresnel( ray.direction, N, iorEntering );
    if ( !rayOutsideObject )
    {
        N = -N;
    }
    if ( mat.Ks != glm::vec3( 0 ) )
    {
        Ray reflectRay( hitData.position + 0.0001f * N, glm::normalize( glm::reflect( ray.direction, N ) ) );
        reflectColor += mat.Ks * ShootRay( reflectRay, scene, depth + 1 );
    }

    if ( mat.Tr != glm::vec3( 0 ) && kr < 1 )
    {
        Ray refractRay( hitData.position - 0.0001f * N, Refract( ray.direction, hitData.normal, mat.ior ) );
        assert( refractRay.direction != glm::vec3( 0 ) );
        refractColor += mat.Tr * ShootRay( refractRay, scene, depth + 1 );
    }

    color += kr * reflectColor + (1 - kr) * refractColor;
    return color;
}

glm::vec3 ShootRay( const Ray& ray, Scene* scene, int depth )
{
    glm::vec3 pixelColor;
    IntersectionData hitData;
    if ( depth < scene->maxDepth && scene->Intersect( ray, hitData ) )
    {
        pixelColor = Illuminate( scene, ray, hitData, depth );
    }
    else
    {
        pixelColor = scene->GetBackgroundColor( ray );
    }

    return pixelColor;
}

void PathTracer::Render( Scene* scene )
{
    renderedImage = Image( scene->imageResolution.x, scene->imageResolution.y );
    std::cout << "\nRendering scene..." << std::endl;

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

    std::atomic< int > renderProgress( 0 );
    int onePercent = static_cast< int >( std::ceil( renderedImage.GetHeight() / 100.0f ) );

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
                totalColor              += ShootRay( ray, scene, 0 );
            }

            renderedImage.SetPixel( row, col, totalColor / (float)antiAliasIterations );
        }

        int rowsCompleted = ++renderProgress;
        if ( rowsCompleted % onePercent == 0 )
        {
            float progress = rowsCompleted / (float) renderedImage.GetHeight();
            int val  = (int) (progress * 100);
            int lpad = (int) (progress * PROGRESS_BAR_WIDTH);
            int rpad = PROGRESS_BAR_WIDTH - lpad;
            printf( "\r%3d%% [%.*s%*s]", val, lpad, PROGRESS_BAR_STR, rpad, "" );
            fflush( stdout );
        }
    }

    std::cout << "\nRendered scene in " << Time::GetDuration( timeStart ) / 1000 << " seconds" << std::endl;
    
#if USING( TONEMAP_AND_GAMMA )
    renderedImage.ForAllPixels( [&cam]( const glm::vec3& pixel )
        {
            glm::vec3 tonemapped = Uncharted2Tonemap( pixel, cam.exposure );
            //return GammaCorrect( tonemapped, cam.gamma );
            return tonemapped;
        }
    );
#endif // #if USING( TONEMAP_AND_GAMMA )
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    return renderedImage.Save( filename );
}

} // namespace PT
