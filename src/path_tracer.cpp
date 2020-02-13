#include "path_tracer.hpp"

namespace PT
{

void PathTracer::InitImage( unsigned int width, unsigned int height )
{
    renderedImage = Image( width, height );
}

void PathTracer::Render( Scene* scene )
{
    assert( renderedImage.GetPixels() );

    for ( unsigned row = 0; row < renderedImage.GetHeight(); ++row )
    {
        for ( unsigned col = 0; col < renderedImage.GetWidth(); ++col )
        {
            glm::vec3 color = glm::vec3( 10, 2, -1 );
            renderedImage.SetPixel( row, col, color );
        }
    }
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    return renderedImage.Save( filename );
}

} // namespace PT