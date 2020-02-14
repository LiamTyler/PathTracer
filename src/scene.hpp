#pragma once

#include "camera.hpp"
#include "lights.hpp"
#include "shapes.hpp"
#include <string>
#include <vector>

namespace PT
{

class Scene
{
public:
    Scene() = default;

    bool Load( const std::string& filename );

    bool Intersect( const Ray& ray, IntersectionData& hitData );
    
    Camera camera;
    std::vector< Sphere > spheres;
    std::vector< PointLight > pointLights;
    glm::vec3 backgroundColor;
};

} // namespace PT