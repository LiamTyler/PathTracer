#pragma once

#include "bvh.hpp"
#include "camera.hpp"
#include "lights.hpp"
#include "resource/material.hpp"
#include "shapes.hpp"
#include "resource/skybox.hpp"
#include <string>
#include <vector>

namespace PT
{

class Scene
{
public:
    Scene() = default;
    ~Scene();

    bool Load( const std::string& filename );

    bool Intersect( const Ray& ray, IntersectionData& hitData );
    glm::vec3 LEnvironment( const Ray& ray );
    
    Camera camera;
    std::vector< std::shared_ptr< Shape > > shapes; // invalid after bvh is built. Use bvh.shapes
    std::vector< Light* > lights;
    glm::vec3 ambientLight          = glm::vec3( 0.1f );
    glm::vec3 backgroundColor       = glm::vec3( 0.1f );
    std::shared_ptr< Skybox > skybox;
    std::string outputImageFilename = "rendered.png";
    glm::ivec2 imageResolution      = glm::ivec2( 1280, 720 );
    int maxDepth                    = 5;
    int numSamplesPerAreaLight      = 16;
    int numSamplesPerPixel          = 32;
    BVH bvh;
};

} // namespace PT
