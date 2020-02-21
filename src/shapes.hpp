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
};

struct Sphere : public Shape
{
    std::shared_ptr< Material > material;
    Transform transform;

    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
};

struct ModelInstance : public Shape
{
    Transform transform;
    std::shared_ptr< Model > model;
    std::vector< std::shared_ptr< Material > > materials;

    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
};

} // namespace PT
