#pragma once

#include "image.hpp"
#include "scene.hpp"

namespace PT
{

class PathTracer
{
public:
    PathTracer() = default;

    void Render( Scene* scene, int samplesPerPixelIteration = 0 );

    bool SaveImage( const std::string& filename ) const;

    Image renderedImage;
};

} // namespace PT