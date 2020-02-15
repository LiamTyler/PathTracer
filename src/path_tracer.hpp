#pragma once

#include "image.hpp"
#include "scene.hpp"

namespace PT
{

class PathTracer
{
public:
    PathTracer() = default;

    void InitImage( int width, int height );

    void Render( Scene* scene );

    bool SaveImage( const std::string& filename ) const;

    Image renderedImage;
};

} // namespace PT