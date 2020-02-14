#include <iostream>
#include "configuration.hpp"
#include "path_tracer.hpp"

using namespace PT;

int main( int argc, char** argv )
{
    if ( argc != 3 )
    {
        std::cout << "Usage: pathTracer INPUT_FILE OUTPUT_IMAGE" << std::endl;
        return 0;
    }

    Scene scene;
    if ( !scene.Load( argv[1] ) )
    {
        std::cout << "Could not load scene file '" << argv[1] << "'" << std::endl;
        return 0;
    }

    PathTracer pathTracer;
    pathTracer.InitImage( 1280, 720 );
    pathTracer.Render( &scene );

    if ( !pathTracer.SaveImage( argv[2] ) )
    {
        std::cout << "Could not save image '" << argv[2] << "'" << std::endl;
    }

    return 0;
}
