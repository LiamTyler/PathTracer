#pragma once

#include "lights.hpp"
#include "glm/glm.hpp"
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
    
    std::vector< Sphere > spheres;
    std::vector< PointLight > pointLights;
    glm::vec3 backgroundColor;
};

} // namespace PT