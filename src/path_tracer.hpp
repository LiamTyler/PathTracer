#pragma once

#include "scene.hpp"
#include "image.hpp"

namespace PT
{

class PathTracer
{
public:
    PathTracer() = default;

    void InitImage( unsigned int width, unsigned int height );

    void Render( Scene* scene );

    bool SaveImage( const std::string& filename ) const;

    Image renderedImage;
};

} // namespace PT