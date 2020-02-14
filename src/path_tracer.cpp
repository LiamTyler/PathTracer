#include "path_tracer.hpp"
#include "glm/ext.hpp"
#include "utils/time.hpp"
#include <algorithm>

namespace PT
{

void PathTracer::InitImage( int width, int height )
{
    renderedImage = Image( width, height );
}

glm::vec3 Illuminate( Scene* scene, const Ray& ray, const IntersectionData& hitData )
{
    glm::vec3 color( 0 );
    for ( const auto& light : scene->pointLights )
    {
        glm::vec3 L = light.position - hitData.position;
        float dist  = glm::length( L );
        L /= dist;
        float attenuation = 1.0f / ( dist * dist );

        color += attenuation * light.color * hitData.color * std::max( glm::dot( hitData.normal, L ), 0.0f );
    }

    return color;
}

void PathTracer::Render( Scene* scene )
{
    auto timeStart = Time::GetTimePoint();
    assert( renderedImage.GetPixels() );
    Camera& cam = scene->camera;

    float halfHeight = std::tanf( cam.vfov / 2 );
    float halfWidth  = halfHeight * cam.aspectRatio;
    glm::vec3 UL     = cam.position + cam.GetViewDir() + halfHeight * cam.GetUpDir() - halfWidth * cam.GetRightDir();
    glm::vec3 dU     = cam.GetRightDir() * (2 * halfWidth  / renderedImage.GetWidth());
    glm::vec3 dV     = -cam.GetUpDir()   * (2 * halfHeight / renderedImage.GetHeight());
    UL               += 0.5f * (dU + dV); // move to center of pixel

    #pragma omp parallel for
    for ( int row = 0; row < renderedImage.GetHeight(); ++row )
    {
        Ray ray;
        ray.position = cam.position;
        for ( int col = 0; col < (int)renderedImage.GetWidth(); ++col )
        {
            glm::vec3 imagePlanePos = UL + dV * (float)row + dU * (float)col;
            ray.direction           = glm::normalize( imagePlanePos - ray.position );

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

            renderedImage.SetPixel( row, col, pixelColor );
        }
    }

    std::cout << "Rendered scene in " << Time::GetDuration( timeStart ) << " ms" << std::endl;
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    return renderedImage.Save( filename );
}

} // namespace PT