#pragma once

#include "math.hpp"

namespace PT
{

struct AABB
{
    AABB();
    AABB( const glm::vec3& _min, const glm::vec3& _max );

    void Union( const AABB& aabb );
    void Union( const glm::vec3& point );
    glm::vec3 Centroid() const;
    int LongestDimension() const;

    glm::vec3 min;
    glm::vec3 max;
};

} // namespace PT