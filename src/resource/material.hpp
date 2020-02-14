#pragma once

#include "math.hpp"
#include "resource/resource.hpp"

namespace PT
{

struct Material : public Resource
{
    glm::vec3 albedo;
};

} // namespace PT