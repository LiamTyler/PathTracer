#pragma once
#include "glm/glm.hpp"

namespace PT
{

struct PointLight
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );
    glm::vec3 color    = glm::vec3( 1, 1, 1 );
};

} // namespace PT