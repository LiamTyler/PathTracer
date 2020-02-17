#include <iostream>
#include "configuration.hpp"
#include "path_tracer.hpp"
#include "resource/resource_manager.hpp"
#include "utils/time.hpp"

using namespace PT;

int main( int argc, char** argv )
{
    if ( argc != 2 )
    {
        std::cout << "Usage: pathTracer SCENE_FILE" << std::endl;
        return 0;
    }

    Scene scene;
    PathTracer pathTracer;

    auto sceneStartTime = Time::GetTimePoint();
    if ( !scene.Load( argv[1] ) )
    {
        std::cout << "Could not load scene file '" << argv[1] << "'" << std::endl;
        return 0;
    }
    pathTracer.InitImage( scene.imageResolution.x, scene.imageResolution.y );
    std::cout << "Scene loaded + path tracer initialized in: " << Time::GetDuration( sceneStartTime ) << " ms" << std::endl;
    
    pathTracer.Render( &scene );

    if ( !pathTracer.SaveImage( scene.outputImageFilename ) )
    {
        std::cout << "Could not save image '" << scene.outputImageFilename << "'" << std::endl;
    }

    return 0;
}
