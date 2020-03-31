#pragma once

#include "glm/glm.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "transform.hpp"
#include <memory>

namespace PT
{

struct Shape
{
    Shape() = default;

    virtual bool Intersect( const Ray& ray, IntersectionData* hitData ) const = 0;
    virtual AABB WorldSpaceAABB() const = 0;
};

struct Sphere : public Shape
{
    std::shared_ptr< Material > material;
    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    float radius       = 1;

    Transform worldToLocal;

    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    AABB WorldSpaceAABB() const override;
};

} // namespace PT
