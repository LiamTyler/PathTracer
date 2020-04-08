#include "path_tracer.hpp"
#include "core_defines.hpp"
#include "glm/ext.hpp"
#include "sampling.hpp"
#include "tonemap.hpp"
#include "utils/random.hpp"
#include "utils/time.hpp"
#include <algorithm>
#include <atomic>
#include <fstream>

#define TONEMAP_AND_GAMMA IN_USE
#define PROGRESS_BAR_STR "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
#define PROGRESS_BAR_WIDTH 60
#define EPSILON 0.00001f

namespace PT
{   

float Fresnel(const glm::vec3& I, const glm::vec3& N, const float &ior )
{
    // this happens when the material is reflective, but not refractive
    if ( ior == 1 )
    {
        return 1;
    }

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

glm::vec3 EstimateSingleDirect( Light* light, const IntersectionData& hitData, Scene* scene, const glm::vec3& brdf )
{
    Interaction it{ hitData.position, hitData.normal };
    glm::vec3 wi;
    float pdf;
    glm::vec3 Li = light->Sample_Li( it, wi, pdf, scene );
    if ( pdf == 0 )
    {
        return glm::vec3( 0 );
    }

    float cosineTerm = std::max( 0.f, glm::dot( hitData.normal, wi ) );
    return Li * brdf * cosineTerm / pdf;
}

glm::vec3 LDirect( const IntersectionData& hitData, Scene* scene, const glm::vec3& brdf )
{
    glm::vec3 L( 0 );
    for ( const auto& light : scene->lights )
    {
        glm::vec3 Ld( 0 );
        for ( int i = 0; i < light->nSamples; ++i )
        {
            Ld += EstimateSingleDirect( light, hitData, scene, brdf );
        }
        L += Ld / (float)light->nSamples;
    }
    return L * brdf;
}

glm::vec3 Li( const Ray& ray, Scene* scene )
{
    Ray currentRay           = ray;
    glm::vec3 L              = glm::vec3( 0 );
    glm::vec3 pathThroughput = glm::vec3( 1 );
    
    for ( int bounce = 0; bounce < scene->maxDepth; ++bounce )
    {
        IntersectionData hitData;
        if ( !scene->Intersect( currentRay, hitData ) )
        {
            L += pathThroughput * scene->LEnvironment( currentRay );
            break;
        }

        auto N             = hitData.normal;
        glm::vec3 albedo   = hitData.material->GetAlbedo( hitData.texCoords.x, hitData.texCoords.y );
        const auto& T      = hitData.tangent;
        const auto  B      = glm::cross( T, N );
        glm::mat3 TBN      = glm::mat3( T, B, N );

        // estimate direct
        L += LDirect( hitData, scene, albedo / M_PI );

        L += pathThroughput * hitData.material->Ke;

        // indirect
        glm::vec3 wi     = glm::normalize( TBN * CosineSampleHemisphere( Random::Rand(), Random::Rand() ) );
        glm::vec3 brdf   = albedo / M_PI;
        float pdf        = std::max( 0.f, glm::dot( wi, N ) ) / M_PI;
        if ( pdf == 0.f || brdf == glm::vec3( 0 ) )
        {
            break;
        }
        float cosineTerm = std::abs( glm::dot( wi, N ) );
        
        pathThroughput  *= (brdf * cosineTerm) / pdf;
        if ( pathThroughput == glm::vec3( 0 ) )
        {
            break;
        }

        currentRay = Ray( hitData.position + EPSILON * N, wi );
    }

    return L;
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

    std::atomic< int > renderProgress( 0 );
    int onePercent = static_cast< int >( std::ceil( renderedImage.GetHeight() / 100.0f ) );

    #pragma omp parallel for schedule( dynamic )
    for ( int row = 0; row < renderedImage.GetHeight(); ++row )
    {
        for ( int col = 0; col < renderedImage.GetWidth(); ++col )
        {
            glm::vec3 imagePlanePos = UL + dV * (float)row + dU * (float)col;

            glm::vec3 totalColor = glm::vec3( 0 );
            for ( int rayCounter = 0; rayCounter < scene->numSamplesPerPixel; ++rayCounter )
            {
                glm::vec3 antiAliasedPos = AntiAlias::Jitter( rayCounter, imagePlanePos, dU, dV );
                Ray ray                  = Ray( cam.position, glm::normalize( antiAliasedPos - ray.position ) );
                totalColor              += Li( ray, scene );
            }

            renderedImage.SetPixel( row, col, totalColor / (float)scene->numSamplesPerPixel );
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
    renderedImage.ForAllPixels( [&]( const glm::vec3& pixel )
        {
            glm::vec3 tonemapped = Uncharted2Tonemap( pixel, cam.exposure );
            return GammaCorrect( tonemapped, cam.gamma );
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
