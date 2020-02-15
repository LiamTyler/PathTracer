#pragma once

#include "camera.hpp"
#include "lights.hpp"
#include "resource/material.hpp"
#include "shapes.hpp"
#include <string>
#include <vector>

namespace PT
{

struct IntersectionData
{
    Sphere* sphere;
    glm::vec3 position;
    glm::vec3 normal;
    Material* material;
    float t;
};

class Scene
{
public:
    Scene() = default;
    ~Scene();

    bool Load( const std::string& filename );

    bool Intersect( const Ray& ray, IntersectionData& hitData );
    
    Camera camera;
    std::vector< Sphere > spheres;
    std::vector< Light* > lights;
    glm::vec3 backgroundColor       = glm::vec3( 0.1f );
    std::string outputImageFilename = "rendered.png";
    glm::ivec2 imageResolution      = glm::ivec2( 1280, 720 );
};

} // namespace PT
